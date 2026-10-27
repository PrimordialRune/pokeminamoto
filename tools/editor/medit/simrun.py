"""Runs minasim for the editor: `python3 -m medit.simrun < request.json > result.json`.

A separate process because the engine keeps the game's globals (one library per process) and because the editor
never imports tools/balance_sim into its own server. This file uses minasim as a library (Game, Catalog,
TeamBuilder, signals._common.battle, journey) and changes none of it: edits are applied to the in-process data
(`overlay`) of this one run, so nothing is written and nothing needs a rebuild.

Modes (request["mode"]):
  trainer  an edited party vs plausible player teams (every starter, both bands) + the same fight inside the
           Masara-to-Nibi journey when a step carries the trainer's name
  route    an edited wild table: win rate of its slots, plus walk-backs to an inn in a short journey
  species  the Masara-to-Nibi journey with and without the edited species records
  arena    the journey with and without the edited arena knobs
"""
import json
import os
import random
import statistics
import sys
import tempfile

from . import paths

SC = "scenarios/masara_nibi.yaml"
STARTERS = ["BULBASAUR", "CHARMANDER", "SQUIRTLE"]
BANDS = ["average", "skilled"]


def _setup():
    sim = paths.SIM_ROOT
    os.chdir(sim)
    if sim not in sys.path:
        sys.path.insert(0, sim)


def _overlay(g, recs):
    """Swap species records in the in-process game data (learnsets, evolutions, stats)."""
    if not recs:
        return
    allrecs = dict(g._records())
    allrecs.update(recs)
    g._recs = allrecs
    g.learnsets, g.evolutions, g.species = {}, {}, {}
    g._load_species()
    g._load_learnsets()
    g._load_evolutions()


def _party(g, mons, edited):
    out, problems = [], []
    for m in mons:
        sp = g.try_resolve(m["species"])
        if sp is None:
            problems.append("unknown species %r" % m["species"])
            continue
        moves = [x for x in (g.try_resolve_move(v) for v in m.get("moves") or []) if x] or None
        if moves is None and sp in edited:
            moves = g.level_moves(sp, int(m["level"])) or None     # the engine would use the ROM learnset
        out.append({"species": sp, "level": int(m["level"]), "moves": moves, "item": (m.get("item") or "").upper().replace(" ", "_") or None})
    return out, problems


def _wilson(w, n):
    from minasim import stats
    lo, hi = stats.wilson(w, n)
    return [round(lo, 3), round(hi, 3)]


def _cell(wins, n, target, turns=None):
    lo, hi = target
    rate = wins / n if n else 0
    ci = _wilson(wins, n) if n else [0, 0]
    verdict = "in band" if lo <= rate <= hi else ("too hard" if rate < lo else "too easy")
    if ci[1] < lo:
        verdict = "too hard"
    elif ci[0] > hi:
        verdict = "too easy"
    out = {"win": round(rate, 3), "ci": ci, "n": n, "target": [lo, hi], "verdict": verdict}
    if turns is not None:
        out["turns"] = round(turns, 1)
    return out


def _duel_cells(ctx, C, fight, n, label):
    cells = {}
    for band in BANDS:
        for st in STARTERS:
            wins = turns = 0
            for i in range(n):
                seed = C.seed_of("edit", label, band, st, i)
                draw = ctx.teams.player_team(fight, band, st, random.Random(seed))
                foe = ctx.catalog.party_for(fight, st)
                res = C.battle(None, seed=seed, player=C.make_mons(draw["mons"]), foe=C.make_mons(foe), player_policy=band,
                               foe_policy="rom", bag=draw["bag"], **C.foe_kwargs(fight, ctx))
                wins += res["winner"] == 0
                turns += res["turns"]
            cells["%s/%s" % (band, st)] = _cell(wins, n, ctx.target_for(fight, band), turns / n)
    return cells


