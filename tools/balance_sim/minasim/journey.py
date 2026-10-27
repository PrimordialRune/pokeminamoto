"""Scripted journeys: one chapter of the game played battle after battle, exactly as a scenario file lays it out.

    python3 -m minasim journey scenarios/masara_nibi.yaml [--runs 300] [--bands average,skilled] [--json out.json]

The stage report (`run`) places fights by map and badge; a journey instead follows a designed route step by step
(wild grass, catches, gifts, trainers, the rival, inns, the leader), so a chapter's encounter tables, learnsets and
trainer parties can be tested as one piece. HP, TP, EXP, levels and evolutions carry between battles through the
real engine. A run is one starter, one band and one seed; the summary answers:

  - first-try win rate of every fight, per starter and band (and the levels the player arrived with)
  - how often the player has to walk back to an inn, and whether HP or TP sent them there
  - blackouts, battles per rest, and which species the party ended up with

Scenario format (see scenarios/masara_nibi.yaml): `steps` is a list of
  {wild: AREA, battles: N, table: ["SPECIES:LO-HI:WEIGHT", ...]}   random encounters (catches happen here)
  {catch_slot: N}                                                  party size the player fills up to from now on
  {gift: SPECIES, level: L, take: 0.0-1.0, moves: [...]}           a gift/event Pokemon, taken with probability `take`
  {fight: ID, name: ..., class: trainer|rival|gym, team: ["SPECIES:LV[MOVE/MOVE]@ITEM", ...], no_whiteout: bool}
  {grind: AREA, until: LEVEL}                                     grind the last table until the lead reaches LEVEL (default: next leader ace - grind_gap)
  {inn: TOWN}                                                      rest (also restocks the bag)
  {town: TOWN}                                                     where a walk back to rest goes from here on
`RIVAL_STARTER` in a team is the starter that beats the player's, evolved for its level.
"""
import argparse
import json
import os
import random
import re
import statistics
from collections import Counter, defaultdict

import yaml

from . import arena as ARENA, policies, stats
from .fights import RIVAL_OF, STARTERS, parse_mon
from .gamedata import Game, SIM_ROOT
from .policies import move_value, stat
from .signals import _common as C

BAND = {
    # catch: chance to catch a new species met while the party has room; train_bench: chance a wild battle is led by
    # the lowest-level healthy member (how catches get levels); grind_gap: lead level the player grinds to
    # before a leader (ace - gap, as targets.yaml level_offset), at most grind_max wild battles; rest_hp/rest_tp: party
    # averages under which they walk back to the inn.
    "average": {"catch": 0.6, "train_bench": 0.35, "grind_gap": 1, "grind_max": 15, "rest_hp": 0.40, "rest_tp": 0.20, "potions": 3, "smart_moves": False},
    "skilled": {"catch": 0.9, "train_bench": 0.5, "grind_gap": 0, "grind_max": 20, "rest_hp": 0.50, "rest_tp": 0.30, "potions": 5, "smart_moves": os.environ.get("MINASIM_SMART", "1") == "1"},
}
GYM_AI = C.GYM_AI
MAX_TRIES = 4


# ------------------------------------------------------------------ scenario
def load(path):
    if not os.path.isabs(path) and not os.path.exists(path):
        path = os.path.join(SIM_ROOT, path)
    with open(path, encoding="utf-8") as f:
        sc = yaml.safe_load(f)
    g = Game.get()
    for st in sc["steps"]:
        if "wild" in st or "grind" in st:
            st["_table"] = [_wild_row(g, w) for w in st.get("table", [])]
        if "fight" in st:
            st["_party"] = []
            for s in st["team"]:
                if s.split(":")[0].strip().upper() == "RIVAL_STARTER":
                    lv = int(re.search(r":(\d+)", s).group(1))
                    st["_party"].append({"species": "RIVAL_STARTER", "level": lv, "moves": None, "item": None})
                else:
                    m = parse_mon(g, s)
                    if m is None:
                        raise ValueError("unknown species in %r" % s)
                    st["_party"].append(m)
        if "gift" in st:
            st["gift"] = g.try_resolve(st["gift"]) or st["gift"]
    return sc


