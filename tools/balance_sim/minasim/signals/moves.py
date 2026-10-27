"""Signal 3.2: move strength at its learn level.

Static: expected damage per TP against the stage pool at the learn level. Dynamic: paired ablation, the mon at
that level duelling the stage pool with its last-4 moveset vs the same moveset without the move (same seeds);
delta_win and usage (share of the mon's own turns spent on the move).
"""
import random

from .. import stats
from ..policies import move_value, stat
from . import _common as C


def entries(ctx):
    g = ctx.game
    out = []
    for sp, ls in g.learnsets.items():
        if sp not in g.species or sp == "NONE":
            continue
        done = set()
        evo = g.evolutions.get(sp)
        for lv, mv in ls:
            if evo and lv >= evo[0]:                    # the mon has evolved by then; its evolved form owns the entry
                continue
            if lv < 1 or mv in done or mv not in g.moves:
                continue
            done.add(mv)
            out.append((sp, mv, lv))
    return out


def stage_for_level(ctx, lv):
    return min(range(18), key=lambda i: abs(ctx.catalog.ref_level[i] - lv))


def static_dpt(ctx, sp, mv, lv, pool):
    g = ctx.game
    m = g.moves[mv]
    if m["power"] == 0:
        return None
    me = {"species": sp, "level": lv, "max_hp": 1, "hp": 1}
    vals = []
    for osp, olv in pool:
        d = {"species": osp, "level": olv, "max_hp": stat(g, osp, olv, "hp"), "hp": 1}
        vals.append(move_value(g, me, d, mv))
    return (sum(vals) / len(vals)) / max(1, m["tp"]) if vals else None


def move_job(job):
    ctx = C.worker_ctx()
    sp, mv, lv, pool, n, band = job
    g = ctx.game
    known = []
    for l, m in g.learnsets[sp]:
        if l <= lv and m not in known:
            known.append(m)
    with_m = known[-4:]
    without = [m for m in known if m != mv][-4:]
    rng = random.Random(C.seed_of("mv", sp, mv, band))
    opps = [rng.choice(pool) for _ in range(n)]
    a, b, use, turns_tot = [], [], 0, 0
    for i, (osp, olv) in enumerate(opps):
        seed = C.seed_of("mv", sp, mv, band, i)
        foe = C.make_mons([{"species": g.evolve_to(osp, olv), "level": olv, "moves": None}])
        for arm, moves in ((a, with_m), (b, without)):
            res = C.battle(None, seed=seed, player=C.make_mons([{"species": sp, "level": lv, "moves": moves}]), foe=foe,
                           player_policy=band, foe_policy="rom", max_turns=60)
            s = C.summ(res)
            arm.append(1.0 if s["w"] == 0 else 0.0)
            if arm is a:
                mu = s["moves"] or {}
                use += mu.get(mv, 0)
                turns_tot += sum(mu.values()) or s["turns"]
    d, ci, z = stats.paired_diff(a, b)
    return {"sp": sp, "mv": mv, "lv": lv, "a": sum(a), "b": sum(b), "n": len(a), "delta": d, "ci": ci, "z": z,
            "usage": use / max(1, turns_tot)}


def run(ctx):
    sec = C.Section()
    smp = ctx.targets["sample"]
    ents = entries(ctx)
    cap = max(40, int(smp["max_moves"] * (1 if ctx.scale >= 1 else max(ctx.scale * 4, 0.3))))
    if len(ents) > cap:                                 # spread over species and levels, deterministic
        random.Random(11).shuffle(ents)
        ents = sorted(ents[:cap])
    n = max(30, int(smp["move_n"] * min(1.0, ctx.scale * 2)))
    jobs, pools = [], {}
    for sp, mv, lv in ents:
        si = stage_for_level(ctx, lv)
        if si not in pools:
            from .species import pool_for
            pools[si] = pool_for(ctx, si)
        if pools[si]:
            jobs.append((sp, mv, lv, pools[si], n, "skilled" if "skilled" in ctx.bands else ctx.bands[-1]))
    res = C.pmap(move_job, jobs, ctx, "moves")
    sec.battles = 2 * sum(r["n"] for r in res)
    g = ctx.game
    rows, flags = [], []
    dpts = []
    for r, j in zip(res, jobs):
        si = stage_for_level(ctx, r["lv"])
        dpts.append(static_dpt(ctx, r["sp"], r["mv"], r["lv"], pools[si]))
    allv = sorted(x for x in dpts if x is not None)
    p90 = allv[int(len(allv) * 0.9)] if allv else 0
    fl = ctx.targets["flags"]
    pvals = [stats.p_from_z(r["z"]) for r in res]
    disc = stats.bh(pvals) if pvals else []
    for r, dpt, d in zip(res, dpts, disc):
        si = stage_for_level(ctx, r["lv"])
        m = g.moves[r["mv"]]
        if r["usage"] < fl["move_dead_usage"] and r["delta"] < 0.01:
            verdict = "dead"
        elif r["delta"] > fl["move_dominant_delta"] and r["usage"] > 0.5 and (dpt or 0) >= p90:
            verdict = "dominant"
        elif r["delta"] > 0.05:
            verdict = "strong"
        elif r["delta"] < 0.01 and r["usage"] < 0.1:
            verdict = "weak"
        else:
            verdict = "ok"
        row = {"move": r["mv"], "species": r["sp"], "learn_level": r["lv"], "stage": "s%02d" % si, "tp_cost": m["tp"],
               "damage_per_tp": round(dpt, 4) if dpt is not None else None, "usage": round(r["usage"], 3),
               "delta_win": round(r["delta"], 3), "ci": [round(r["ci"][0], 3), round(r["ci"][1], 3)], "verdict": verdict, "n": r["n"]}
        rows.append(row)
        if verdict in ("dominant", "dead") and (d or verdict == "dead") and r["n"] >= 30:
            dom = verdict == "dominant"
            flags.append(C.flag("move:%s:%s" % (r["sp"], r["mv"]), "move", "amber" if dom else "info", (1.2 if dom else 0.3) * min(2.0, abs(r["z"]) / 3 + 0.5),
                                "%s is %s on %s" % (r["mv"].title().replace("_", " "), "dominant" if dom else "dead weight", r["sp"].title()),
                                "At L%d it lifts the win rate %+.0f pts [%+.0f, %+.0f] and takes %.0f%% of the mon's turns (%.3f dmg/TP, cost %d)." % (
                                    r["lv"], 100 * r["delta"], 100 * r["ci"][0], 100 * r["ci"][1], 100 * r["usage"], dpt or 0, m["tp"]),
                                stage="s%02d" % si, band="skilled", subject={"species": r["sp"], "move": r["mv"]},
                                metric={"name": "delta_win", "value": row["delta_win"], "ci": row["ci"], "target": [0.01, fl["move_dominant_delta"]]},
                                repro={"seed": C.seed_of("mv", r["sp"], r["mv"], "skilled", 0),
                                       "cmd": "python3 -m minasim duel --player %s:%d --foe PIDGEY:%d --band skilled --n 200" % (r["sp"], r["lv"], r["lv"])}))
    sec.data["moves"] = rows
    sec.flags = flags
    return sec
