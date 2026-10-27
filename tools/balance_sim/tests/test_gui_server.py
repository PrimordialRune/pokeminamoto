"""Starts the GUI server on a random port and hits every endpoint. Run from tools/balance_sim:
    python3 -m unittest tests.test_gui_server -v
"""
import json
import os
import struct
import sys
import threading
import time
import unittest
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from minasim.gui import server, sprites  # noqa: E402

OUT = Path(os.environ.get("MINASIM_TEST_OUT",
           "/tmp/claude-0/-home-user-pokeminamoto/762d4f20-40fd-5f4b-b29c-91a9476d2301/scratchpad/ui-backend"))


def png_info(b):
    assert b[:8] == b"\x89PNG\r\n\x1a\n", "not a PNG"
    w, h = struct.unpack(">II", b[16:24])
    return w, h, b[24], b[25]  # bitdepth, colortype


class GuiServerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.srv = server.make_server(0)
        cls.port = cls.srv.server_address[1]
        cls.t = threading.Thread(target=cls.srv.serve_forever, daemon=True)
        cls.t.start()
        cls.base = "http://127.0.0.1:%d" % cls.port

    @classmethod
    def tearDownClass(cls):
        cls.srv.shutdown()
        cls.srv.server_close()

    def get(self, path, headers=None):
        req = urllib.request.Request(self.base + path, headers=headers or {})
        try:
            with urllib.request.urlopen(req) as r:
                return r.status, dict(r.headers), r.read()
        except urllib.error.HTTPError as e:
            return e.code, dict(e.headers), e.read()

    def post(self, path, obj):
        req = urllib.request.Request(self.base + path, data=json.dumps(obj).encode(), method="POST",
                                     headers={"Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(req) as r:
                return r.status, json.loads(r.read())
        except urllib.error.HTTPError as e:
            return e.code, json.loads(e.read())

    def test_index_and_static(self):
        s, h, b = self.get("/")
        self.assertEqual(s, 200)
        self.assertIn(b"<html", b.lower())
        self.assertEqual(self.get("/../server.py")[0] in (403, 404), True)
        self.assertEqual(self.get("/%2e%2e/server.py")[0] in (403, 404), True)

    def test_status_runs_report(self):
        s, h, b = self.get("/api/status")
        self.assertEqual(s, 200)
        st = json.loads(b)
        s, h, b = self.get("/api/runs")
        runs = json.loads(b)
        self.assertEqual(s, 200)
        self.assertGreaterEqual(len(runs), 1)
        if st["demo"]:
            self.assertEqual(h.get("X-Minasim-Demo"), "1")
            self.assertGreaterEqual(len(runs), 2)
        rid = runs[0]["run_id"]
        s, h, b = self.get("/api/report/" + rid)
        self.assertEqual(s, 200)
        rep = json.loads(b)
        for k in ("meta", "stages", "flags", "species", "moves", "fights", "items", "tp", "level_curve"):
            self.assertIn(k, rep)
        self.assertEqual(self.get("/api/report/nope")[0], 404)
        self.assertEqual(self.get("/api/report/..%2F..%2Fetc")[0], 404)

    def test_fixture_integrity(self):
        data = json.loads(self.get("/api/data")[2])
        ids = {s["id"] for s in data["species"]}
        mv = {m["id"] for m in data["moves"]}
        s, h, b = self.get("/api/runs")
        rep = json.loads(self.get("/api/report/" + json.loads(b)[0]["run_id"])[2])
        if not rep["meta"].get("demo"):
            self.skipTest("real run")
        for f in rep["fights"]:
            for m in f["party"]:
                self.assertIn(m["species"], ids, m)
        for m in rep["moves"]:
            self.assertIn(m["move"], mv)
        self.assertEqual(len({s["species"] for s in rep["species"]}), 151)
        self.assertGreaterEqual(len(rep["flags"]), 25)
        self.assertGreaterEqual(len(rep["fights"]), 40)
        self.assertEqual({f["kind"] for f in rep["flags"]},
                         {"species", "move", "fight", "item", "tp", "level_curve", "special", "talk", "rng", "dominance", "dead_content"})

    def test_data(self):
        s, h, b = self.get("/api/data", {"Accept-Encoding": "gzip"})
        self.assertEqual(s, 200)
        import gzip
        d = json.loads(gzip.decompress(b) if h.get("Content-Encoding") == "gzip" else b)
        self.assertEqual(len(d["species"]), 151)
        bulb = d["species"][0]
        self.assertEqual(bulb["id"], "BULBASAUR")
        self.assertEqual(bulb["minamoto"], "FUSHIGIDANE")
        self.assertEqual(bulb["types"], ["GRASS", "POISON"])
        self.assertTrue(bulb["learnset"]["level"] and bulb["evolutions"])
        for k in ("moves", "items", "trainers", "stages"):
            self.assertTrue(d[k], k)
        ember = next(m for m in d["moves"] if m["id"] == "EMBER")
        self.assertEqual((ember["power"], ember["type"], ember["tp_cost"]), (40, "FIRE", 7))
        potion = next(i for i in d["items"] if i["id"] == "POTION")
        self.assertEqual(potion["price"], 300)
        etag = h["ETag"]
        self.assertEqual(self.get("/api/data", {"If-None-Match": etag})[0], 304)

    def test_duel_and_run_without_engine(self):
        eng, _ = server._engine_mods()
        s, o = self.post("/api/duel", {"player": [{"species": "CHARMANDER", "level": 10}], "foe": [{"species": "PIDGEY", "level": 8}], "n": 5})
        if eng is None:
            self.assertEqual(s, 503)
            self.assertEqual(o, {"error": "engine not built", "hint": "make balance-lib"})
            s, o = self.post("/api/run", {})
            self.assertEqual(s, 503)
        else:
            self.assertIn(s, (200, 500, 503))
        self.assertEqual(self.post("/api/nope", {})[0], 404)
        self.assertEqual(self.get("/api/run/zzz/progress")[0], 404)

    def test_run_job_with_fake_report(self):
        import types
        fake = types.ModuleType("minasim.report")

        def run_report(run_id, options, progress):
            progress(0, 4, "init")
            for i in range(4):
                time.sleep(0.02)
                progress(i + 1, 4, "battles")
        fake.run_report = run_report
        fake_eng = types.ModuleType("minasim.engine")
        fake_eng.Engine = object
        saved = {k: sys.modules.get(k) for k in ("minasim.report", "minasim.engine")}
        sys.modules["minasim.report"], sys.modules["minasim.engine"] = fake, fake_eng
        try:
            s, o = self.post("/api/run", {"battles": 10})
            self.assertEqual(s, 200)
            rid = o["run_id"]
            for _ in range(100):
                s, p = json.loads(self.get("/api/run/%s/progress" % rid)[2]) and (200, json.loads(self.get("/api/run/%s/progress" % rid)[2]))
                if p["state"] != "running":
                    break
                time.sleep(0.02)
            self.assertEqual(p["state"], "done")
            self.assertEqual((p["done"], p["total"]), (4, 4))
        finally:
            for k, v in saved.items():
                if v is None:
                    sys.modules.pop(k, None)
                else:
                    sys.modules[k] = v

    def test_sprites(self):
        if not sprites.available():
            self.skipTest("no PIL")
        OUT.mkdir(parents=True, exist_ok=True)
        cases = [("pokemon/BULBASAUR/front", (64, 64)), ("pokemon/bulbasaur/back", (64, 64)), ("pokemon/charmander/icon", (32, 32)),
                 ("pokemon/SPECIES_PIKACHU/front", (64, 64)), ("item/POTION", (24, 24)), ("item/POKE_BALL", (24, 24)),
                 ("trainer/leader_roxanne", (64, 64)), ("trainer/hiker", (64, 64)), ("type/FIRE", (32, 16)), ("type/water", (32, 16))]
        for p, size in cases:
            s, h, b = self.get("/sprites/%s.png" % p)
            self.assertEqual(s, 200, p)
            w, hh, depth, ctype = png_info(b)
            self.assertEqual((w, hh), size, p)
            self.assertEqual(ctype, 6, "RGBA expected")
            (OUT / (p.replace("/", "_") + ".png")).write_bytes(b)
        s, h, b = self.get("/sprites/pokemon/bulbasaur/front.png?scale=3")
        self.assertEqual(png_info(b)[:2], (192, 192))
        (OUT / "bulbasaur_front_x3.png").write_bytes(b)
        from PIL import Image
        import io
        im = Image.open(io.BytesIO(self.get("/sprites/pokemon/bulbasaur/front.png")[2]))
        self.assertEqual(im.getpixel((0, 0))[3], 0, "index 0 must be transparent")
        self.assertEqual(self.get("/sprites/pokemon/nope/front.png")[0], 404)
        self.assertEqual(self.get("/sprites/item/NOPE.png")[0], 404)
        s, h, b = self.get("/sprites/manifest.json")
        m = json.loads(b)
        self.assertEqual(len(m["pokemon"]), 151)
        self.assertIn("POTION", m["item"])

    def test_sprite_coverage(self):
        if not sprites.available():
            self.skipTest("no PIL")
        d = json.loads(self.get("/api/data")[2])
        ok = {"pokemon": 0, "item": 0, "trainer": 0}
        bad = []
        for s in d["species"]:
            for v in ("front", "back", "icon"):
                if self.get("/sprites/pokemon/%s/%s.png" % (s["id"], v))[0] == 200:
                    ok["pokemon"] += 1
                else:
                    bad.append((s["id"], v))
        for i in d["items"]:
            if self.get("/sprites/item/%s.png" % i["id"])[0] == 200:
                ok["item"] += 1
            else:
                bad.append(i["id"])
        for t in d["trainers"]:
            if self.get("/sprites/trainer/%s.png" % t["pic_slug"])[0] == 200:
                ok["trainer"] += 1
            else:
                bad.append(t["id"])
        print("\nsprite coverage:", ok, "of", 3 * len(d["species"]), len(d["items"]), len(d["trainers"]), "bad:", bad[:10])
        self.assertEqual(ok["pokemon"], 3 * len(d["species"]))


if __name__ == "__main__":
    unittest.main()