def _wild_row(g, spec):
    sp, lv, wt = spec.split(":")
    lo, _, hi = lv.partition("-")
    s = g.try_resolve(sp)
    if s is None:
        raise ValueError("unknown species %r" % sp)
    return (s, int(lo), int(hi or lo), float(wt))


def last_table(sc, upto):
    for st in reversed(sc["steps"][:upto]):
        if st.get("_table"):
            return st["_table"]
    return []


# ------------------------------------------------------------------ the party
def new_mon(g, species, level, moves=None, friendship=None):
    return {"species": species, "level": level, "exp": g.exp_for_level(g.species[species]["growth"], level), "moves": moves,
            "hp": 1.0, "tp": 1.0, "max_hp": stat(g, species, level, "hp"), "caught_at": level}


def known_moves(g, species, level):
    out = []
    for lv, mv in g.learnsets.get(species, []):
        if lv <= level and mv not in out:
            out.append(mv)
    return out


def smart_moves(g, mon, foes):
    """A skilled player's four: the best damage per TP against the next foes (up to 3 attacks), plus one status move."""
    known = known_moves(g, mon["species"], mon["level"])
    if not known:
        return None
    me = {"species": mon["species"], "level": mon["level"], "max_hp": 1, "hp": 1}
    defs = [{"species": f["species"], "level": f["level"], "max_hp": stat(g, f["species"], f["level"], "hp"), "hp": 1} for f in foes] or [me]
    atk = [m for m in known if g.moves[m]["power"] > 0]
    chosen, best = [], [0.0] * len(defs)
    for _ in range(min(3, len(atk))):
        def gain(mv):
            return sum(max(b, move_value(g, me, d, mv)) - b for b, d in zip(best, defs)) / (1.0 + g.tp_cost(mv) / 40.0) + 1e-6 * g.moves[mv]["power"]
        mv = max((m for m in atk if m not in chosen), key=gain)
        chosen.append(mv)
        best = [max(b, move_value(g, me, d, mv)) for b, d in zip(best, defs)]
    good_status = ("SLEEP", "PARALYZE", "LEECH_SEED", "TOXIC", "POISON", "CONFUSE", "ACCURACY_DOWN", "ATTACK_UP_2", "DEFENSE_UP")
    st = [m for m in known if g.moves[m]["power"] == 0 and any(g.moves[m]["effect"].startswith(x) for x in good_status)]
    if st:
        chosen.append(st[-1])
    for m in reversed(known):                      # fill with the newest moves
        if len(chosen) >= 4:
            break
        if m not in chosen:
            chosen.append(m)
    return chosen[:4]


# ------------------------------------------------------------------ one battle
def foe_party(g, party, starter):
    out = []
    for m in party:
        m = dict(m)
        if m["species"] == "RIVAL_STARTER":
            m["species"] = g.evolve_to(RIVAL_OF[starter], m["level"])
        out.append(m)
    return out


def order_lead(g, team, foe, rng, prob):
    if len(team) < 2 or rng.random() > prob:
        return
    fo = {"species": foe["species"], "level": foe["level"], "max_hp": stat(g, foe["species"], foe["level"], "hp"), "hp": 1}
    fmoves = foe.get("moves") or g.level_moves(foe["species"], foe["level"]) or ["TACKLE"]

    def score(m):
        if m["hp"] < 0.35:
            return -9
        me = {"species": m["species"], "level": m["level"], "max_hp": m["max_hp"], "hp": m["max_hp"]}
        mine = max(move_value(g, me, fo, mv) for mv in (m["moves"] or g.level_moves(m["species"], m["level"]) or ["TACKLE"]))
        theirs = max(move_value(g, fo, me, mv) for mv in fmoves)
        return mine - 0.8 * theirs + min(0, m["level"] - foe["level"] + 6) * 0.05
    team.sort(key=score, reverse=True)


