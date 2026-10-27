"""Signal 3.4: items per stage. Paired ablation: the same draws and seeds with and without 3 of the item in the bag,
in the hardest fights of the stage. Non-healing items need engine support for battle items (reported as a gap)."""
import random

from .. import stats
from ..fights import STARTERS
from . import _common as C

# healing tiers: item -> first badge count at which a shop sells it (PLACEHOLDER until marts are scripted)
HEAL_ITEMS = {"POTION": 0, "SUPER_POTION": 2, "HYPER_POTION": 4, "MAX_POTION": 6, "FULL_RESTORE": 7}


def item_job(job):
    ctx = C.worker_ctx()
    item, si, fids, n, band = job
    a, b, used = [], [], 0
    for fid in fids:
        f = ctx.fight_by_id[fid]
        for k in range(n):
            st = STARTERS[k % 3]
            seed = C.seed_of("item", item, fid, band, k)
            draw = ctx.teams.player_team(f, band, st, random.Random(seed))
            foe = C.make_mons(ctx.teams.foe_mons(f, st))
            base_bag = {i: c for i, c in draw["bag"].items() if i not in HEAL_ITEMS or i == item and False}
            bag_b = {i: c for i, c in draw["bag"].items() if i != item}
            bag_a = dict(bag_b)
            bag_a[item] = bag_a.get(item, 0) + 3
            for arm, bag in ((a, bag_a), (b, bag_b)):
                res = C.battle(None, seed=seed, player=C.make_mons(draw["mons"]), foe=foe, player_policy=band,
                               foe_policy="rom", bag=bag, max_turns=ctx.targets["sample"]["turn_cap"], **C.foe_kwargs(f))
                s = C.summ(res)
                arm.append(1.0 if s["w"] == 0 else 0.0)
                if arm is a:
                    used += s["item_by"].get(item, 0)
    d, ci, z = stats.paired_diff(a, b)
    return {"item": item, "stage": si, "n": len(a), "delta": d, "ci": ci, "z": z, "uses": used / max(1, len(a)),
            "win": sum(a) / max(1, len(a))}


def run(ctx):
    sec = C.Section()
    n = max(15, int(ctx.targets["sample"]["item_n"] * ctx.scale / 4))
    jobs = []
    band = ctx.bands[0]
    for item, badge in HEAL_ITEMS.items():
        if item not in ctx.game.items:
            continue
        for si in range(2 * badge, 18, 3):
            fs = [f for f in ctx.catalog.fights if f["stage"] == si and f["class"] in ("gym", "boss", "rival")]
            fs += [f for f in ctx.catalog.fights if f["stage"] == si and f["class"] == "trainer"][-3:]
            if fs:
                jobs.append((item, si, [f["id"] for f in fs][:5], n, band))
    res = C.pmap(item_job, jobs, ctx, "items")
    sec.battles = sum(2 * r["n"] for r in res)
    fl = ctx.targets["flags"]
    rows, flags = [], []
    for r in res:
        price = ctx.game.items[r["item"]]["price"]
        per = r["delta"] / (price * 3 / 1000.0)
        verdict = "strong" if r["delta"] > fl["item_strong_delta"] else "dead" if r["delta"] < fl["item_dead_delta"] else "ok"
        rows.append({"item": r["item"], "stage": "s%02d" % r["stage"], "delta_win": round(r["delta"], 3),
                     "ci": [round(r["ci"][0], 3), round(r["ci"][1], 3)], "price": price, "delta_per_1000": round(per, 3),
                     "uses_per_fight": round(r["uses"], 2), "verdict": verdict, "n": r["n"]})
    # an item that is dead in every stage it exists in is dead content; strong in one stage is a flag
    by = {}
    for row in rows:
        by.setdefault(row["item"], []).append(row)
    for item, lst in by.items():
        if all(x["verdict"] == "dead" and x["ci"][1] < 0.03 for x in lst) and sum(x["n"] for x in lst) >= 60:
            flags.append(C.flag("item:%s:dead" % item, "item", "amber", 0.9, "%s changes nothing" % item.replace("_", " ").title(),
                                "Across %d stages the win rate moves %+.1f pts at most when 3 are in the bag (price %d)." % (
                                    len(lst), 100 * max(x["delta_win"] for x in lst), lst[0]["price"]),
                                subject={"item": item}, metric={"name": "delta_win", "value": max(x["delta_win"] for x in lst), "ci": None,
                                                                "target": [fl["item_dead_delta"], fl["item_strong_delta"]]}))
        for x in lst:
            if x["verdict"] == "strong" and x["ci"][0] > fl["item_strong_delta"] * 0.6:
                flags.append(C.flag("item:%s:%s" % (item, x["stage"]), "item", "amber", min(2.0, x["delta_win"] / 0.1),
                                    "%s carries %s" % (item.replace("_", " ").title(), x["stage"]),
                                    "3 in the bag lift the win rate %+.0f pts [%+.0f, %+.0f] (price %d, %.2f per 1000)." % (
                                        100 * x["delta_win"], 100 * x["ci"][0], 100 * x["ci"][1], x["price"], x["delta_per_1000"]),
                                    stage=x["stage"], band=band, subject={"item": item},
                                    metric={"name": "delta_win", "value": x["delta_win"], "ci": x["ci"], "target": [fl["item_dead_delta"], fl["item_strong_delta"]]}))
    sec.data["items"] = rows
    sec.notes.append("Only healing items are ablated; status cures, X items and held items need engine battle-item support.")
    sec.flags = flags
    return sec
