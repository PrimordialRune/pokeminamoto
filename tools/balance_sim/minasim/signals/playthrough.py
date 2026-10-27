"""Signals 3.5 (TP economy) and 3.7 (level curve): whole playthroughs, battle after battle.

One run = a starter and a band walking the stages in order: wild grind, the stage's trainers, rival, gym.
HP, TP and EXP carry between battles (no Pokemon Centers: rest only at an inn, at a stage's end or after a
blackout; skilled also walks back to an inn when TP/HP run low). Levels and evolutions come from the real
EXP curves, so the level curve is emergent rather than assumed. A third of the runs do no wild grinding at
all to give the trainers-only `exp_level`.
"""
import random
import statistics

from .. import stats
from ..fights import STARTERS
from ..policies import stat
from . import _common as C

WILD_GRIND = {"average": 5, "skilled": 2}
MAX_TRIES = 4


def _party_state(team):
    return {"hp": sum(m["hp"] for m in team) / max(1, len(team)), "tp": sum(m["tp"] for m in team) / max(1, len(team))}


def _pick_lead(ctx, team, party, band, rng):
    """Order the party for a fight: the member with the best matchup against the foe's first mon leads."""
    from ..policies import move_value
    par = ctx.targets["policies"][band]
    if len(team) < 2 or rng.random() > par.get("lead_matchup_prob", 1.0):
        return
    g = ctx.game
    foe = party[0]
    fo = {"species": foe["species"], "level": foe["level"], "max_hp": stat(g, foe["species"], foe["level"], "hp"), "hp": 1}
    fmoves = g.level_moves(foe["species"], foe["level"]) or ["TACKLE"]

    def score(m):
        if m["hp"] < 0.35:
            return -9
        me = {"species": m["species"], "level": m["level"], "max_hp": m["max_hp"], "hp": m["max_hp"]}
        mine = max(move_value(g, me, fo, mv) for mv in (m["moves"] or g.level_moves(m["species"], m["level"]) or ["TACKLE"]))
        theirs = max(move_value(g, fo, me, mv) for mv in fmoves)
        lvl = min(0, m["level"] - foe["level"] + 6) * 0.05          # do not lead a member far below the foe
        return mine - 0.8 * theirs + lvl
    team.sort(key=score, reverse=True)


def _do_battle(ctx, team, bag, fight_party, band, seed, kind, stage):
    """Run one battle with carried state; returns (won, result summary). Updates team hp/tp/bag in place."""
    g = ctx.game
    mons = C.make_mons([{"species": m["species"], "level": m["level"], "moves": m["moves"], "item": None,
                         "awakened": m.get("awakened", False)} for m in team],
                       tp_spent=[int(round((1 - m["tp"]) * 100)) for m in team],
                       hp=[max(1, int(round(m["hp"] * m["max_hp"]))) if m["hp"] > 0 else 0 for m in team] if ctx.engine_hp else None,
                       exps=[m["exp"] for m in team] if ctx.engine_exp else None)
    foe = C.make_mons(fight_party)
    res = C.battle(None, seed=seed, player=mons, foe=foe, player_policy=band, foe_policy="rom", bag=dict(bag),
                   max_turns=ctx.targets["sample"]["turn_cap"])
    for m, r in zip(team, res["player"]):
        if ctx.engine_hp:
            m["hp"] = max(0.0, r["hp"]) / r["max_hp"]
        m["tp"] = r["tp"] / r["tp_max"]
        m["max_hp"] = r["max_hp"]
        m["fought"] = r["tp"] < r["tp_max"] or r["hp"] < r["max_hp"]
        if ctx.engine_exp and r.get("exp") is not None:
            m["exp_engine"] = (r["level"], r["exp"], r.get("exp_gained", 0))
    for it, n in (res.get("items_used") or {}).items():
        bag[it] = max(0, bag.get(it, 0) - n)
    return res["winner"] == 0, res