def battle(g, team, bag, foes, band, seed, cls, arena=0):
    alive = [m for m in team if m["hp"] > 0]
    order = alive + [m for m in team if m["hp"] <= 0]
    mons = C.make_mons([{"species": m["species"], "level": m["level"], "moves": m["moves"], "item": None} for m in order],
                       tp_spent=[int(round((1 - m["tp"]) * 100)) for m in order],
                       hp=[int(round(m["hp"] * m["max_hp"])) for m in order],
                       exps=[m["exp"] for m in order])
    kw = {}
    if cls != "wild":
        kw = {"is_trainer": True, "ai_flags": GYM_AI if cls in ("gym", "rival") else ["CHECK_BAD_MOVE"],
              "trainer_class": {"gym": "LEADER", "rival": "RIVAL"}.get(cls, "YOUNGSTER")}
    policies.ARENA["id"] = arena
    res = C.battle(None, seed=seed, player=mons, foe=C.make_mons(foes), player_policy=band, foe_policy="rom", bag=dict(bag),
                   max_turns=200, arena=arena, **kw)
    for m, r in zip(order, res["player"]):
        m["hp"] = max(0, r["hp"]) / max(1, r["max_hp"])
        m["tp"] = r["tp"] / max(1, r["tp_max"])
        if r.get("level"):
            m["level"], m["exp"] = r["level"], r.get("exp", m["exp"])
        new = g.evolve_to(m["species"], m["level"])
        if new != m["species"]:
            m["species"] = new
            if m["moves"]:
                m["moves"] = None                  # re-pick after evolving
        m["max_hp"] = stat(g, m["species"], m["level"], "hp")
    for it, n in (res.get("items_used") or {}).items():
        bag[it] = max(0, bag.get(it, 0) - n)
    return res


# ------------------------------------------------------------------ one run
def run_one(job):
    sc, starter, band, seed = job
    g = Game.get()
    knobs = ARENA.apply(C.engine(), sc.get("arena_tuning"))
    policies.ARENA.update(knobs=knobs, blockers=ARENA.blockers(knobs), average_use=sc.get("arena_use", 0.5))
    P = dict(BAND[band], **(sc.get("bands", {}).get(band, {})))
    rng = random.Random(seed)
    team = [new_mon(g, starter, sc.get("starter_level", 5))]
    bag = {"POTION": P["potions"]}
    size = 1
    rec = {"starter": starter, "band": band, "fights": [], "rests": [], "blackouts": 0, "battles": 0, "wild": 0, "catches": [],
           "segments": defaultdict(lambda: {"battles": 0, "rests": 0, "hp_rests": 0, "tp_rests": 0})}
    town = sc.get("start", "home")
    seg = "start"
    k = [0]

    def rest(reason):
        for m in team:
            m["hp"], m["tp"] = 1.0, 1.0
        bag["POTION"] = max(bag.get("POTION", 0), P["potions"])
        s = rec["segments"][seg]
        s["rests"] += 1
        s["hp_rests"] += reason == "hp"
        s["tp_rests"] += reason == "tp"
        rec["rests"].append((seg, reason))

    def maybe_walk_back():
        hp = sum(m["hp"] for m in team) / len(team)
        tp = sum(m["tp"] for m in team) / len(team)
        if team[0]["hp"] <= 0 and all(m["hp"] <= 0.25 for m in team[1:]):
            rest("hp")
        elif hp < P["rest_hp"]:
            rest("hp")
        elif tp < P["rest_tp"]:
            rest("tp")

    def fight(foes, cls, fid=None, no_whiteout=False, arena=0):
        won, tries = False, 0
        for tries in range(1, MAX_TRIES + 1):
            maybe_walk_back()
            if P["smart_moves"]:
                for m in team:                      # re-picked every fight: new moves are learned along the way
                    m["moves"] = smart_moves(g, m, foes)
            order_lead(g, team, foes[0], rng, 0.7 if band == "average" else 1.0)
            if cls == "wild" and len(team) > 1 and rng.random() < P["train_bench"]:
                low = min((m for m in team if m["hp"] > 0.5), key=lambda m: m["level"], default=None)
                if low is not None:
                    team.remove(low)
                    team.insert(0, low)
            before = [(m["species"], m["level"]) for m in team]
            k[0] += 1
            res = battle(g, team, bag, foes, band, seed * 7919 + k[0], cls, arena)
            rec["battles"] += 1
            rec["segments"][seg]["battles"] += 1
            won = res["winner"] == 0
            if fid and tries == 1:
                rec["fights"].append({"id": fid, "won": won, "lead": max(l for _s, l in before), "party": before, "turns": res["turns"],
                                      "hp_after": sum(m["hp"] for m in team) / len(team), "tp_after": sum(m["tp"] for m in team) / len(team)})
            if cls == "wild":
                if not won and all(m["hp"] <= 0 for m in team):
                    rec["blackouts"] += 1
                    rest("blackout")
                return won
            if won or no_whiteout:
                return won
            rec["blackouts"] += 1
            rest("blackout")
        return won

    for i, st in enumerate(sc["steps"]):
        if "segment" in st:
            seg = st["segment"]
        if "town" in st:
            town = st["town"]
        if "inn" in st:
            town = st["inn"]
            rest("inn")
        if "catch_slot" in st:
            size = st["catch_slot"]
        if "gift" in st and len(team) < 6 and rng.random() < st.get("take", 1.0):
            team.append(new_mon(g, st["gift"], st["level"], st.get("moves")))
            rec["catches"].append(st["gift"])
        if "wild" in st or "grind" in st:
            table = st.get("_table") or last_table(sc, i)
            n = st.get("battles", 0)
            if "grind" in st:
                nxt = next((s for s in sc["steps"][i:] if s.get("class") == "gym"), None)
                ace = max(m["level"] for m in nxt["_party"]) if nxt else 0
                target = st["until"] if isinstance(st.get("until"), int) else ace - P["grind_gap"]
                n = 0
                while max(m["level"] for m in team) < target and n < P["grind_max"]:
                    n += 1
                    _wild_battle(g, team, table, rng, size, P, rec, fight)
                continue
            for _ in range(n):
                _wild_battle(g, team, table, rng, size, P, rec, fight)
        if "fight" in st:
            foes = foe_party(g, st["_party"], starter)
            ar = 0 if sc.get("no_arena") else ARENA.ARENAS[st.get("arena", "NONE")]
            fight(foes, st.get("class", "trainer"), st["fight"], st.get("no_whiteout", False), ar)
    rec["segments"] = dict(rec["segments"])
    rec["final"] = [(m["species"], m["level"]) for m in team]
    return rec


