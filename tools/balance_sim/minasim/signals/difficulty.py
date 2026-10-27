"""Signal 3.3: fight difficulty per fight, band and starter (the headline signal).

Runs every catalogue fight for both bands and the three starters with a fresh player draw per battle
(seed = hash(fight, band, starter, k)), so results include team-building variance. Also collects move usage
and resource use that the TP, dominance and move signals reuse.
"""
import random

from .. import stats
from ..fights import STARTERS
from . import _common as C

SOURCE_LABEL = {"rom": "ROM data", "design": "design placeholder (early_game.yaml)", "placeholder": "generated placeholder"}


def fight_job(job):
    ctx = C.worker_ctx()
    fid, band, starter, ks = job
    fight = ctx.fight_by_id[fid]
    out = []
    for k in ks:
        seed = C.seed_of(fid, band, starter, k)
        draw = ctx.teams.player_team(fight, band, starter, random.Random(seed))
        foe = ctx.teams.foe_mons(fight, starter)
        res = C.battle(None, seed=seed, player=C.make_mons(draw["mons"]), foe=C.make_mons(foe),
                       player_policy=band, foe_policy="rom", log=False, bag=draw["bag"],
                       max_turns=ctx.targets["sample"]["turn_cap"], **C.foe_kwargs(fight))
        s = C.summ(res)
        del s["player"]
        s["seed"] = seed
        s["lead"] = draw["mons"][0]["species"]
        out.append(s)
    return (fid, band, starter, out)


def cell_n(ctx, fight, band):
    smp = ctx.targets["sample"]
    if fight["class"] in ("gym", "rival", "boss"):
        n = smp["gym_n"]
    elif fight["class"] == "wild":
        n = smp["fight_n"] // 2
    else:
        n = smp["fight_n"] // 3
    return max(ctx.min_cell, int(n * ctx.scale))


def run(ctx):
    sec = C.Section()
    jobs, chunk = [], 25
    for f in ctx.catalog.fights:
        for band in ctx.bands:
            n = cell_n(ctx, f, band)
            for st in STARTERS:
                for i in range(0, n, chunk):
                    jobs.append((f["id"], band, st, list(range(i, min(n, i + chunk)))))
    res = C.pmap(fight_job, jobs, ctx, "fights")
    agg = {}
    for fid, band, st, rows in res:
        agg.setdefault((fid, band, st), []).extend(rows)
    ctx.fight_rows = agg
    sec.battles = sum(len(v) for v in agg.values())
    fights_out, flags = [], []
    for f in ctx.catalog.fights:
        bb, by_start, by_start_sk = {}, {}, {}
        for band in ctx.bands:
            rows = [r for st in STARTERS for r in agg.get((f["id"], band, st), [])]
            n = len(rows)
            k = sum(r["w"] == 0 for r in rows)
            ci = stats.wilson(k, n)
            bb[band] = {"win_rate": round(k / n, 4) if n else 0, "ci": [round(ci[0], 4), round(ci[1], 4)], "n": n,
                        "hp_cost": round(sum(1 - r["hp"] for r in rows) / n, 3) if n else 0,
                        "tp_cost": round(sum(r["tp"] for r in rows) / n, 3) if n else 0,
                        "items_used": round(sum(r["items"] for r in rows) / n, 2) if n else 0,
                        "blackout": round(1 - k / n, 4) if n else 1, "turns": round(sum(r["turns"] for r in rows) / n, 1) if n else 0}
            for st in STARTERS:
                rs = agg.get((f["id"], band, st), [])
                rate = sum(r["w"] == 0 for r in rs) / len(rs) if rs else 0
                (by_start if band == ctx.bands[0] else by_start_sk)[st] = round(rate, 4)
        tgt = ctx.target_for(f, ctx.bands[0])
        fights_out.append({"trainer": f.get("trainer_id") or f["id"], "id": f["id"], "name": f["name"], "class": f["class"],
                           "stage": f["stage_id"], "map": f["map"], "source": f["source"], "gym_no": f["gym_no"],
                           "party": [{"species": m["species"], "level": m["level"]} for m in ctx.catalog.party_for(f, "CHARMANDER")],
                           "by_band": bb, "by_starter": by_start, "by_starter_other_band": by_start_sk, "target": tgt})
        flags.extend(fight_flags(ctx, f, bb, by_start, by_start_sk))
    sec.data["fights"] = fights_out
    sec.flags = flags
    return sec


def repro_cmd(f, band, starter, seed):
    return "python3 -m minasim duel --fight %s --band %s --starter %s --seed %d" % (f["id"], band, starter, seed)


def first_loss(ctx, f, band, starter=None):
    for st in ([starter] if starter else STARTERS):
        for r in ctx.fight_rows.get((f["id"], band, st), []):
            if r["w"] != 0:
                return r["seed"], st
    return None, STARTERS[0]


