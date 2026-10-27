"""The editor's web server: stdlib http.server on localhost, JSON API plus static pages.

    python3 tools/editor/run.py [--port 8766] [--out DIR] [--in-place] [--no-browser]

Reads the repo; saves go to --out (default build/editor/out) unless --in-place. The pages reuse the Balance Lab's
stylesheet (tools/balance_sim/minasim/gui/static/css) so both tools look the same.
"""
import argparse
import difflib
import hashlib
import json
import os
import re
import subprocess
import sys
import threading
import traceback
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import unquote, urlparse

from . import paths, validate
from .arena import ArenaKnobs
from .encounters import Encounters
from .species import SpeciesStore
from .trainers import Party, const, make_head, parse_head, words

STATIC = os.path.join(paths.EDITOR_DIR, "static")
LAB_CSS = os.path.join(paths.SIM_ROOT, "minasim", "gui", "static", "css")
MAX_BODY = 4 * 1024 * 1024
SIM_LOCK = threading.Lock()
SIM_TIMEOUT = 420
NAME_RE = re.compile(r"^[A-Za-z0-9_]{1,40}$")


class ApiError(Exception):
    def __init__(self, code, msg, **extra):
        super().__init__(msg)
        self.code, self.extra = code, extra


def define_names(rel, prefix, lo=None, hi=None):
    out = []
    with open(os.path.join(paths.REPO, rel), encoding="utf-8") as f:
        for m in re.finditer(r"^#define\s+%s(\w+)\s+(\d+)\s*(?://.*)?$" % prefix, f.read(), re.M):
            if lo is None or lo <= int(m.group(2)) <= hi:
                out.append((int(m.group(2)), m.group(1)))
    return [n for _i, n in sorted(out)]