def _wild_battle(g, team, table, rng, size, P, rec, fight):
    sp, lo, hi, _w = rng.choices(table, [t[3] for t in table])[0]
    lv = rng.randint(lo, hi)
    rec["wild"] += 1
    have = {g.base_form(m["species"]) for m in team}
    if len(team) < size and g.base_form(sp) not in have and rng.random() < P["catch"]:
        team.append(new_mon(g, sp, lv))                       # caught (the catch fight itself is not simulated)
        rec["catches"].append(sp)
        return
    fight([{"species": sp, "level": lv, "moves": None, "item": None}], "wild")


# ------------------------------------------------------------------ summary
def summarise(sc, recs):
    out = {"scenario": sc.get("name"), "runs": len(recs), "fights": [], "segments": [], "parties": {}}
    by = defaultdict(list)
    for r in recs:
        by[(r["band"], r["starter"])].append(r)
    fids = [st["fight"] for st in sc["steps"] if "fight" in st]
    names = {st["fight"]: st.get("name", st["fight"]) for st in sc["steps"] if "fight" in st}
    for fid in fids:
        row = {"id": fid, "name": names[fid], "by": {}}
        for (band, starter), rs in sorted(by.items()):
            fs = [f for r in rs for f in r["fights"] if f["id"] == fid]
            if not fs:
                continue
            w = sum(f["won"] for f in fs)
            ci = stats.wilson(w, len(fs))
            row["by"]["%s/%s" % (band, starter)] = {"win": round(w / len(fs), 3), "ci": [round(ci[0], 3), round(ci[1], 3)], "n": len(fs),
                                                    "lead": round(statistics.mean(f["lead"] for f in fs), 1),
                                                    "party": round(statistics.mean(len(f["party"]) for f in fs), 2),
                                                    "turns": round(statistics.mean(f["turns"] for f in fs), 1)}
        out["fights"].append(row)
    segs = []
    for st in sc["steps"]:
        if "segment" in st and st["segment"] not in segs:
            segs.append(st["segment"])
    for s in segs:
        row = {"segment": s, "by": {}}
        for (band, starter), rs in sorted(by.items()):
            ss = [r["segments"].get(s) for r in rs if r["segments"].get(s)]
            if not ss:
                continue
            b = sum(x["battles"] for x in ss)
            hp = sum(x["hp_rests"] for x in ss)
            tp = sum(x["tp_rests"] for x in ss)
            row["by"]["%s/%s" % (band, starter)] = {"battles": round(b / len(ss), 1), "walk_backs": round((hp + tp) / len(ss), 2),
                                                    "hp_share": round(hp / max(1, hp + tp), 2),
                                                    "battles_per_rest": round(b / max(1, sum(x["rests"] for x in ss)), 1)}
        out["segments"].append(row)
    for (band, starter), rs in sorted(by.items()):
        out["parties"]["%s/%s" % (band, starter)] = {
            "blackouts": round(statistics.mean(r["blackouts"] for r in rs), 2),
            "battles": round(statistics.mean(r["battles"] for r in rs), 1),
            "final_levels": round(statistics.mean(max(l for _s, l in r["final"]) for r in rs), 1),
            "species": Counter(g for r in rs for g, _l in r["final"][1:]).most_common(8)}
    return out