def _journey(sc, runs, arena=None):
    from minasim import journey
    from minasim.signals import _common as C
    sc["arena_tuning"] = dict(arena or {})
    sc.setdefault("no_arena", False)
    sc.setdefault("arena_use", 0.5)
    jobs = [(sc, s, b, C.seed_of("journey", s, b, i)) for b in BANDS for s in STARTERS for i in range(runs)]
    import multiprocessing as mp
    with mp.get_context("fork").Pool(os.cpu_count() or 1) as pool:
        recs = pool.map(journey.run_one, jobs, chunksize=8)
    return journey.summarise(sc, recs)


def _journey_rows(summary, targets, only=None):
    rows = []
    for f in summary["fights"]:
        if only and f["id"] not in only:
            continue
        rows.append({"id": f["id"], "name": f["name"], "by": f["by"]})
    return rows


def mode_trainer(req):
    from minasim.report import Ctx
    from minasim.signals import _common as C
    ctx = Ctx(jobs=1)
    g = ctx.game
    _overlay(g, req.get("records"))
    t = req["trainer"]
    party, problems = _party(g, t["mons"], set(req.get("records") or {}))
    if not party:
        return {"error": "no usable Pokemon in the party", "problems": problems}
    cls = {"Leader": "gym", "Rival": "rival"}.get(t.get("class"), "trainer")
    stage = ctx.catalog.level_stage(max(m["level"] for m in party))
    fight = {"id": "edit:" + t["id"], "name": t.get("name") or t["id"], "class": cls, "stage": stage, "party": party,
             "source": "edit", "rival": False, "map": None, "gym_no": None}
    n = int(req.get("n", 40))
    out = {"mode": "trainer", "id": t["id"], "class": cls, "stage": stage, "problems": problems, "party": party,
           "duel": _duel_cells(ctx, C, fight, n, t["id"])}
    # the same fight in the chapter: a step whose name matches the trainer's name
    from minasim import journey
    sc = journey.load(SC)
    name = (t.get("name") or "").strip().lower()
    step = next((s for s in sc["steps"] if "fight" in s and name and (name == s.get("name", "").strip().lower()
                                                                     or name in s.get("name", "").lower())), None)
    if step and req.get("chapter", True):
        step["_party"] = party
        step["class"] = cls if step.get("class") in (None, "trainer") else step.get("class")
        summary = _journey(sc, int(req.get("runs", 40)))
        row = next((f for f in summary["fights"] if f["id"] == step["fight"]), None)
        tg = {b: ctx.target_for({"class": step.get("class", "trainer"), "source": "edit", "gym_no": None}, b) for b in BANDS}
        out["chapter"] = {"fight": step["fight"], "name": step.get("name"), "target": tg,
                          "by": {k: dict(v, **{"target": tg[k.split("/")[0]]}) for k, v in (row or {"by": {}})["by"].items()}}
    return out


