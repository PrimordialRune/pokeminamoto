"""Signal 3.1: species strength per stage.

1v1 duels of each obtainable species (at a stage-typical level, with its natural moveset) against a pool of
the stage's trainer and wild mons. Win rate -> rating (1500 + 400 log10 odds), robust z within the stage,
flagged by BH across all cells and a practical-size guard. Also yields power spikes/dips along evolution lines.
"""
import math
import random

from .. import stats
from . import _common as C


def pool_for(ctx, si):
    seen = {}
    for f in ctx.catalog.fights:
        if f["stage"] == si and f["class"] in ("trainer", "wild", "gym", "rival", "boss") and f["source"] != "placeholder":
            for m in ctx.catalog.party_for(f, "CHARMANDER"):
                seen[(m["species"], m["level"])] = 1
    if len(seen) < 6:                                  # thin data: add the generated fights too
        for f in ctx.catalog.fights:
            if f["stage"] == si:
                for m in ctx.catalog.party_for(f, "CHARMANDER"):
                    seen[(m["species"], m["level"])] = 1
    return sorted(seen)


def matchups(per):
    """Best and worst opposing species (up to 5 each) from the duels already run."""
    rows = sorted(((v[0] / v[1], k) for k, v in per.items() if v[1]), key=lambda t: (-t[0], t[1]))
    mk = lambda lst: [{"species": k, "win_rate": round(w, 3)} for w, k in lst]
    return {"best": mk(rows[:5]), "worst": mk(sorted(rows, key=lambda t: (t[0], t[1]))[:5])}


def cells(ctx):
    g, cat = ctx.game, ctx.catalog
    out = []
    for si in range(18):
        pool = pool_for(ctx, si)
        if not pool:
            continue
        lv = cat.ref_level[si] - 2
        sp_pool = ctx.teams.catch_pool(si)
        starters = ["BULBASAUR", "CHARMANDER", "SQUIRTLE"]
        allb = sorted(g.species)
        cap = ctx.targets["sample"]["max_cells_species"]
        # full run: every species line at every stage (the dex view); quick: the obtainable ones first, capped
        cand = sorted(set(allb) | set(starters), key=lambda b: (-sp_pool.get(g.base_form(b), 0), b))
        if ctx.scale < 1.0:
            cand = cand[:cap]
        pool_all = {g.evolve_to(x, lv) for x in sp_pool}
        for sp in cand:
            pre = [e for e in g.evolutions.items() if e[1][1] == sp]
            floor = pre[0][1][0] if pre else 1                       # an evolved form is not met before its evolution level
            out.append((si, sp, max(3, lv, floor if pre and floor < 100 else 1), pool, sp in pool_all or sp in sp_pool))
    return out


def species_job(job):
    ctx = C.worker_ctx()
    si, sp, lv, pool, band, k_opp, n = job
    rng = random.Random(C.seed_of("sp", si, sp, band))
    opps = rng.sample(pool, min(k_opp, len(pool)))
    w = 0
    per = {}
    games = 0
    hp, turns = 0.0, 0
    for oi, (osp, olv) in enumerate(opps):
        for j in range(n):
            seed = C.seed_of("sp", si, sp, band, oi, j)
            res = C.battle(None, seed=seed, player=C.make_mons([{"species": sp, "level": lv, "moves": None}]),
                           foe=C.make_mons([{"species": ctx.game.evolve_to(osp, olv), "level": olv, "moves": None}]),
                           player_policy=band, foe_policy="rom", max_turns=60)
            s = C.summ(res)
            w += s["w"] == 0
            games += 1
            a = per.setdefault(osp, [0, 0])
            a[0] += s["w"] == 0
            a[1] += 1
            hp += s["hp"]
            turns += s["turns"]
    return {"stage": si, "species": sp, "level": lv, "band": band, "w": w, "n": games, "hp": hp / max(1, games), "per": per}