def print_summary(s):
    print("%s: %d runs" % (s["scenario"], s["runs"]))
    keys = sorted({k for f in s["fights"] for k in f["by"]})
    print("\nfirst-try win rate (top party level)")
    print("%-26s" % "" + "".join("%-20s" % k for k in keys))
    for f in s["fights"]:
        print("%-26s" % f["name"][:25] + "".join(("%3d%% (L%4.1f)       " % (100 * f["by"][k]["win"], f["by"][k]["lead"])) if k in f["by"] else " " * 20 for k in keys))
    print("\nwalk-backs to an inn per segment (share caused by HP)")
    for r in s["segments"]:
        print("%-26s" % r["segment"][:25] + "".join(("%4.1f (hp %3d%%)      " % (r["by"][k]["walk_backs"], 100 * r["by"][k]["hp_share"])) if k in r["by"] else " " * 20 for k in keys))
    print("\nparty")
    for k, v in s["parties"].items():
        print("  %-20s blackouts %.2f, battles %.0f, top level %.1f, team %s" % (k, v["blackouts"], v["battles"], v["final_levels"],
                                                                             ", ".join("%s %d" % x for x in v["species"])))


def main(argv=None):
    p = argparse.ArgumentParser(prog="minasim journey")
    p.add_argument("scenario")
    p.add_argument("--runs", type=int, default=200, help="runs per starter and band")
    p.add_argument("--bands", default="average,skilled")
    p.add_argument("--starters", default=",".join(STARTERS))
    p.add_argument("--jobs", type=int, default=os.cpu_count())
    p.add_argument("--json")
    p.add_argument("--arena", action="append", metavar="KNOB=VALUE", help="override an arena knob (minasim/arena.py)")
    p.add_argument("--no-arena", action="store_true", help="fight every gym without its arena")
    p.add_argument("--arena-use", type=float, default=0.5, help="chance per turn that an average player plays the arena")
    a = p.parse_args(argv)
    sc = load(a.scenario)
    sc["arena_tuning"] = dict(sc.get("arena_tuning") or {}, **ARENA.parse_overrides(a.arena))
    sc["no_arena"] = a.no_arena
    sc["arena_use"] = a.arena_use
    jobs = [(sc, s, b, C.seed_of("journey", s, b, i)) for b in a.bands.split(",") for s in a.starters.split(",") for i in range(a.runs)]
    if a.jobs > 1:
        import multiprocessing as mp
        with mp.get_context("fork").Pool(a.jobs) as pool:
            recs = pool.map(run_one, jobs, chunksize=8)
    else:
        recs = [run_one(j) for j in jobs]
    s = summarise(sc, recs)
    print_summary(s)
    if a.json:
        with open(a.json, "w") as f:
            json.dump(s, f, indent=1)
    return s


if __name__ == "__main__":
    main()