def _gain_exp(ctx, team, foes, trainer):
    g = ctx.game
    if ctx.engine_exp:                      # the ROM's own EXP split (participants, trainer bonus, level-ups)
        for m in team:
            lv, ex, gained = m.pop("exp_engine", (m["level"], m["exp"], 0))
            m["level"], m["exp"] = lv, ex
            new = g.evolve_to(m["species"], m["level"])
            m["species"] = new
            m["max_hp"] = stat(g, m["species"], m["level"], "hp")
        return
    part = [m for m in team if m.get("fought") and m["hp"] > 0] or [team[0]]
    total = sum(g.species[f["species"]]["exp_yield"] * f["level"] // 7 * (3 if trainer else 2) // 2 for f in foes)
    share = max(1, total // len(part))
    for m in part:
        m["exp"] += share
        grow = g.species[m["species"]]["growth"]
        while m["level"] < 100 and m["exp"] >= g.exp_for_level(grow, m["level"] + 1):
            m["level"] += 1
        new = g.evolve_to(m["species"], m["level"])
        if new != m["species"]:
            m["species"] = new
        m["max_hp"] = stat(g, m["species"], m["level"], "hp")


def _need(srec, team):
    """Count a rest as *needed* when TP or HP got low enough to hurt the next fight (not for optional top-ups)."""
    tp = sum(m["tp"] for m in team) / len(team)
    hp = sum(m["hp"] for m in team) / len(team)
    if tp < 0.35 or hp < 0.5:
        srec["needed"] += 1
        srec["needed_tp"] += tp < 0.35
        srec["needed_hp"] += hp < 0.5


def _rest(team):
    for m in team:
        m["hp"], m["tp"] = 1.0, 1.0


def _refill_bag(ctx, band, badges):
    par = ctx.targets["policies"][band]
    n = int(par["potions_base"] + par["potions_per_badge"] * badges)
    bag = {"POTION": max(0, n - n // 3 * (badges >= 2) - n // 4 * (badges >= 5))}
    if badges >= 2:
        bag["SUPER_POTION"] = n // 3
    if badges >= 5:
        bag["HYPER_POTION"] = n // 4
    return {k: v for k, v in bag.items() if v}


def _new_mon(ctx, species, level):
    return {"awakened": ctx.rng_awake.random() < ctx.targets["policies"][ctx.cur_band].get("special_awake_prob", 0.0),
            "species": species, "level": level, "exp": ctx.game.exp_for_level(ctx.game.species[species]["growth"], level),
            "moves": None, "hp": 1.0, "tp": 1.0, "max_hp": stat(ctx.game, species, level, "hp"), "fought": False}


def run_one(job):
    ctx = C.worker_ctx()
    g, cat = ctx.game, ctx.catalog
    starter, band, idx, grind = job
    ctx.cur_band = band
    import random as _r
    ctx.rng_awake = _r.Random(C.seed_of("awake", starter, band, idx))
    par = ctx.targets["policies"][band]
    seed0 = C.seed_of("play", starter, band, idx, grind)
    rng = random.Random(seed0)
    team = [_new_mon(ctx, starter, 5)]
    bag = _refill_bag(ctx, band, 0)
    rec = {"fights": [], "stages": {}, "stuck": 0, "battles": 0, "rests": 0, "starter": starter, "band": band, "grind": grind}
    k = 0
    for si in range(18):
        badges = si // 2
        srec = rec["stages"].setdefault(si, {"battles": 0, "rests": 0, "needed": 0, "needed_tp": 0, "needed_hp": 0, "blackouts": 0, "tp_left": None, "hp_left": None})
        # catches: fill the party up to the band's party size
        size = int(max(1, min(6, round(par["party_size_base"] + par["party_per_badge"] * badges)))) if si >= 2 else 1
        wild = [w for w in cat.wild[si]]
        while len(team) < size and wild:
            sp, lo, hi, w, _ = rng.choices(wild, [x[3] for x in wild])[0]
            sp = g.base_form(sp)
            if all(g.base_form(m["species"]) != sp for m in team):
                lv = rng.randint(lo, hi)
                team.append(_new_mon(ctx, g.evolve_to(sp, lv), lv))
            else:
                size -= 0
                break
        if band == "skilled":
            for m in team:
                m["moves"] = ctx.teams.best_moves(m["species"], m["level"], si)
        wild_n = grind and WILD_GRIND[band]
        seq = []
        if wild:
            for _ in range(wild_n or 0):
                sp, lo, hi, w, _s = rng.choices(wild, [x[3] for x in wild])[0]
                seq.append(({"id": "wild:%d" % si, "class": "wild", "stage": si},
                            [{"species": sp, "level": rng.randint(lo, hi), "moves": None, "item": None}]))
        fs = [f for f in cat.fights if f["stage"] == si and f["class"] in ("trainer", "rival")]
        gym = [f for f in cat.fights if f["stage"] == si and f["class"] in ("gym", "boss")]
        for f in fs + gym:
            seq.append((f, cat.party_for(f, starter)))
        for f, party in seq:
            kind = f["class"]
            foe_ace = max(m["level"] for m in party)
            if f in gym:                                            # arrival at the gym town: record before the inn
                srec["tp_left"] = sum(m["tp"] for m in team) / len(team)
                srec["hp_left"] = sum(m["hp"] for m in team) / len(team)
                _need(srec, team)
                _rest(team)
                srec["rests"] += 1
                rec["rests"] += 1
                bag = _refill_bag(ctx, band, badges)
            elif band == "skilled" and (min(m["tp"] for m in team[:1]) < 0.15 or sum(m["hp"] for m in team) / len(team) < 0.35):
                _need(srec, team)
                _rest(team)                                         # walk back to an inn
                srec["rests"] += 1
                rec["rests"] += 1
            won = False
            for attempt in range(MAX_TRIES):
                k += 1
                _pick_lead(ctx, team, party, band, rng)
                before = [dict(m) for m in team]
                won, res = _do_battle(ctx, team, bag, party, band, seed0 * 31 + k, kind, si)
                rec["battles"] += 1
                srec["battles"] += 1
                if attempt == 0:
                    rec["fights"].append({"stage": si, "fight": f["id"], "class": kind, "won": won,
                                          "lead": max(m["level"] for m in before), "median": statistics.median(sorted((m["level"] for m in before), reverse=True)[:3]),
                                          "bench": statistics.mean(sorted((m["level"] for m in before), reverse=True)[1:]) if len(before) > 1 else None,
                                          "foe_ace": foe_ace, "tp": sum(m["tp"] for m in before) / len(before),
                                          "hp": sum(m["hp"] for m in before) / len(before), "turns": res["turns"],
                                          "min_tp": res.get("min_tp")})
                if won:
                    _gain_exp(ctx, team, party, kind != "wild")
                    break
                srec["blackouts"] += 1
                srec["needed"] += 1
                if ctx.engine_exp:
                    _gain_exp(ctx, team, party, kind != "wild")
                _rest(team)
                rec["rests"] += 1
                srec["rests"] += 1
                bag = _refill_bag(ctx, band, badges)
                if grind:                                           # a little grinding after a loss
                    for _ in range(3):
                        if wild:
                            sp, lo, hi, w, _s = rng.choices(wild, [x[3] for x in wild])[0]
                            k += 1
                            ok, _r = _do_battle(ctx, team, bag, [{"species": sp, "level": rng.randint(lo, hi), "moves": None, "item": None}],
                                                band, seed0 * 31 + k, "wild", si)
                            rec["battles"] += 1
                            if ok:
                                _gain_exp(ctx, team, [{"species": sp, "level": hi}], False)
                            _rest(team) if sum(m["hp"] for m in team) / len(team) < 0.4 else None
            else:
                rec["stuck"] += 1
                rec.setdefault("stuck_at", []).append(f["id"])
                continue
        if si % 2 == 1:                                           # end of a span: inn
            if srec["tp_left"] is None:
                srec["tp_left"] = sum(m["tp"] for m in team) / len(team)
                srec["hp_left"] = sum(m["hp"] for m in team) / len(team)
            if sum(m["tp"] for m in team) / len(team) < 0.95 or sum(m["hp"] for m in team) / len(team) < 0.95:
                _need(srec, team)
                _rest(team)
                rec["rests"] += 1
                srec["rests"] += 1
        else:
            if srec["tp_left"] is None:
                srec["tp_left"] = sum(m["tp"] for m in team) / len(team)
                srec["hp_left"] = sum(m["hp"] for m in team) / len(team)
            if sum(m["tp"] for m in team) / len(team) < 0.9 or sum(m["hp"] for m in team) / len(team) < 0.9:
                _need(srec, team)
                _rest(team)                                       # towns with an inn appear in every stage half
                rec["rests"] += 1
                srec["rests"] += 1
        rec["final_team"] = [(m["species"], m["level"]) for m in team]
    return rec


def run(ctx):
    sec = C.Section()
    smp = ctx.targets["sample"]
    runs = max(ctx.min_runs, int(smp["tp_runs"] * ctx.scale))
    jobs = []
    for band in ctx.bands:
        for i in range(runs):
            jobs.append((STARTERS[i % 3], band, i, 0 if i % 3 == 2 and i >= 3 else 1))
    recs = C.pmap(run_one, jobs, ctx, "playthroughs")
    sec.battles = sum(r["battles"] for r in recs)
    ctx.playthroughs = recs
    tp, lc, flags = [], [], []
    tgt_b = ctx.targets["flags"]["tp_battles_per_rest"]
    tgt_g = ctx.targets["flags"]["tp_gym_arrival"]
    min_tps = []
    for band in ctx.bands:
        rs = [r for r in recs if r["band"] == band]
        for si in range(18):
            sts = [r["stages"][si] for r in rs if si in r["stages"] and r["stages"][si]["battles"]]
            fg = [f for r in rs if r["grind"] for f in r["fights"] if f["stage"] == si]
            fg0 = [f for r in rs if not r["grind"] for f in r["fights"] if f["stage"] == si]
            if not sts:
                continue
            b = sum(s["battles"] for s in sts)
            rest = sum(s["needed"] for s in sts)
            tpl = [s["tp_left"] for s in sts if s["tp_left"] is not None]
            tp.append({"stage": "s%02d" % si, "band": band, "battles_per_rest": round(b / max(1, rest), 2), "target": tgt_b,
                       "tp_left_at_gym": round(sum(tpl) / len(tpl), 3) if tpl else None,
                       "blackouts_per_run": round(sum(s["blackouts"] for s in sts) / len(sts), 2),
                       "rests_needed_per_run": round(rest / len(sts), 2),
                       "tp_share_of_needed": round(sum(s["needed_tp"] for s in sts) / max(1, rest), 2),
                       "n": len(sts)})
            if fg:
                lead = [f["median"] for f in fg]
                lc.append({"stage": "s%02d" % si, "band": band, "player_level": round(statistics.mean(f["lead"] for f in fg), 1), "party_level": round(statistics.mean(lead), 1),
                           "bench_level": round(statistics.mean(f["bench"] for f in fg if f.get("bench") is not None), 1) if any(f.get("bench") is not None for f in fg) else None,
                           "foe_level": round(statistics.mean(f["foe_ace"] for f in fg), 1),
                           "exp_level": round(statistics.mean(f["median"] for f in fg0), 1) if fg0 else None,
                           "lead_level": round(statistics.mean(f["lead"] for f in fg), 1), "n": len(fg)})
            min_tps += [f["min_tp"] for f in fg if f.get("min_tp") is not None]
    sec.data["tp"], sec.data["level_curve"] = tp, lc
    from collections import Counter
    stuck_at = Counter(x for r in recs for x in r.get("stuck_at", []))
    sec.data["playthrough"] = {"stuck_fights": stuck_at.most_common(12), "runs": len(recs), "stuck_runs": sum(1 for r in recs if r["stuck"]),
                               "stuck_by_band": {b: sum(r["stuck"] for r in recs if r["band"] == b) for b in ctx.bands}}
    # --- flags
    if min_tps and statistics.mean(min_tps) > 70 and len(min_tps) > 20:
        flags.append(C.flag("tp:never_matters", "tp", "red", 2.5, "TP never matters",
                            "The lowest TP reached in a fight averages %.0f of 100 over %d fights; no fight comes near running dry, so TP costs never constrain move choice (FINDINGS s1)." % (statistics.mean(min_tps), len(min_tps)),
                            metric={"name": "mean_min_tp", "value": round(statistics.mean(min_tps), 1), "ci": None, "target": [10, 60]},
                            suggestion="Raise move TP costs or lower regen (tools/balance/tp_model.json)"))
    for row in tp:
        if row["n"] >= 3 and row["battles_per_rest"] > tgt_b[1] * 1.6:
            flags.append(C.flag("tp:%s:%s" % (row["stage"], row["band"]), "tp", "amber", 0.8,
                                "Rest almost never needed at %s" % row["stage"],
                                "%.1f battles per rest for %s players (target %d-%d)." % (row["battles_per_rest"], row["band"], tgt_b[0], tgt_b[1]),
                                stage=row["stage"], band=row["band"],
                                metric={"name": "battles_per_rest", "value": row["battles_per_rest"], "ci": None, "target": tgt_b}))
        elif row["n"] >= 3 and row["battles_per_rest"] < tgt_b[0] * 0.6:
            flags.append(C.flag("tp:%s:%s" % (row["stage"], row["band"]), "tp", "amber", 1.0,
                                "Rest needed constantly at %s" % row["stage"],
                                "%.1f battles per rest for %s players (target %d-%d). %.0f%% of the needed rests were TP-driven, the rest HP or blackouts." % (
                                    row["battles_per_rest"], row["band"], tgt_b[0], tgt_b[1], 100 * row["tp_share_of_needed"]),
                                stage=row["stage"], band=row["band"],
                                metric={"name": "battles_per_rest", "value": row["battles_per_rest"], "ci": None, "target": tgt_b}))
    for r in lc:
        d = r["player_level"] - r["foe_level"]
        fl = ctx.targets["flags"]
        if r["band"] == "skilled" and d > fl["lead_delta_over"] and r["n"] >= 5:
            flags.append(C.flag("level:%s:skilled:over" % r["stage"], "level_curve", "amber", 1.0, "Skilled players are over-levelled at %s" % r["stage"],
                                "Median party level %.1f vs foe ace %.1f (+%.1f; flag above +%d)." % (r["player_level"], r["foe_level"], d, fl["lead_delta_over"]),
                                stage=r["stage"], band="skilled", metric={"name": "lead_delta", "value": round(d, 2), "ci": None, "target": [fl["lead_delta_under"], fl["lead_delta_over"]]}))
        if r["band"] == "average" and d < fl["lead_delta_under"] and r["n"] >= 5:
            flags.append(C.flag("level:%s:average:under" % r["stage"], "level_curve", "amber", 1.2, "Grind wall for average players at %s" % r["stage"],
                                "Median party level %.1f vs foe ace %.1f (%.1f; flag below %d)." % (r["player_level"], r["foe_level"], d, fl["lead_delta_under"]),
                                stage=r["stage"], band="average", metric={"name": "lead_delta", "value": round(d, 2), "ci": None, "target": [fl["lead_delta_under"], fl["lead_delta_over"]]}))
    real_stuck = {r_id for r_id in stuck_at if ctx.fight_by_id.get(r_id, {}).get("source") not in (None, "placeholder")}
    stuck = sum(1 for r in recs if any(x in real_stuck for x in r.get("stuck_at", [])))
    if stuck:
        flags.append(C.flag("level:stuck", "level_curve", "red" if stuck > len(recs) * 0.3 else "amber", 2.0,
                            "%d of %d simulated playthroughs got stuck on a designed fight (%s)" % (stuck, len(recs), ", ".join(sorted(real_stuck))),
                            "A fight was lost %d times in a row (stuck fights: %s). Generated placeholder fights are ignored here." % (MAX_TRIES, ", ".join("%s x%d" % kv for kv in stuck_at.most_common() if kv[0] in real_stuck)),
                            metric={"name": "stuck_runs", "value": stuck, "ci": None, "target": [0, 0]}))
    sec.flags = flags
    return sec