def run(ctx):
    sec = C.Section()
    smp = ctx.targets["sample"]
    k = max(8, int(smp["species_pool"] * max(ctx.scale, 0.6)))
    n = max(2, int(round(smp["species_n"] * min(1.0, ctx.scale * 2))))
    obt = {(si, sp): ob for (si, sp, lv, pool, ob) in cells(ctx)}
    jobs = [(si, sp, lv, pool, band, k, n) for (si, sp, lv, pool, _ob) in cells(ctx) for band in ctx.bands]
    res = C.pmap(species_job, jobs, ctx, "species")
    sec.battles = sum(r["n"] for r in res)
    rows, flags = [], []
    by = {}
    for r in res:
        by.setdefault((r["stage"], r["band"]), []).append(r)
    cellp, cellrow = [], []
    for (si, band), lst in sorted(by.items()):
        rates = [(r["w"] + 0.5) / (r["n"] + 1) for r in lst]
        ratings = [1500 + 400 * math.log10(p / (1 - p)) for p in rates]
        zs = stats.robust_z(ratings)
        for r, p, rt, z in zip(lst, rates, ratings, zs):
            ci = stats.wilson(r["w"], r["n"])
            row = {"species": r["species"], "stage": "s%02d" % si, "band": band, "rating": round(rt), "win_rate": round(r["w"] / r["n"], 3),
                   "ci": [round(ci[0], 3), round(ci[1], 3)], "z": round(z, 2), "n": r["n"], "level": r["level"], "hp_left": round(r["hp"], 2),
                   "matchups": matchups(r["per"]), "obtainable": obt.get((r["stage"], r["species"]), False)}
            rows.append(row)
            cellp.append(stats.p_from_z(z))
            cellrow.append(row)
    disc = stats.bh(cellp) if cellp else []
    zr, za = ctx.targets["flags"]["species_z_red"], ctx.targets["flags"]["species_z_amber"]
    wk, st = ctx.targets["flags"]["species_win_weak"], ctx.targets["flags"]["species_win_strong"]
    for row, d in zip(cellrow, disc):
        strong = row["z"] > 0
        pract = (row["win_rate"] > st) if strong else (row["win_rate"] < wk)
        if abs(row["z"]) >= za and d and pract and row["n"] >= 20 and row["obtainable"]:
            sev = "red" if abs(row["z"]) >= zr else "amber"
            flags.append(C.flag("species:%s:%s:%s" % (row["species"], row["stage"], row["band"]), "species", sev,
                                min(3.0, abs(row["z"]) / 2) * 0.7,
                                "%s is %s at %s (%s)" % (row["species"].title(), "too strong" if strong else "too weak", row["stage"], row["band"]),
                                "Beats the stage pool %.0f%% [%.0f, %.0f], z=%+.1f over %d duels at L%d." % (
                                    100 * row["win_rate"], 100 * row["ci"][0], 100 * row["ci"][1], row["z"], row["n"], row["level"]),
                                stage=row["stage"], band=row["band"], subject={"species": row["species"]},
                                metric={"name": "win_rate", "value": row["win_rate"], "ci": row["ci"], "target": [wk, st]},
                                repro={"seed": C.seed_of("sp", row["stage"], row["species"]),
                                       "cmd": "python3 -m minasim duel --player %s:%d --foe PIDGEY:%d --band %s --n 200" % (row["species"], row["level"], row["level"], row["band"])}))
    # evolution dips: an evolved form rated below its pre-evolution at the same stage
    g = ctx.game
    idx = {(r["species"], r["stage"], r["band"]): r for r in rows}
    for (sp, stg, band), r in idx.items():
        pre = [a for a, (_, to) in g.evolutions.items() if to == sp]
        for p in pre:
            q = idx.get((p, stg, band))
            if q and q["win_rate"] - r["win_rate"] > 0.15 and q["n"] >= 20:
                flags.append(C.flag("species:dip:%s:%s:%s" % (sp, stg, band), "species", "info", 0.4,
                                    "%s is weaker than %s at %s" % (sp.title(), p.title(), stg),
                                    "%.0f%% vs %.0f%% against the same pool (%s)." % (100 * r["win_rate"], 100 * q["win_rate"], band),
                                    stage=stg, band=band, subject={"species": sp},
                                    metric={"name": "win_rate", "value": r["win_rate"], "ci": r["ci"], "target": [q["win_rate"], 1]}))
    sec.data["species"] = rows
    sec.flags = flags
    return sec
