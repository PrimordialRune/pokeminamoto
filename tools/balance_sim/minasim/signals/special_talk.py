"""Signals 3.8/3.9: Signature and Talk. Paired ablations of the skilled policy (same draw, same seed):
  talk:    each talk mode (unnerve / provoke / smart) vs never talking; Charisma follows the stage, leaders and the
           rival carry the ROM's 30-point Talk malus (trainer_class is passed to the engine).
  special: every party mon awakened, Signature allowed vs Signature never used.
The default skilled policy uses talk_mode "smart" (targets.yaml); the numbers here show which mode pays."""
import random

from .. import stats
from ..fights import STARTERS
from . import _common as C

TALK_MODES = {"unnerve": "skilled_unnerve", "provoke": "skilled_provoke", "smart": "skilled"}


def job(j):
    ctx = C.worker_ctx()
    feature, mode, fid, n = j
    f = ctx.fight_by_id[fid]
    on, off = ("skilled", "skilled_nospecial") if feature == "special" else (TALK_MODES[mode], "skilled_notalk")
    a, b, used = [], [], 0
    for k in range(n):
        st = STARTERS[k % 3]
        seed = C.seed_of("st", feature, fid, k)
        draw = ctx.teams.player_team(f, "skilled", st, random.Random(seed))
        if feature == "special":
            for m in draw["mons"]:
                m["awakened"] = True
        for arm, pol in ((a, on), (b, off)):
            res = C.battle(None, seed=seed, player=C.make_mons(draw["mons"]), foe=C.make_mons(ctx.teams.foe_mons(f, st)),
                           player_policy=pol, foe_policy="rom", bag=draw["bag"], max_turns=ctx.targets["sample"]["turn_cap"],
                           **C.foe_kwargs(f))
            s = C.summ(res)
            arm.append(1.0 if s["w"] == 0 else 0.0)
            if arm is a:
                used += s["talk"] if feature == "talk" else s["special"]
    d, ci, z = stats.paired_diff(a, b)
    return {"feature": feature, "mode": mode, "fight": fid, "class": f["class"], "stage": f["stage"], "n": n, "delta": d, "ci": ci,
            "z": z, "uses": used / max(1, n), "source": f["source"]}


def run(ctx):
    sec = C.Section()
    n = max(20, int(300 * ctx.scale))
    if "skilled" not in ctx.bands:
        sec.notes.append("skilled band not run: Signature/Talk not evaluated")
        return sec
    fights = [f for f in ctx.catalog.fights if f["class"] in ("gym", "rival", "boss")]
    fights += [f for f in ctx.catalog.fights if f["class"] == "trainer" and f["source"] != "placeholder"][:12]
    jobs = [("special", "-", f["id"], n) for f in fights]
    jobs += [("talk", m, f["id"], n) for f in fights for m in TALK_MODES]
    res = C.pmap(job, jobs, ctx, "special_talk")
    sec.battles = 2 * sum(r["n"] for r in res)
    out = {"talk": [], "special": []}
    for r in res:
        out[r["feature"]].append({"fight": r["fight"], "class": r["class"], "stage": "s%02d" % r["stage"], "mode": r["mode"],
                                  "source": r["source"], "delta_win": round(r["delta"], 3),
                                  "ci": [round(r["ci"][0], 3), round(r["ci"][1], 3)], "uses_per_fight": round(r["uses"], 2), "n": r["n"]})
    sec.data["talk"], sec.data["special"] = out["talk"], out["special"]
    fl = ctx.targets["flags"]
    flags = []

    def agg(rows):
        n = sum(r["n"] for r in rows)
        d = sum(r["delta_win"] * r["n"] for r in rows) / max(1, n)
        se = (sum(((r["ci"][1] - r["ci"][0]) / 3.92) ** 2 * r["n"] ** 2 for r in rows) ** 0.5) / max(1, n)
        return d, se, sum(r["uses_per_fight"] * r["n"] for r in rows) / max(1, n), n

    designed = lambda rows: [r for r in rows if r["source"] != "placeholder"] or rows
    for name, rows in [("special", out["special"])] + [("talk:" + m, [r for r in out["talk"] if r["mode"] == m]) for m in TALK_MODES]:
        if not rows:
            continue
        d, se, used, nn = agg(rows)
        ph = all(r["source"] == "placeholder" for r in rows)
        kind = name.split(":")[0]
        title = ("Signature" if kind == "special" else "Talk (%s)" % name.split(":")[1])
        base = dict(band="skilled", metric={"name": "delta_win", "value": round(d, 3), "ci": [round(d - 1.96 * se, 3), round(d + 1.96 * se, 3)],
                                            "target": [fl["item_dead_delta"], 0.15]})
        if used < 0.02:
            flags.append(C.flag("%s:unused" % name, kind, "info" if ph else "amber", 0.9, "%s is never used" % title,
                                "Used %.2f times per fight over %d battles." % (used, nn), **base))
        elif d + 1.96 * se < fl["item_dead_delta"] and (kind == "special" or name.endswith("smart")):
            flags.append(C.flag("%s:useless" % name, kind, "info" if ph else "amber", 0.9, "%s does not help" % title,
                                "Win rate %+.1f pts (CI %+.1f..%+.1f) when used %.2f times per fight; %d battles." % (
                                    100 * d, 100 * (d - 1.96 * se), 100 * (d + 1.96 * se), used, nn), **base))
        elif d - 1.96 * se > 0.15:
            flags.append(C.flag("%s:dominant" % name, kind, "info" if ph else "red", 1.8, "%s dominates" % title,
                                "Win rate %+.1f pts when used %.2f times per fight; %d battles." % (100 * d, used, nn), **base))
    sec.data["talk_summary"] = {m: dict(zip(("delta", "se", "uses", "n"), agg([r for r in out["talk"] if r["mode"] == m]))) for m in TALK_MODES if out["talk"]}
    sec.flags = flags
    return sec
