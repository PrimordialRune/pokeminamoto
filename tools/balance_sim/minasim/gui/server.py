"""minasim GUI server: stdlib http.server, localhost only.

    python3 -m minasim gui            (analysis' __main__ calls gui.server.main)
    python3 -m minasim.gui.server --port 8765 --no-browser

Endpoints are listed in tools/balance_sim/CONTRACT.md; extras: GET /api/status,
GET /sprites/manifest.json, GET /api/run/<id>/progress, GET /api/runs/<id> alias of /api/report/<id>.

Hooks into the other teams' modules (all imports guarded, errors become JSON):
  minasim.gamedata.api_data(repo) -> dict   optional replacement for repodata.load()
  minasim.stages.api_stages()     -> list   optional replacement for the stage list in /api/data
  minasim.engine.Engine / minasim.runner.run_batch   for /api/duel
  minasim.report.run_report(run_id=, options=, progress=cb(done,total,phase))   for /api/run
"""
import argparse
import gzip
import hashlib
import json
import math
import mimetypes
import os
import re
import sys
import threading
import time
import traceback
import webbrowser
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, unquote, urlparse

from . import repodata, sprites

HERE = Path(__file__).resolve().parent
REPO = repodata.REPO
STATIC = HERE / "static"
FIXTURES = HERE / "fixtures"
BALANCE = REPO / "build" / "balance"
RUNS = BALANCE / "runs"

RUN_ID_RE = re.compile(r"^[A-Za-z0-9][A-Za-z0-9._-]{0,80}$")
MAX_BODY = 2 * 1024 * 1024
ENGINE_MISSING = {"error": "engine not built", "hint": "make balance-lib"}

PLACEHOLDER = """<!doctype html><html lang="en"><head><meta charset="utf-8"><title>minasim</title>
<meta name="viewport" content="width=device-width,initial-scale=1"></head>
<body style="font-family:system-ui;margin:2rem"><h1>minasim</h1>
<p>The front-end has not been installed yet (<code>minasim/gui/static/</code> is empty).
The API is live: <a href="/api/status">/api/status</a>, <a href="/api/runs">/api/runs</a>,
<a href="/api/data">/api/data</a>, <a href="/sprites/manifest.json">/sprites/manifest.json</a>.</p></body></html>
"""


# ---------------------------------------------------------------------------- helpers

def _import(name):
    try:
        import importlib
        return importlib.import_module(name)
    except Exception:
        return None


def _wilson(w, n, z=1.96):
    if n == 0:
        return [0.0, 1.0]
    p = w / n
    d = 1 + z * z / n
    c = p + z * z / (2 * n)
    a = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n))
    return [round(max(0.0, (c - a) / d), 4), round(min(1.0, (c + a) / d), 4)]