class App:
    def __init__(self, store):
        self.store = store
        self._lists = None

    # ------------------------------------------------------------ shared lists
    def lists(self):
        if self._lists is None:
            ss = SpeciesStore(self.store)
            self._lists = {
                "species": [s["species"] for s in ss.summary()],
                "moves": define_names("include/constants/moves.h", "MOVE_", 1, 165),
                "items": define_names("include/constants/items.h", "ITEM_", 1, 10000),
                "tmhm": self._tmhm(),
                "types": ["NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "FIRE", "WATER",
                          "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON"],
                "evoMethods": ["LEVEL", "ITEM", "TRADE", "TRADE_ITEM", "FRIENDSHIP", "FRIENDSHIP_DAY", "FRIENDSHIP_NIGHT"],
                "classes": sorted({m for m in re.findall(r"^Class:\s*(.*)$", self.store.read(paths.TRAINERS), re.M)}),
                "maps": sorted({e["map"] for e in Encounters(self.store).flat()}),
            }
        return self._lists

    def _tmhm(self):
        text = open(os.path.join(paths.REPO, "include/constants/tms_hms.h"), encoding="utf-8").read()
        out = []
        for macro in ("FOREACH_TM", "FOREACH_HM"):
            m = re.search(r"#define %s\(F\)(.*?)\n\n" % macro, text, re.S)
            out += re.findall(r"F\((\w+)\)", m.group(1) if m else "")
        return out

    # ------------------------------------------------------------ status / pending
    def status(self):
        sim = os.path.exists(os.path.join(paths.REPO, "build", "host", "libminasim.so"))
        return {"repo": paths.REPO, "out": self.store.out, "inPlace": self.store.in_place, "simReady": sim,
                "pending": [p["file"] for p in self.pending()]}

    def pending(self):
        """Files saved in `out` that differ from the repo."""
        if self.store.in_place:
            return []
        out = []
        for dirpath, _d, files in os.walk(self.store.out):
            for fn in files:
                rel = os.path.relpath(os.path.join(dirpath, fn), self.store.out)
                try:
                    new = open(os.path.join(self.store.out, rel), encoding="utf-8", newline="").read()
                    old = self.store.read_repo(rel)
                except OSError:
                    continue
                if new != old:
                    diff = "".join(difflib.unified_diff(old.splitlines(True), new.splitlines(True), "repo/" + rel, "saved/" + rel, n=2))
                    out.append({"file": rel, "diff": diff[:60000]})
        return sorted(out, key=lambda p: p["file"])

    def discard(self, rel):
        p = os.path.join(self.store.out, rel)
        if self.store.in_place or rel not in {x["file"] for x in self.pending()}:
            raise ApiError(400, "nothing to discard")
        os.unlink(p)

    # ------------------------------------------------------------ species
    def species_list(self):
        return SpeciesStore(self.store).summary()

    def species_get(self, name):
        ss = SpeciesStore(self.store)
        rec = self._species_rec(ss, name)
        return {"species": name, "name": rec.get("name"), "types": rec.get("types"), "growthRate": rec.get("growthRate"),
                "baseStats": rec.get("baseStats"),
                **{k: rec.get(k) for k in ("levelUpMoves", "evolutions", "tmhm", "tutor", "eggMoves")},
                "errors": validate.validate_species(name, rec)}

    def _species_rec(self, ss, name):
        if name not in ss.names() and name.lower() not in ss.names():
            raise ApiError(404, "unknown species %s" % name)
        return ss.load(name.lower())

    def species_validate(self, name, body):
        ss = SpeciesStore(self.store)
        rec = ss.apply_edit(self._species_rec(ss, name), body.get("edit") or {})
        return {"errors": validate.validate_species(name, rec)}

    def species_save(self, name, body):
        ss = SpeciesStore(self.store)
        rec = ss.apply_edit(self._species_rec(ss, name), body.get("edit") or {})
        errs = validate.validate_species(name, rec)
        if any(e["new"] for e in errs) and not body.get("force"):
            return {"saved": False, "errors": errs}
        return {"saved": True, "path": ss.save(name.lower(), rec), "errors": errs}

    def species_sim(self, name, body):
        ss = SpeciesStore(self.store)
        rec = ss.apply_edit(self._species_rec(ss, name), body.get("edit") or {})
        return self.simulate({"mode": "species", "records": {name: rec}, "runs": int(body.get("runs", 40))})

    # ------------------------------------------------------------ trainers
    def party(self):
        return Party(self.store.read(paths.TRAINERS))

    def trainers_list(self):
        p = self.party()
        out = []
        for t in p.trainers():
            lv = [int(m.field("Level", 100)) if str(m.field("Level", "100")).isdigit() else 100 for m in t.mons]
            out.append({"id": t.id, "owner": t.owner, "name": t.key("Name", ""), "class": t.key("Class", ""),
                        "mons": len(t.mons), "levels": [min(lv), max(lv)] if lv else None,
                        "species": [parse_head(m.head)["species"] for m in t.mons]})
        return {"trainers": out, "blocks": p.blocks()}

    def trainers_get(self, tid):
        t = self.party().get(tid)
        if t is None:
            raise ApiError(404, "unknown trainer %s" % tid)
        return t.to_json()

    def _edited(self, tid, body):
        p = self.party()
        t = p.get(tid)
        if t is None:
            raise ApiError(404, "unknown trainer %s" % tid)
        if "trainer" in body:
            t.set_from_json(body["trainer"])
        return p, t

    def trainers_validate(self, tid, body):
        p, _t = self._edited(tid, body)
        return {"errors": validate.validate_trainers(p.dumps())}

    def trainers_save(self, tid, body):
        p, _t = self._edited(tid, body)
        text = p.dumps()
        errs = validate.validate_trainers(text)
        if any(e["new"] for e in errs) and not body.get("force"):
            return {"saved": False, "errors": errs}
        return {"saved": True, "path": self.store.write(paths.TRAINERS, text), "errors": errs}

    def trainers_sim(self, tid, body):
        _p, t = self._edited(tid, body)
        mons = []
        for m in t.mons:
            h = parse_head(m.head)
            mons.append({"species": h["species"], "level": m.field("Level", "100"), "item": h["item"],
                         "moves": [const(x, "MOVE") for x in m.moves]})
        req = {"mode": "trainer", "n": int(body.get("n", 40)), "runs": int(body.get("runs", 40)),
               "trainer": {"id": tid, "name": t.key("Name", ""), "class": t.key("Class", ""), "mons": mons}}
        return self.simulate(req)

    # ------------------------------------------------------------ encounters
    def enc_list(self):
        e = Encounters(self.store)
        return {"entries": e.summary(), "rates": {k: e.rates(k) for k in ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons")}}

    def enc_get(self, i):
        e = Encounters(self.store)
        try:
            d = e.get(i)
        except IndexError:
            raise ApiError(404, "no encounter entry %d" % i)
        d["errors"] = validate.validate_encounters(e.render())
        return d

    def _enc_edited(self, i, body):
        e = Encounters(self.store)
        try:
            e.flat()[i]
        except IndexError:
            raise ApiError(404, "no encounter entry %d" % i)
        if "entry" in body:
            e.replace(i, body["entry"])
        return e

    def enc_validate(self, i, body):
        return {"errors": validate.validate_encounters(self._enc_edited(i, body).render())}

    def enc_save(self, i, body):
        e = self._enc_edited(i, body)
        errs = validate.validate_encounters(e.render())
        if any(x["new"] for x in errs) and not body.get("force"):
            return {"saved": False, "errors": errs}
        return {"saved": True, "path": e.save(), "errors": errs}

    def enc_sim(self, i, body):
        e = self._enc_edited(i, body)
        entry = e.flat()[i]
        kind = body.get("kind", "land_mons")
        if kind not in entry:
            raise ApiError(400, "this map has no %s" % kind)
        rates = e.rates(kind)
        mons = entry[kind]["mons"]
        slots = [{"species": m["species"].replace("SPECIES_", ""), "min": m["min_level"], "max": m["max_level"],
                  "weight": rates[j] if j < len(rates) else 1} for j, m in enumerate(mons)]
        return self.simulate({"mode": "route", "slots": slots, "n": int(body.get("n", 60)), "runs": int(body.get("runs", 30)),
                              "battles": int(body.get("battles", 12))})

    # ------------------------------------------------------------ arena
    def arena_get(self):
        a = ArenaKnobs(self.store)
        return {"knobs": a.to_json(), "errors": validate.validate_arena(a.values())}

    def _arena_edited(self, body):
        a = ArenaKnobs(self.store)
        try:
            a.set_values(body.get("values") or {})
        except (KeyError, ValueError, TypeError) as e:
            raise ApiError(400, str(e))
        return a

    def arena_validate(self, body):
        return {"errors": validate.validate_arena(self._arena_edited(body).values())}

    def arena_save(self, body):
        a = self._arena_edited(body)
        errs = validate.validate_arena(a.values())
        if errs and not body.get("force"):
            return {"saved": False, "errors": errs}
        return {"saved": True, "path": a.save(), "errors": errs}

    def arena_sim(self, body):
        a = self._arena_edited(body)
        knobs = {k["name"]: k["value"] for k in a.to_json() if k["value"] != k["default"]}
        return self.simulate({"mode": "arena", "knobs": knobs, "runs": int(body.get("runs", 60))})

    # ------------------------------------------------------------ simulation
    def simulate(self, req):
        if not os.path.exists(os.path.join(paths.REPO, "build", "host", "libminasim.so")):
            raise ApiError(501, "the Balance Lab engine is not built", hint="make balance-lib")
        env = dict(os.environ, PYTHONPATH=paths.EDITOR_DIR, MEDIT_REPO=paths.REPO)
        if not SIM_LOCK.acquire(timeout=5):
            raise ApiError(429, "another simulation is running")
        try:
            r = subprocess.run([sys.executable, "-m", "medit.simrun"], input=json.dumps(req), capture_output=True, text=True,
                               timeout=SIM_TIMEOUT, env=env, cwd=paths.EDITOR_DIR)
        except subprocess.TimeoutExpired:
            raise ApiError(504, "the simulation took more than %d s" % SIM_TIMEOUT)
        finally:
            SIM_LOCK.release()
        try:
            return json.loads(r.stdout)
        except ValueError:
            raise ApiError(500, "simulation failed", stderr=r.stderr[-1500:])


class Handler(BaseHTTPRequestHandler):
    app = None
    server_version = "medit"

    def log_message(self, fmt, *a):
        if os.environ.get("MEDIT_LOG"):
            sys.stderr.write("%s\n" % (fmt % a))

    def _send(self, code, body, ctype="application/json", extra=None):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode()
        elif isinstance(body, str):
            body = body.encode()
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-cache")
        for k, v in (extra or {}).items():
            self.send_header(k, v)
        self.end_headers()
        self.wfile.write(body)

    def _err(self, code, msg, **extra):
        self._send(code, dict({"error": msg}, **extra))

    def _handle(self, method):
        try:
            u = urlparse(self.path)
            path = unquote(u.path)
            if path.startswith("/api/"):
                body = {}
                if method == "POST":
                    n = int(self.headers.get("Content-Length") or 0)
                    if n > MAX_BODY:
                        return self._err(413, "body too large")
                    body = json.loads(self.rfile.read(n) or b"{}")
                    if not isinstance(body, dict):
                        return self._err(400, "body must be a JSON object")
                return self._send(200, self.route(method, path, body))
            if method != "GET":
                return self._err(405, "GET only")
            if path.startswith("/sprites/"):
                return self._sprite(path)
            return self._static(path)
        except ApiError as e:
            self._err(e.code, str(e), **e.extra)
        except ValueError as e:
            self._err(400, str(e))
        except Exception as e:
            self._err(500, "%s: %s" % (type(e).__name__, e), trace=traceback.format_exc(limit=5))

    do_GET = lambda self: self._handle("GET")      # noqa: E731
    do_POST = lambda self: self._handle("POST")    # noqa: E731

    def route(self, method, path, body):
        a = self.app
        parts = [p for p in path.split("/") if p][1:]
        if method == "GET":
            if parts == ["status"]:
                return a.status()
            if parts == ["lists"]:
                return a.lists()
            if parts == ["pending"]:
                return a.pending()
            if parts == ["species"]:
                return a.species_list()
            if len(parts) == 2 and parts[0] == "species":
                return a.species_get(self._name(parts[1]))
            if parts == ["trainers"]:
                return a.trainers_list()
            if len(parts) == 2 and parts[0] == "trainers":
                return a.trainers_get(self._name(parts[1]))
            if parts == ["encounters"]:
                return a.enc_list()
            if len(parts) == 2 and parts[0] == "encounters":
                return a.enc_get(self._int(parts[1]))
            if parts == ["arena"]:
                return a.arena_get()
        else:
            if parts == ["discard"]:
                a.discard(str(body.get("file", "")))
                return {"ok": True}
            if len(parts) == 3 and parts[2] in ("validate", "save", "simulate"):
                kind, ident, act = parts
                fn = {"species": ("species", self._name), "trainers": ("trainers", self._name), "encounters": ("enc", self._int)}.get(kind)
                if fn:
                    meth = {"validate": "validate", "save": "save", "simulate": "sim"}[act]
                    return getattr(a, "%s_%s" % (fn[0], meth))(fn[1](ident), body)
            if len(parts) == 2 and parts[0] == "arena" and parts[1] in ("validate", "save", "simulate"):
                return getattr(a, "arena_" + {"validate": "validate", "save": "save", "simulate": "sim"}[parts[1]])(body)
        raise ApiError(404, "no such endpoint")

    @staticmethod
    def _name(s):
        if not NAME_RE.match(s):
            raise ApiError(400, "bad name")
        return s

    @staticmethod
    def _int(s):
        if not s.isdigit():
            raise ApiError(400, "bad index")
        return int(s)

    def _sprite(self, path):
        m = re.match(r"^/sprites/pokemon/([A-Za-z0-9_]+)/(icon|front)\.png$", path)
        if not m:
            return self._err(404, "bad sprite path")
        try:
            sys.path.insert(0, paths.SIM_ROOT)
            from minasim.gui import sprites
            data = sprites.render_png("pokemon", m.group(1), m.group(2), 1, False, paths.REPO, use_cache=False)
        except Exception as e:
            return self._err(404, "no sprite: %s" % e)
        self._send(200, data, "image/png", {"ETag": '"%s"' % hashlib.md5(data).hexdigest()[:16]})

    def _static(self, path):
        rel = path.lstrip("/") or "index.html"
        for root, prefix in ((LAB_CSS, "lab/css/"), (STATIC, "")):
            if prefix and not rel.startswith(prefix):
                continue
            sub = rel[len(prefix):]
            base = os.path.realpath(root)
            p = os.path.realpath(os.path.join(base, sub))
            if (p == base or p.startswith(base + os.sep)) and os.path.isfile(p):
                ctype = {".html": "text/html; charset=utf-8", ".js": "text/javascript; charset=utf-8", ".css": "text/css; charset=utf-8",
                         ".svg": "image/svg+xml"}.get(os.path.splitext(p)[1], "application/octet-stream")
                with open(p, "rb") as f:
                    return self._send(200, f.read(), ctype)
        self._err(404, "not found")


def make_server(store, port=8766):
    Handler.app = App(store)
    return ThreadingHTTPServer(("127.0.0.1", port), Handler)


def main(argv=None):
    ap = argparse.ArgumentParser(prog="medit", description="Minamoto data editor")
    ap.add_argument("--port", type=int, default=8766)
    ap.add_argument("--out", help="where saves go (default build/editor/out under the repo)")
    ap.add_argument("--in-place", action="store_true", help="write the repo files themselves (off by default)")
    ap.add_argument("--no-browser", action="store_true")
    a = ap.parse_args(argv)
    store = paths.Store(out=a.out, in_place=a.in_place)
    srv = make_server(store, a.port)
    url = "http://127.0.0.1:%d/" % srv.server_address[1]
    print("Minamoto editor on %s  (reads %s, saves to %s)" % (url, store.root, store.out))
    if not a.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    return 0
