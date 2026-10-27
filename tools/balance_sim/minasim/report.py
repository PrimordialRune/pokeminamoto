"""Orchestrates the signals and writes build/balance/runs/<run_id>/report.json (CONTRACT.md).

run_report(run_id=None, options=None, progress=None) is what the GUI server calls for POST /api/run;
options: quick (bool), jobs (int), baseline (run id), bands (list), signals (list of names), scale (float).
"""
import copy
import datetime
import json
import os
import subprocess
import time

from . import policies
from .fights import Catalog, STARTERS
from .gamedata import Game, ROOT
from .stages import Stages
from .teams import TeamBuilder

RUNS = os.environ.get("MINASIM_RUNS") or os.path.join(ROOT, "build", "balance", "runs")
ORDER = ["difficulty", "playthrough", "species", "moves", "items", "special_talk", "dominance"]
LABEL = {"difficulty": "fights", "playthrough": "playthroughs", "species": "species", "moves": "moves",
         "items": "items", "special_talk": "special_talk", "dominance": "dominance"}


class Ctx:
    def __init__(self, scale=1.0, jobs=None, bands=("average", "skilled"), progress=None, min_cell=12, min_runs=6):
        self.game = Game.get()
        self.stages = Stages(self.game)
        self.catalog = Catalog(self.game, self.stages)
        self.targets = policies.load_targets()
        self.teams = TeamBuilder(self.catalog, self.targets)
        self.scale = scale
        self.jobs = jobs or os.cpu_count() or 1
        self.bands = list(bands)
        self.min_cell, self.min_runs = min_cell, min_runs
        self.fight_by_id = {f["id"]: f for f in self.catalog.fights}
        self._cb = progress
        self._phase, self._nphase = "", 1
        self._pi = 0
        self.fight_rows = {}
        self.engine_hp = _engine_has_hp()
        self.engine_exp = _engine_has_exp()

    def target_for(self, fight, band):
        cls = fight["class"]
        if cls == "gym" and fight["source"] == "design" and fight.get("gym_no") == 1 and False:
            cls = "boss"
        return list(self.targets["bands"][cls][band])

    def progress(self, phase, done, total):
        if self._cb:
            frac = (self._pi + (done / total if total else 1)) / max(1, self._nphase)
            self._cb(int(frac * 1000), 1000, phase)


def _engine_has_hp():
    from .signals import _common as C
    try:
        import dataclasses
        return "hp" in {f.name for f in dataclasses.fields(C.mon_cls())}
    except TypeError:
        import inspect
        try:
            return "hp" in inspect.signature(C.mon_cls().__init__).parameters
        except (TypeError, ValueError):
            return False


def _engine_has_exp():
    from .signals import _common as C
    try:
        return "exp" in C.mon_cls().__slots__
    except Exception:
        return False