def fight_flags(ctx, f, bb, by_a, by_s):
    fl = []
    lowconf = f["source"] == "placeholder"
    exposure = {"gym": 1.0, "boss": 1.0, "rival": 0.9, "trainer": 0.6, "wild": 0.35}[f["class"]]
    for band, m in bb.items():
        lo, hi = ctx.target_for(f, band)
        dist, conf = C.outside_conf(m["win_rate"], m["ci"], lo, hi)
        if dist < 0.03 or conf < 0.8:
            continue
        sev = "red" if dist >= 0.15 else "amber"
        score = min(3.0, dist / 0.10) * conf * exposure * (0.2 if lowconf else 1.0)
        if lowconf:
            sev = "info"
        seed, st = first_loss(ctx, f, band)
        hard = m["win_rate"] < lo
        fl.append(C.flag(
            "fight:%s:%s" % (f["id"], band), "fight", sev, score,
            "%s is too %s for %s players" % (f["name"], "hard" if hard else "easy", band),
            "First-try win rate %.0f%% [%.0f, %.0f] (target %.0f-%.0f%%) over %d battles; %s. Party: %s." % (
                100 * m["win_rate"], 100 * m["ci"][0], 100 * m["ci"][1], 100 * lo, 100 * hi, m["n"], SOURCE_LABEL[f["source"]],
                ", ".join("%s L%d" % (x["species"].title(), x["level"]) for x in ctx.catalog.party_for(f, "CHARMANDER"))),
            stage=f["stage_id"], band=band, subject={"trainer": f.get("trainer_id") or f["id"]},
            metric={"name": "win_rate", "value": m["win_rate"], "ci": m["ci"], "target": [lo, hi]},
            suggestion=suggest_level(ctx, f, band, hard),
            repro={"seed": seed, "cmd": repro_cmd(f, band, st, seed or 1)}))
    if len(bb) == 2:
        a, s = bb["average"]["win_rate"], bb["skilled"]["win_rate"]
        if s - a > ctx.targets["flags"]["band_gap_pts"] and f["class"] != "wild":
            seed, st = first_loss(ctx, f, "average")
            fl.append(C.flag("fight:%s:gap" % f["id"], "fight", "info" if lowconf else "amber",
                             min(2.5, (s - a) / 0.2) * exposure * (0.2 if lowconf else 0.8),
                             "%s rewards knowledge too much" % f["name"],
                             "Skilled wins %.0f%%, average %.0f%% (gap %.0f pts, flag at %.0f)." % (
                                 100 * s, 100 * a, 100 * (s - a), 100 * ctx.targets["flags"]["band_gap_pts"]),
                             stage=f["stage_id"], band="both", subject={"trainer": f.get("trainer_id") or f["id"]},
                             metric={"name": "band_gap", "value": round(s - a, 4), "ci": None, "target": [0, ctx.targets["flags"]["band_gap_pts"]]},
                             repro={"seed": seed, "cmd": repro_cmd(f, "average", st, seed or 1)}))
    if f["class"] in ("gym", "rival", "boss") and by_a:
        lo_s = min(by_a, key=by_a.get)
        hi_s = max(by_a, key=by_a.get)
        spread = by_a[hi_s] - by_a[lo_s]
        if spread > ctx.targets["flags"]["starter_spread_pts"]:
            seed, st = first_loss(ctx, f, ctx.bands[0], lo_s)
            fl.append(C.flag("fight:%s:starter" % f["id"], "fight", "info" if lowconf else ("red" if spread > 0.5 else "amber"),
                             min(3.0, spread / 0.2) * exposure * (0.2 if lowconf else 0.9),
                             "%s is starter-gated" % f["name"],
                             "%s wins %.0f%% vs %s %.0f%% (spread %.0f pts; design target 15)." % (
                                 hi_s.title(), 100 * by_a[hi_s], lo_s.title(), 100 * by_a[lo_s], 100 * spread),
                             stage=f["stage_id"], band=ctx.bands[0], subject={"trainer": f.get("trainer_id") or f["id"]},
                             metric={"name": "starter_spread", "value": round(spread, 4), "ci": None,
                                     "target": [0, ctx.targets["flags"]["starter_spread_pts"]]},
                             repro={"seed": seed, "cmd": repro_cmd(f, ctx.bands[0], st, seed or 1)}))
    return fl


def suggest_level(ctx, f, band, hard):
    """A cheap what-if hint from the fight itself: how many levels the player's side is off."""
    top = max(m["level"] for m in ctx.catalog.party_for(f, "CHARMANDER"))
    par = ctx.targets["policies"][band]
    if f["class"] == "wild":
        return None
    return ("Lower the ace by 1-3 levels or add one weaker mon (player arrives at ace%+d under the %s band)" % (par["level_offset"], band)
            if hard else "Raise the ace by 1-3 levels or add a mon (player arrives at ace%+d under the %s band)" % (par["level_offset"], band))
