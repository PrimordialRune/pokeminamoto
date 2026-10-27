"""The HTTP API end to end on a temp save directory: edit, validate, save, diff, discard; the repo never changes."""
import hashlib
import json
import os
import tempfile
import threading
import unittest
import urllib.error
import urllib.request

import helpers
from medit import paths, server

REPO = helpers.REPO
WATCH = [paths.TRAINERS, paths.ENCOUNTERS, paths.ARENA, paths.SPECIES_DIR + "/pikachu.json"]


def digest():
    return {f: hashlib.sha256(open(os.path.join(REPO, f), "rb").read()).hexdigest() for f in WATCH}


class ServerTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.out = tempfile.TemporaryDirectory()
        cls.srv = server.make_server(paths.Store(out=cls.out.name), 0)
        cls.base = "http://127.0.0.1:%d/" % cls.srv.server_address[1]
        threading.Thread(target=cls.srv.serve_forever, daemon=True).start()
        cls.before = digest()

    @classmethod
    def tearDownClass(cls):
        cls.srv.shutdown()
        cls.srv.server_close()
        cls.out.cleanup()
        assert digest() == cls.before, "the repo files changed"

    def call(self, path, body=None, expect=200):
        data = None if body is None else json.dumps(body).encode()
        req = urllib.request.Request(self.base + path, data, {"Content-Type": "application/json"} if data else {})
        try:
            with urllib.request.urlopen(req) as r:
                self.assertEqual(r.status, expect)
                raw = r.read()
        except urllib.error.HTTPError as e:
            raw = e.read()
            self.assertEqual(e.code, expect, raw)
        try:
            return json.loads(raw)
        except ValueError:
            return raw

    def test_pages_and_style_are_served(self):
        self.assertIn(b"Minamoto Data Editor", self.call("", expect=200))
        self.assertIn(b"--coral", self.call("lab/css/minasim.css"))     # the Balance Lab's stylesheet, read-only
        self.call("lab/css/../../../../../../etc/passwd", expect=404)
        self.call("css/..%2f..%2fmedit/server.py", expect=404)

    def test_trainer_edit_validate_save_discard(self):
        t = self.call("api/trainers/TRAINER_YUJIROU")
        self.assertEqual(t["owner"], "tokiwa-spine")
        t["mons"][0]["fields"][0][1] = "9"
        self.assertEqual(self.call("api/trainers/TRAINER_YUJIROU/validate", {"trainer": t})["errors"], [])
        bad = json.loads(json.dumps(t))
        bad["mons"][0]["fields"][0][1] = "140"
        r = self.call("api/trainers/TRAINER_YUJIROU/save", {"trainer": bad})
        self.assertFalse(r["saved"])
        self.assertIn("level 140", r["errors"][0]["message"])
        r = self.call("api/trainers/TRAINER_YUJIROU/save", {"trainer": t})
        self.assertTrue(r["saved"])
        self.assertTrue(r["path"].startswith(self.out.name))
        pend = self.call("api/pending")
        self.assertEqual([p["file"] for p in pend], [paths.TRAINERS])
        self.assertIn("-Level: 2\n+Level: 9", pend[0]["diff"])
        self.assertEqual(self.call("api/trainers/TRAINER_YUJIROU")["mons"][0]["fields"][0][1], "9")   # reload shows the saved edit
        self.call("api/discard", {"file": "../../etc/passwd"}, expect=400)
        self.call("api/discard", {"file": paths.TRAINERS})
        self.assertEqual(self.call("api/pending"), [])

    def test_species_encounters_arena(self):
        s = self.call("api/species/PIKACHU")
        edit = {"levelUpMoves": s["levelUpMoves"], "evolutions": s["evolutions"], "tmhm": s["tmhm"]}
        edit["levelUpMoves"][-1] = [50, "NOT_A_MOVE"]
        errs = self.call("api/species/PIKACHU/validate", {"edit": edit})["errors"]
        self.assertTrue(any("unknown move NOT_A_MOVE" in e["message"] and e["new"] for e in errs))
        e = self.call("api/encounters/0")
        e["entry"]["land_mons"]["mons"][0]["max_level"] = 120
        self.assertTrue(any("levels 2..120" in x["message"] for x in self.call("api/encounters/0/validate", {"entry": e["entry"]})["errors"]))
        a = self.call("api/arena")
        self.assertEqual(len(a["knobs"]), 21)
        self.assertEqual(self.call("api/arena/validate", {"values": {"wetTurns": 70000}})["errors"][0]["check"], "arena")
        self.call("api/arena/validate", {"values": {"nope": 1}}, expect=400)
        r = self.call("api/arena/save", {"values": {"wetTurns": 6}})
        self.assertTrue(r["saved"])
        self.assertIn("wetTurns = 6", open(r["path"]).read())
        self.call("api/discard", {"file": paths.ARENA})

    def test_bad_requests(self):
        self.call("api/species/NOT_A_SPECIES", expect=404)
        self.call("api/trainers/TRAINER_NOPE", expect=404)
        self.call("api/encounters/9999", expect=404)
        self.call("api/species/..%2f..", expect=404)
        self.call("api/species/bad%20name", expect=400)
        self.call("api/nothing", expect=404)

    @unittest.skipUnless(os.path.exists(os.path.join(REPO, "build/host/libminasim.so")), "make balance-lib first")
    def test_simulate_trainer(self):
        t = self.call("api/trainers/TRAINER_YUJIROU")
        r = self.call("api/trainers/TRAINER_YUJIROU/simulate", {"trainer": t, "n": 6, "runs": 6})
        self.assertNotIn("error", r)
        self.assertEqual(len(r["duel"]), 6)
        for c in r["duel"].values():
            self.assertEqual(len(c["target"]), 2)
            self.assertIn(c["verdict"], ("in band", "too hard", "too easy"))
        self.assertEqual(r["chapter"]["fight"], "yujirou")


if __name__ == "__main__":
    unittest.main()