def _load_json(p):
    with open(p, encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------- runs

_RUN_ROW_CACHE = {}


def _scan_runs():
    """One row per run folder that has a report.json, newest first. Counts come from the report itself
    (cached by mtime), so a stale or half-written index.json can never list a run that is gone or
    show wrong numbers."""
    out = []
    if not RUNS.is_dir():
        return out
    for d in RUNS.iterdir():
        rp = d / "report.json"
        try:
            st = rp.stat()
        except OSError:
            continue
        key = (str(rp), st.st_mtime_ns, st.st_size)
        row = _RUN_ROW_CACHE.get(key)
        if row is None:
            try:
                r = _load_json(rp)
            except Exception:
                continue
            m = r.get("meta", {}) if isinstance(r, dict) else {}
            fl = r.get("flags", []) if isinstance(r, dict) else []
            row = {"run_id": d.name, "created": m.get("created"), "commit": m.get("commit"), "battles": m.get("battles"),
                   "flags_red": sum(f.get("severity") == "red" for f in fl),
                   "flags_amber": sum(f.get("severity") == "amber" for f in fl)}
            _RUN_ROW_CACHE[key] = row
        out.append(row)
    out.sort(key=lambda r: str(r.get("created") or ""), reverse=True)
    return out


def _fixture_runs():
    out = []
    for p in sorted(FIXTURES.glob("*.json")):
        try:
            r = _load_json(p)
        except Exception:
            continue
        if not isinstance(r, dict) or "meta" not in r:
            continue
        m = r["meta"]
        fl = r.get("flags", [])
        out.append({"run_id": m.get("run_id", p.stem), "created": m.get("created"), "commit": m.get("commit"),
                    "battles": m.get("battles"), "flags_red": sum(f.get("severity") == "red" for f in fl),
                    "flags_amber": sum(f.get("severity") == "amber" for f in fl), "demo": True,
                    "_file": p.name})
    return out


def list_runs():
    """-> (rows, demo). Demo mode when there are no real runs."""
    rows = _scan_runs()
    if rows:
        return rows, False
    return [{k: v for k, v in r.items() if k != "_file"} for r in _fixture_runs()], True


def get_report(run_id):
    if not RUN_ID_RE.match(run_id):
        return None
    p = RUNS / run_id / "report.json"
    if p.exists():
        return p
    for r in _fixture_runs():
        if r["run_id"] == run_id:
            return FIXTURES / r["_file"]
    return None


# ---------------------------------------------------------------------------- /api/data

class DataCache:
    def __init__(self):
        self.lock = threading.Lock()
        self.key = None
        self.body = None
        self.gz = None
        self.etag = None

    def _key(self):
        parts = []
        for f in repodata.source_files(REPO):
            try:
                st = f.stat()
                parts.append((str(f), st.st_mtime_ns, st.st_size))
            except OSError:
                pass
        return hash(tuple(parts))

    def get(self):
        with self.lock:
            k = self._key()
            if k != self.key or self.body is None:
                self.body = json.dumps(build_data(), separators=(",", ":")).encode()
                self.gz = gzip.compress(self.body, 6)
                self.etag = '"%s"' % hashlib.md5(self.body).hexdigest()[:16]
                self.key = k
            return self.body, self.gz, self.etag


def build_data():
    data = None
    source = "repodata"
    gd = _import("minasim.gamedata")
    if gd is not None and hasattr(gd, "api_data"):
        try:
            data = gd.api_data(REPO)
            source = "minasim.gamedata"
        except Exception:
            data = None
    if data is None:
        data = repodata.load(REPO)
    st = _import("minasim.stages")
    if st is not None and hasattr(st, "api_stages"):
        try:
            data["stages"] = st.api_stages()
        except Exception:
            pass
    data["source"] = source
    data["types"] = sorted({t for s in data.get("species", []) for t in s.get("types", [])})
    return data


DATA = DataCache()


# ---------------------------------------------------------------------------- duel

_engine_lock = threading.Lock()
_engine = {"obj": None}


def _engine_mods():
    eng = _import("minasim.engine")
    if eng is None or not hasattr(eng, "Engine"):
        return None, None
    return eng, _import("minasim.runner")


def do_duel(body):
    eng, runner = _engine_mods()
    if eng is None:
        return 503, ENGINE_MISSING
    try:
        player = body["player"]
        foe = body.get("foe")
        trainer = body.get("trainer")
        n = max(1, min(int(body.get("n", 200)), 20000))
        seed0 = int(body.get("seed", 1))
        pp = body.get("player_policy", "greedy")
        fp = body.get("foe_policy", "rom")
    except (KeyError, TypeError, ValueError) as e:
        return 400, {"error": "bad request: %s" % e}
    if not isinstance(player, list) or not player or (not foe and not trainer):
        return 400, {"error": "need player:[Mon] and foe:[Mon] or trainer"}

    def mon(d):
        keys = ("species", "level", "moves", "item", "ivs", "nature", "tp_spent")
        return eng.Mon(**{k: d[k] for k in keys if k in d})

    try:
        pl = [mon(m) for m in player]
        fo = [mon(m) for m in foe] if foe else None
        base = {"player": pl, "player_policy": pp, "foe_policy": fp}
        base.update({"foe": fo} if fo else {"trainer": trainer})
        specs = [dict(base, seed=seed0 + i, log=False) for i in range(n)]
        with _engine_lock:
            if runner is not None and hasattr(runner, "run_batch") and n > 1:
                results = runner.run_batch(specs)
            else:
                if _engine["obj"] is None:
                    _engine["obj"] = eng.Engine()
                results = [_engine["obj"].battle(**s) for s in specs]
            if _engine["obj"] is None:
                _engine["obj"] = eng.Engine()
            sample = _engine["obj"].battle(**dict(base, seed=seed0, log=True))
    except FileNotFoundError:
        return 503, ENGINE_MISSING
    except Exception as e:
        return 500, {"error": "engine failure: %s" % e, "trace": traceback.format_exc(limit=4)}
    wins = sum(1 for r in results if r["winner"] == 0)
    draws = sum(1 for r in results if r["winner"] == 2)
    turns = sum(r["turns"] for r in results) / len(results)
    return 200, {"wins": wins, "losses": len(results) - wins - draws, "draws": draws, "n": len(results),
                 "win_rate": wins / len(results), "ci": _wilson(wins, len(results)), "turns": round(turns, 2),
                 "sample_log": sample.get("log", []), "sample": {k: v for k, v in sample.items() if k != "log"}}


# ---------------------------------------------------------------------------- report runs

JOBS = {}
_jobs_lock = threading.Lock()


def start_run(opts):
    rep = _import("minasim.report")
    eng, _ = _engine_mods()
    if eng is None or rep is None or not hasattr(rep, "run_report"):
        return 503, ENGINE_MISSING if eng is None else {"error": "report generator not available",
                                                         "hint": "minasim.report.run_report is missing"}
    run_id = time.strftime("%Y-%m-%dT%H%M", time.localtime()) + "-" + hashlib.md5(os.urandom(8)).hexdigest()[:8]
    job = {"run_id": run_id, "done": 0, "total": 0, "phase": "starting", "state": "running", "error": None,
           "started": time.time()}
    with _jobs_lock:
        JOBS[run_id] = job

    def progress(done=None, total=None, phase=None):
        if done is not None:
            job["done"] = done
        if total is not None:
            job["total"] = total
        if phase is not None:
            job["phase"] = phase

    def work():
        try:
            rep.run_report(run_id=run_id, options=opts, progress=progress)
            job["state"] = "done"
            job["phase"] = "done"
            if job["total"]:
                job["done"] = job["total"]
        except Exception as e:
            job["state"] = "error"
            job["error"] = str(e)
            job["trace"] = traceback.format_exc(limit=6)

    threading.Thread(target=work, name="minasim-run-" + run_id, daemon=True).start()
    return 200, {"run_id": run_id}


def run_progress(run_id):
    job = JOBS.get(run_id)
    if job:
        return 200, {k: v for k, v in job.items() if k != "started"} | {"elapsed": round(time.time() - job["started"], 1)}
    pj = RUNS / run_id / "progress.json" if RUN_ID_RE.match(run_id) else None
    if pj and pj.exists():
        try:
            return 200, _load_json(pj)
        except Exception:
            pass
    if get_report(run_id):
        return 200, {"run_id": run_id, "done": 1, "total": 1, "phase": "done", "state": "done"}
    return 404, {"error": "unknown run"}


def status():
    eng, runner = _engine_mods()
    rep = _import("minasim.report")
    _, demo = list_runs()
    return {"demo": demo, "engine": eng is not None, "report_generator": bool(rep and hasattr(rep, "run_report")),
            "sprites": sprites.available(), "repo": str(REPO), "data_source": "gamedata" if
            (_import("minasim.gamedata") and hasattr(_import("minasim.gamedata"), "api_data")) else "repodata"}


# ---------------------------------------------------------------------------- HTTP

class Handler(BaseHTTPRequestHandler):
    server_version = "minasim"
    protocol_version = "HTTP/1.1"

    def log_message(self, fmt, *a):
        if os.environ.get("MINASIM_GUI_LOG"):
            sys.stderr.write("[gui] " + fmt % a + "\n")

    # -- output
    def _send(self, code, body, ctype="application/json", extra=None, gz=None):
        if isinstance(body, (dict, list)):
            body = json.dumps(body).encode()
        elif isinstance(body, str):
            body = body.encode()
        if gz is not None and "gzip" in self.headers.get("Accept-Encoding", ""):
            body = gz
            extra = dict(extra or {}, **{"Content-Encoding": "gzip"})
        self.send_response(code)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        for k, v in (extra or {}).items():
            self.send_header(k, v)
        self.end_headers()
        if self.command != "HEAD":
            self.wfile.write(body)

    def _json(self, code, obj, extra=None):
        self._send(code, obj, "application/json; charset=utf-8", dict({"Cache-Control": "no-store"}, **(extra or {})))

    def _err(self, code, msg, **kw):
        self._json(code, dict(error=msg, **kw))

    # -- routing
    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        try:
            u = urlparse(self.path)
            path = unquote(u.path)
            q = {k: v[0] for k, v in parse_qs(u.query).items()}
            if path == "/api/status":
                return self._json(200, status())
            if path == "/api/runs":
                rows, demo = list_runs()
                return self._json(200, rows, {"X-Minasim-Demo": "1" if demo else "0"})
            m = re.match(r"^/api/(?:report|runs)/([^/]+)$", path)
            if m:
                p = get_report(m.group(1))
                if not p:
                    return self._err(404, "unknown run")
                body = p.read_bytes()
                demo = FIXTURES in p.parents
                return self._send(200, body, "application/json; charset=utf-8",
                                  {"Cache-Control": "no-store", "X-Minasim-Demo": "1" if demo else "0"})
            if path == "/api/data":
                body, gz, etag = DATA.get()
                if self.headers.get("If-None-Match") == etag:
                    return self._send(304, b"", extra={"ETag": etag})
                return self._send(200, body, "application/json; charset=utf-8",
                                  {"ETag": etag, "Cache-Control": "no-cache"}, gz=gz)
            m = re.match(r"^/api/run/([^/]+)/progress$", path)
            if m:
                code, obj = run_progress(m.group(1))
                return self._json(code, obj)
            if path == "/sprites/manifest.json":
                return self._json(200, sprites.manifest(REPO))
            if path.startswith("/sprites/"):
                return self._sprite(path, q)
            if path.startswith("/api/"):
                return self._err(404, "no such endpoint")
            return self._static(path)
        except BrokenPipeError:
            pass
        except Exception as e:
            self._err(500, str(e), trace=traceback.format_exc(limit=4))

    def do_POST(self):
        try:
            path = urlparse(self.path).path
            n = int(self.headers.get("Content-Length") or 0)
            if n > MAX_BODY:
                return self._err(413, "body too large")
            raw = self.rfile.read(n) if n else b"{}"
            try:
                body = json.loads(raw or b"{}")
            except ValueError:
                return self._err(400, "invalid JSON")
            if not isinstance(body, dict):
                return self._err(400, "body must be a JSON object")
            if path == "/api/duel":
                code, obj = do_duel(body)
            elif path == "/api/run":
                code, obj = start_run(body)
            else:
                return self._err(404, "no such endpoint")
            self._json(code, obj)
        except BrokenPipeError:
            pass
        except Exception as e:
            self._err(500, str(e), trace=traceback.format_exc(limit=4))

    # -- sprites
    def _sprite(self, path, q):
        m = re.match(r"^/sprites/(pokemon|item|trainer|type)/([^/]+?)(?:/(front|back|icon))?\.png$", path)
        if not m:
            return self._err(404, "bad sprite path")
        kind, name, variant = m.groups()
        if kind == "pokemon" and not variant:
            variant = "front"
        if kind == "pokemon" and variant is None:
            return self._err(404, "bad sprite path")
        if not sprites.available():
            return self._err(501, "Pillow not installed", hint="pip install pillow")
        try:
            data = sprites.render_png(kind, name, variant, q.get("scale", 1), q.get("shiny") in ("1", "true"), REPO)
        except sprites.SpriteError as e:
            return self._err(e.status, str(e))
        etag = '"%s"' % hashlib.md5(data).hexdigest()[:16]
        if self.headers.get("If-None-Match") == etag:
            return self._send(304, b"", extra={"ETag": etag})
        self._send(200, data, "image/png", {"ETag": etag, "Cache-Control": "no-cache"})

    # -- static
    def _static(self, path):
        rel = path.lstrip("/") or "index.html"
        if rel.endswith("/"):
            rel += "index.html"
        root = STATIC.resolve()
        p = (root / rel).resolve()
        if root not in p.parents and p != root:
            return self._err(403, "forbidden")
        if p.is_dir():
            p = p / "index.html"
        if not p.exists():
            if rel == "index.html":
                return self._send(200, PLACEHOLDER, "text/html; charset=utf-8", {"Cache-Control": "no-store"})
            return self._err(404, "not found")
        ctype = mimetypes.guess_type(str(p))[0] or "application/octet-stream"
        if ctype.startswith("text/") or ctype in ("application/javascript", "application/json"):
            ctype += "; charset=utf-8"
        self._send(200, p.read_bytes(), ctype, {"Cache-Control": "no-cache"})


class Server(ThreadingHTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def make_server(port=8765):
    """Bind 127.0.0.1:port (0 = random); caller runs serve_forever()."""
    STATIC.mkdir(exist_ok=True)
    idx = STATIC / "index.html"
    if not idx.exists():
        idx.write_text(PLACEHOLDER, encoding="utf-8")
    return Server(("127.0.0.1", port), Handler)


def main(port=8765, open_browser=True):
    try:
        srv = make_server(port)
    except OSError as e:
        print("minasim gui: cannot bind 127.0.0.1:%s (%s)" % (port, e), file=sys.stderr)
        return 1
    url = "http://127.0.0.1:%d/" % srv.server_address[1]
    print("minasim gui at " + url + "  (Ctrl-C to stop)")
    _, demo = list_runs()
    if demo:
        print("no runs in build/balance/runs: serving demo fixtures")
    if open_browser:
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        srv.server_close()
    return 0


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8765)
    ap.add_argument("--no-browser", action="store_true")
    a = ap.parse_args()
    sys.exit(main(a.port, not a.no_browser))