def mode_route(req):
    from minasim.report import Ctx
    from minasim.signals import _common as C
    ctx = Ctx(jobs=1)
    g = ctx.game
    _overlay(g, req.get("records"))
    slots = []
    problems = []
    for s in req["slots"]:
        sp = g.try_resolve(s["species"])
        if sp is None:
            problems.append("unknown species %r" % s["species"])
            continue
        slots.append((sp, int(s["min"]), int(s["max"]), float(s["weight"])))
    if not slots:
        return {"error": "no usable slots", "problems": problems}
    n = int(req.get("n", 60))
    cells = {}
    per_slot = []
    for sp, lo, hi, w in slots:
        stage = ctx.catalog.level_stage(hi)
        fight = {"id": "edit:wild", "name": "wild", "class": "wild", "stage": stage, "party": [], "source": "edit",
                 "rival": False, "map": None, "gym_no": None}
        wins = {b: 0 for b in BANDS}
        k = max(8, n // 4)
        for band in BANDS:
            for st in STARTERS:
                for i in range(k):
                    seed = C.seed_of("route", sp, lo, hi, band, st, i)
                    rng = random.Random(seed)
                    lv = rng.randint(lo, hi)
                    fight["party"] = [{"species": sp, "level": lv, "moves": None, "item": None}]
                    draw = ctx.teams.player_team(fight, band, st, rng)
                    res = C.battle(None, seed=seed, player=C.make_mons(draw["mons"]), foe=C.make_mons(fight["party"]),
                                   player_policy=band, foe_policy="rom", bag=draw["bag"], **C.foe_kwargs(fight, ctx))
                    wins[band] += res["winner"] == 0
        per_slot.append({"species": sp, "min": lo, "max": hi, "weight": w,
                         "win": {b: round(wins[b] / (k * len(STARTERS)), 3) for b in BANDS}})
    tot = sum(s["weight"] for s in per_slot) or 1
    cells = {}
    for b in BANDS:
        rate = sum(s["weight"] * s["win"][b] for s in per_slot) / tot
        cells[b] = _cell(round(rate * 100), 100, ctx.targets["bands"]["wild"][b])
        cells[b]["win"] = round(rate, 3)
        cells[b]["n"] = "weighted over %d slots" % len(per_slot)
    out = {"mode": "route", "problems": problems, "slots": per_slot, "wild": cells}
    if req.get("journey", True):
        from minasim import journey
        with tempfile.NamedTemporaryFile("w", suffix=".yaml", delete=False) as f:
            json.dump({"name": "route", "start": "Town", "starter_level": 5, "steps": [
                {"segment": "Route"},
                {"wild": "Route", "battles": int(req.get("battles", 12)),
                 "table": ["%s:%d-%d:%g" % (sp, lo, hi, w) for sp, lo, hi, w in slots]},
                {"inn": "Town"}]}, f)
        sc = journey.load(f.name)
        os.unlink(f.name)
        s = _journey(sc, int(req.get("runs", 30)))
        out["journey"] = next((r["by"] for r in s["segments"] if r["segment"] == "Route"), {})
        out["blackouts"] = {k: v["blackouts"] for k, v in s["parties"].items()}
    return out


def _chapter_rows(summary):
    return {f["id"]: {"name": f["name"], "by": f["by"]} for f in summary["fights"]}


def mode_species(req):
    from minasim import journey
    from minasim.gamedata import Game
    runs = int(req.get("runs", 40))
    sc = journey.load(SC)
    before = _journey(sc, runs)
    _overlay(Game.get(), req.get("records"))
    sc2 = journey.load(SC)
    after = _journey(sc2, runs)
    return {"mode": "species", "before": _chapter_rows(before), "after": _chapter_rows(after),
            "segments_before": before["segments"], "segments_after": after["segments"]}


def mode_arena(req):
    from minasim import journey
    from minasim.report import Ctx
    runs = int(req.get("runs", 60))
    ctx = Ctx(jobs=1)
    sc = journey.load(SC)
    arena_fights = {s["fight"] for s in sc["steps"] if s.get("arena")}
    before = _journey(sc, runs)
    after = _journey(journey.load(SC), runs, arena=req.get("knobs"))
    tg = {b: ctx.target_for({"class": "gym", "source": "edit", "gym_no": None}, b) for b in BANDS}
    rows = {}
    for fid in arena_fights:
        rows[fid] = {"before": next(f for f in before["fights"] if f["id"] == fid),
                     "after": next(f for f in after["fights"] if f["id"] == fid)}
    return {"mode": "arena", "target": tg, "fights": rows, "arena": {s["fight"]: s["arena"] for s in sc["steps"] if s.get("arena")}}


MODES = {"trainer": mode_trainer, "route": mode_route, "species": mode_species, "arena": mode_arena}


def main():
    req = json.load(sys.stdin)
    _setup()
    try:
        res = MODES[req["mode"]](req)
    except Exception as e:      # engine not built, bad request: report it as data
        import traceback
        res = {"error": "%s: %s" % (type(e).__name__, e), "trace": traceback.format_exc()[-1500:]}
        if "libminasim" in str(e) or "engine" in str(e).lower():
            res["hint"] = "make balance-lib"
    sys.stdout.write(json.dumps(res))


if __name__ == "__main__":
    main()