def _git(*a):
    try:
        return subprocess.check_output(["git", "-C", ROOT] + list(a), text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return ""


def run_report(run_id=None, options=None, progress=None):
    options = dict(options or {})
    t0 = time.time()
    quick = bool(options.get("quick"))
    sm = policies.load_targets()["sample"]
    scale = float(options.get("scale") or (sm["quick_scale"] if quick else 1.0))
    bands = options.get("bands") or ["average", "skilled"]
    names = options.get("signals") or ORDER
    ctx = Ctx(scale=scale, jobs=options.get("jobs"), bands=bands, progress=progress,
              min_cell=12 if quick else 30, min_runs=6 if quick else 20)
    from .signals import _common as C
    from importlib import import_module
    _, _, engine_kind = C.load_engine()
    commit = _git("rev-parse", "--short=8", "HEAD") or "unknown"
    created = datetime.datetime.now().replace(microsecond=0)
    run_id = run_id or "%s-%s" % (created.strftime("%Y-%m-%dT%H%M"), commit)
    out_dir = os.path.join(RUNS, run_id)
    os.makedirs(out_dir, exist_ok=True)
    ctx._nphase = len(names)
    sections, flags, battles, notes = {}, [], 0, []
    ptimes = {}
    for i, name in enumerate(names):
        ctx._pi = i
        ctx._phase = LABEL.get(name, name)
        t1 = time.time()
        sec = import_module("minasim.signals." + name).run(ctx)
        ptimes[name] = round(time.time() - t1, 1)
        sections.update(sec.data)
        flags += sec.flags
        battles += sec.battles
        notes += sec.notes
    if "difficulty" in names:
        ctx._pi = len(names) - 1
        whatif(ctx, flags)
    annotate_sources(ctx, flags)
    base = options.get("baseline")
    base_rep = load_report(base) if base else None
    flags = rank_flags(flags, base_rep, base, ctx.targets["flags"]["max_flags_listed"])
    st_pub = ctx.stages.public()
    rep = {
        "meta": {"run_id": run_id, "commit": commit, "branch": _git("rev-parse", "--abbrev-ref", "HEAD") or "unknown",
                 "created": created.isoformat(), "battles": battles, "seconds": round(time.time() - t0, 1), "jobs": ctx.jobs,
                 "bands": bands, "baseline_run": base if base_rep else None, "quick": quick, "scale": scale,
                 "engine": engine_kind, "phase_seconds": ptimes, "fight_sources": ctx.catalog.stats(), "source_legend": {"rom": "src/data/trainers.party on a map", "design": "planned fight from early_game.yaml / design docs", "placeholder": "generated Gen-1 canon or route filler", "mixed": "rests on several"}, "notes": notes,
                 "engine_hp_carry": ctx.engine_hp},
        "stages": st_pub,
        "flags": flags,
        "species": sections.get("species", []), "moves": sections.get("moves", []), "fights": sections.get("fights", []),
        "items": sections.get("items", []), "tp": sections.get("tp", []), "level_curve": sections.get("level_curve", []),
        "talk": sections.get("talk", []), "special": sections.get("special", []), "talk_summary": sections.get("talk_summary", {}),
        "dominance": {k: sections[k] for k in ("move_usage", "dead_species", "starter_win") if k in sections},
        "playthrough": sections.get("playthrough", {}),
        "lint": [list(x) for x in dict.fromkeys(ctx.stages.lint + ctx.catalog.lint)][:300],
    }
    with open(os.path.join(out_dir, "report.json"), "w", encoding="utf-8") as f:
        json.dump(rep, f, indent=1)
    update_index(rep)
    if progress:
        progress(1000, 1000, "done")
    return rep


def annotate_sources(ctx, flags):
    """flags[].source = what the flag rests on: rom | design | placeholder | mixed. Placeholder-only flags stay info."""
    by_stage = {}
    for f in ctx.catalog.fights:
        by_stage.setdefault(f["stage_id"], set()).add(f["source"])
    for fl in flags:
        src = None
        if fl["kind"] == "fight":
            fid = fl["id"][len("fight:"):].rsplit(":", 1)[0]
            src = ctx.fight_by_id.get(fid, {}).get("source")
        elif fl["kind"] in ("tp", "level_curve", "dead_content", "dominance"):
            src = "mixed"
        elif fl.get("stage"):
            have = by_stage.get(fl["stage"], set())
            src = "rom" if "rom" in have and have == {"rom"} else "design" if have - {"placeholder"} else "placeholder"
        fl["source"] = src or "mixed"
        if fl["source"] == "placeholder" and fl["severity"] != "info":
            fl["severity"], fl["score"] = "info", round(fl["score"] * 0.2, 3)


def load_report(run_id):
    p = os.path.join(RUNS, run_id, "report.json")
    if os.path.exists(p):
        with open(p, encoding="utf-8") as f:
            return json.load(f)
    return None


def update_index(rep):
    p = os.path.join(RUNS, "index.json")
    try:
        with open(p, encoding="utf-8") as fh:
            idx = json.load(fh)
    except Exception:
        idx = []
    m = rep["meta"]
    idx = [r for r in idx if r["run_id"] != m["run_id"]]
    idx.insert(0, {"run_id": m["run_id"], "created": m["created"], "commit": m["commit"], "battles": m["battles"],
                   "flags_red": sum(f["severity"] == "red" for f in rep["flags"] if f["change"]["status"] != "resolved"),
                   "flags_amber": sum(f["severity"] == "amber" for f in rep["flags"] if f["change"]["status"] != "resolved")})
    os.makedirs(RUNS, exist_ok=True)
    with open(p, "w", encoding="utf-8") as f:
        json.dump(idx, f, indent=1)


def _dist(metric):
    if not metric or metric.get("target") is None or metric.get("value") is None:
        return None
    lo, hi = metric["target"]
    v = metric["value"]
    return max(0.0, lo - v, v - hi)


def rank_flags(flags, base_rep, base_id, cap):
    seen = {}
    for f in flags:
        seen[f["id"]] = f
    flags = list(seen.values())
    bmap = {f["id"]: f for f in (base_rep or {}).get("flags", [])}
    for f in flags:
        if base_rep is None:
            f["change"] = {"vs": None, "delta": None, "status": "new"}
            continue
        old = bmap.get(f["id"])
        if not old:
            f["change"] = {"vs": base_id, "delta": None, "status": "new"}
            continue
        d_new, d_old = _dist(f["metric"]), _dist(old["metric"])
        delta = None
        if f["metric"] and old.get("metric") and f["metric"].get("value") is not None and old["metric"].get("value") is not None:
            delta = round(f["metric"]["value"] - old["metric"]["value"], 4)
        st = "same"
        if d_new is not None and d_old is not None:
            st = "worse" if d_new > d_old + 0.02 else "better" if d_new < d_old - 0.02 else "same"
        f["change"] = {"vs": base_id, "delta": delta, "status": st}
    if base_rep:
        now = {f["id"] for f in flags}
        for old in base_rep.get("flags", []):
            if old["id"] not in now and old["severity"] in ("red", "amber"):
                r = copy.deepcopy(old)
                r["score"] = 0.0
                r["change"] = {"vs": base_id, "delta": None, "status": "resolved"}
                flags.append(r)
    flags.sort(key=lambda f: -f["score"])
    return flags[:cap]


def whatif(ctx, flags):
    """For the worst fight flags, re-run with the foe ace +-2 levels and say what lands in the target band."""
    import random
    from .signals import _common as C
    from .signals.difficulty import fight_job
    cands = [f for f in flags if f["kind"] == "fight" and f["id"].count(":") >= 2 and f["id"].split(":")[-1] in ctx.bands
             and f["severity"] in ("red", "amber") and f["metric"]["name"] == "win_rate"]
    cands.sort(key=lambda f: -f["score"])
    cands = cands[:6]
    jobs, keys = [], []
    for f in cands:
        fid = f["id"][len("fight:"):].rsplit(":", 1)[0]
        base = ctx.fight_by_id[fid]
        band = f["band"]
        hard = f["metric"]["value"] < f["metric"]["target"][0]
        for delta in ((-1, -2, -3) if hard else (1, 2, 3)):
            v = copy.deepcopy(base)
            for m in v["party"]:
                if m["species"] != "RIVAL_STARTER" or True:
                    m["level"] = max(2, m["level"] + delta)
            v["id"] = "%s~%+d" % (fid, delta)
            ctx.fight_by_id[v["id"]] = v
            for st in STARTERS:
                jobs.append((v["id"], band, st, list(range(max(20, int(60 * max(ctx.scale, 0.25)))))))
            keys.append((f, fid, delta, v["id"]))
    if not jobs:
        return
    res = C.pmap(fight_job, jobs, ctx, "what-if")
    agg = {}
    for fid, band, st, rows in res:
        agg.setdefault((fid, band), []).extend(rows)
    for f in cands:
        fid = f["id"][len("fight:"):].rsplit(":", 1)[0]
        lo, hi = f["metric"]["target"]
        opts = []
        for (ff, fid2, delta, vid) in keys:
            if ff is f:
                rows = agg.get((vid, f["band"]), [])
                if rows:
                    opts.append((delta, sum(r["w"] == 0 for r in rows) / len(rows)))
        hit = [o for o in opts if lo <= o[1] <= hi]
        txt = "; ".join("foe levels %+d: %.0f%%" % (d, 100 * w) for d, w in sorted(opts, key=lambda o: abs(o[0])))
        best = min(hit, key=lambda o: abs(o[0])) if hit else None
        f["suggestion"] = ("Move every foe level by %+d (win rate %.0f%% in a what-if run; %s)" % (best[0], 100 * best[1], txt)
                           if best else "What-if runs (%s) do not reach the %.0f-%.0f%% band by levels alone; change moves or party" % (txt, 100 * lo, 100 * hi))
