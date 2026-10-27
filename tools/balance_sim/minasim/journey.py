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
  {give: ITEM, n: N, take: P}                                      a free item (Okido's balls, the Kit, the vendor sample, quest Potions)
  {shop: TOWN}                                                     the Nibi Market: the player spends money on Potions
  {museum: true, take: P}                                          the 50 yen museum ticket, paid by a share of the players
`RIVAL_STARTER` in a team is the starter that beats the player's, evolved for its level; `KEN_STARTER` the leftover one.

Where the repo has the data, it wins over the scenario: a fight whose `fight:` id (or `trainer:` const, a list of three
for the by-starter pairs) is a TRAINER_ const in src/data/trainers.party plays that party; a `wild:` step with
`map: MAP_X` plays that map's land table from src/data/wild_encounters.json. The scenario's `team`/`table` is the fallback
while a party is still the scaffold stub (one Lv 2 Rattata) or the map has no entry, and is listed as `yaml` in the gate.
`model: legacy` (or --model legacy) replays the pre-economy lab for parity checks: inns restock Potions, no money, no
status-move policy, and steps marked `new: true` are skipped.
"""
import argparse
import json
import os
import random
import re
import statistics
from collections import Counter, defaultdict

import yaml

from . import arena as ARENA, economy as ECON, policies, sigdata as SIG, stats
from .fights import RIVAL_OF, STARTERS, parse_mon
from .gamedata import Game, ROOT, SIM_ROOT
from .policies import move_value, stat
from .signals import _common as C

BAND = {
    # catch: chance to catch a new species met while the party has room; train_bench: chance a wild battle is led by
    # the lowest-level healthy member (how catches get levels); grind_gap: lead level the player grinds to
    # before a leader (ace - gap, as targets.yaml level_offset), at most grind_max wild battles; rest_hp/rest_tp: party
    # averages under which they walk back to the inn.
    # potions: the legacy bag (restocked at every inn); shop_*: what the player tops up to at the Nibi Market (slice model, PLACEHOLDER habits)
    "average": {"catch": 0.6, "train_bench": 0.35, "grind_gap": 1, "grind_max": 15, "rest_hp": 0.40, "rest_tp": 0.20, "potions": 3, "smart_moves": False,
                "shop_potions": 4, "shop_balls": 3, "shop_antidote": 1},
    "skilled": {"catch": 0.9, "train_bench": 0.5, "grind_gap": 0, "grind_max": 20, "rest_hp": 0.50, "rest_tp": 0.30, "potions": 5,
                "smart_moves": os.environ.get("MINASIM_SMART", "1") == "1", "shop_potions": 6, "shop_balls": 5, "shop_antidote": 2},
}
GYM_AI = C.GYM_AI
MAX_TRIES = 4
KEN_OF = {"BULBASAUR": "SQUIRTLE", "CHARMANDER": "BULBASAUR", "SQUIRTLE": "CHARMANDER"}   # the leftover ball (plan 4.3 #13)
TOKEN_STARTER = {"RIVAL_STARTER": RIVAL_OF, "KEN_STARTER": KEN_OF}
STUB = ("RATTATA", 2)             # the scaffold's placeholder party (slice-plan 5.3): not a real party yet
HEAL_ITEMS = ("POTION", "SUPER_POTION", "HYPER_POTION")


# ------------------------------------------------------------------ scenario
def load(path, model=None, wild="json"):
    """Reads a scenario. `model` overrides its `model:` key (slice|legacy); `wild` is json (the repo's table, YAML as the
    fallback) or yaml (the scenario's own tables)."""
    if not os.path.isabs(path) and not os.path.exists(path):
        path = os.path.join(SIM_ROOT, path)
    with open(path, encoding="utf-8") as f:
        sc = yaml.safe_load(f)
    if model:
        sc["model"] = model
    sc.setdefault("model", "slice")
    g = Game.get()
    if sc["model"] == "legacy":
        sc["steps"] = [st for st in sc["steps"] if not st.get("new")]
    sc["_sources"] = {}
    for st in sc["steps"]:
        if st.get("slice_no_whiteout") and sc["model"] != "legacy":
            st["no_whiteout"] = True
        if "wild" in st or "grind" in st:
            rows, src = None, "yaml"
            if wild == "json" and st.get("map"):
                rows = wild_rows_json(g, st["map"])
                src = "json" if rows else "yaml"
            st["_table"] = rows or [_wild_row(g, w) for w in st.get("table", [])]
            st["_source"] = src
            if "wild" in st:
                sc["_sources"]["wild:" + st["wild"]] = src
        if "fight" in st:
            st["_party"] = []
            for s_ in st.get("team", []):
                if s_.split(":")[0].strip().upper() in TOKEN_STARTER:
                    lv = int(re.search(r":(\d+)", s_).group(1))
                    st["_party"].append({"species": s_.split(":")[0].strip().upper(), "level": lv, "moves": None, "item": None})
                else:
                    m = parse_mon(g, s_)
                    if m is None:
                        raise ValueError("unknown species in %r" % s_)
                    st["_party"].append(m)
            st["_by_starter"] = resolve_trainer(g, st)
            sc["_sources"][st["fight"]] = {s_: v["source"] for s_, v in st["_by_starter"].items()}
        if "gift" in st:
            st["gift"] = g.try_resolve(st["gift"]) or st["gift"]
    return sc


def wild_rows_json(g, map_const):
    """[(species, lo, hi, weight)] of a map's land table in src/data/wild_encounters.json (slot rates from the file); None if absent."""
    with open(os.path.join(ROOT, "src/data/wild_encounters.json"), encoding="utf-8") as f:
        d = json.load(f)
    for grp in d["wild_encounter_groups"]:
        rates = next((x["encounter_rates"] for x in grp.get("fields", []) if x["type"] == "land_mons"), None)
        for e in grp.get("encounters", []):
            if e["map"] == map_const and e.get("land_mons") and rates:
                rows = Counter()
                for rate, m in zip(rates, e["land_mons"]["mons"]):
                    sp = g.try_resolve(m["species"].replace("SPECIES_", ""))
                    if sp:
                        rows[(sp, m["min_level"], m["max_level"])] += rate
                return [(sp, lo, hi, float(w)) for (sp, lo, hi), w in rows.items()] or None
    return None


def is_stub(party):
    return len(party) == 1 and (party[0]["species"], party[0]["level"]) == STUB


def resolve_trainer(g, st):
    """{starter: {"party", "source", "class", "ai", "tid"}}: the party each starter faces, from trainers.party when it is a real one."""
    ids = st.get("trainer") or st["fight"]
    ids = ids if isinstance(ids, list) else [ids] * len(STARTERS)
    out = {}
    for starter, tid in zip(STARTERS, ids):
        t = g.party_trainers.get(tid) if str(tid).startswith("TRAINER_") else None
        if t and not is_stub(t["party"]):
            out[starter] = {"party": [dict(m) for m in t["party"]], "source": "party", "class": t["class"], "ai": t.get("ai") or None, "tid": tid}
        elif st.get("_party"):
            out[starter] = {"party": None, "source": "yaml", "class": (t or {}).get("class") or st.get("trainer_class"), "ai": (t or {}).get("ai") or None,
                            "tid": tid if t else None}
        else:
            raise ValueError("fight %s: %s is a stub and the scenario has no team to fall back on" % (st["fight"], tid))
    return out


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
def foe_party(g, st, starter):
    """The party a fight puts against `starter`: trainers.party's when it is real, else the scenario's (tokens resolved)."""
    info = st["_by_starter"][starter]
    out = []
    for m in info["party"] or st["_party"]:
        m = dict(m)
        tok = TOKEN_STARTER.get(m["species"])
        if tok:
            m["species"] = g.evolve_to(tok[starter], m["level"])
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


def policy_name(band, model, signature, no_status=False):
    """Engine policy for a band: legacy average never uses status moves; the signature check lets every band use a Signature."""
    if band == "average" and signature:
        return "average_sig"
    if band == "average" and no_status:
        return "average_nostatus"
    return band + "_legacy" if model == "legacy" else band


def battle(g, team, bag, foes, band, seed, cls, arena=0, ai=None, model="slice", signature=False, no_status=False):
    alive = [m for m in team if m["hp"] > 0]
    order = alive + [m for m in team if m["hp"] <= 0]
    mons = C.make_mons([{"species": m["species"], "level": m["level"], "moves": m["moves"], "item": None, "awakened": m.get("awakened")} for m in order],
                       tp_spent=[int(round((1 - m["tp"]) * 100)) for m in order],
                       hp=[int(round(m["hp"] * m["max_hp"])) for m in order],
                       exps=[m["exp"] for m in order])
    kw = {}
    if cls != "wild":
        kw = {"is_trainer": True, "ai_flags": ai or (GYM_AI if cls in ("gym", "rival") else ["CHECK_BAD_MOVE"]),
              "trainer_class": {"gym": "LEADER", "rival": "RIVAL"}.get(cls, "YOUNGSTER")}
    policies.ARENA["id"] = arena
    res = C.battle(None, seed=seed, player=mons, foe=C.make_mons(foes), player_policy=policy_name(band, model, signature, no_status), foe_policy="rom",
                   bag={k: v for k, v in bag.items() if k in HEAL_ITEMS}, max_turns=200, arena=arena, **kw)
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
    legacy = sc["model"] == "legacy"
    sigs = SIG.base_species() if sc.get("signature") == "awake" else set()
    knobs = ARENA.apply(C.engine(), sc.get("arena_tuning"))
    policies.ARENA.update(knobs=knobs, blockers=ARENA.blockers(knobs), average_use=sc.get("arena_use", 0.5))
    P = dict(BAND[band], **(sc.get("bands", {}).get(band, {})))
    rng = random.Random(seed)
    erng = random.Random(seed ^ 0x5EC0DE)                   # the economy's own stream: it never moves the battle stream
    team = [new_mon(g, starter, sc.get("starter_level", 5))]
    bag = {"POTION": P["potions"]} if legacy else {"POTION": 0, "POKE_BALL": 0, "ANTIDOTE": 0}
    money = [0 if legacy else ECON.start_money()]
    size = 1
    rec = {"starter": starter, "band": band, "fights": [], "rests": [], "blackouts": 0, "battles": 0, "wild": 0, "catches": [],
           "tp_rests": {}, "money_at": {}, "potions_used": defaultdict(int), "balls_at_nibi": None, "balls_end": 0, "money_end": 0,
           "status_turns": 0, "turns": 0,
           "segments": defaultdict(lambda: {"battles": 0, "rests": 0, "hp_rests": 0, "tp_rests": 0, "potions": 0})}
    town = sc.get("start", "home")
    seg = "start"
    k = [0]
    tp_total = [0]
    last_gym_tp = [0]

    def mark_sig(m):
        m["awakened"] = bool(sigs) and g.base_form(m["species"]) in sigs
        return m

    if sigs:
        mark_sig(team[0])

    def rest(reason):
        for m in team:
            m["hp"], m["tp"] = 1.0, 1.0
        if legacy:
            bag["POTION"] = max(bag.get("POTION", 0), P["potions"])
        elif reason == "blackout":
            money[0] -= int(money[0] * ECON.WHITEOUT_LOSS)
        s = rec["segments"][seg]
        s["rests"] += 1
        s["hp_rests"] += reason == "hp"
        s["tp_rests"] += reason == "tp"
        tp_total[0] += reason == "tp"
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

    def fight(foes, cls, fid=None, no_whiteout=False, arena=0, ai=None, tclass=None):
        won, tries = False, 0
        for tries in range(1, MAX_TRIES + 1):
            maybe_walk_back()
            if tries == 1 and cls == "gym":
                rec["tp_rests"][fid] = tp_total[0] - last_gym_tp[0]
                last_gym_tp[0] = tp_total[0]
                rec["money_at"][fid] = money[0]
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
            res = battle(g, team, bag, foes, band, seed * 7919 + k[0], cls, arena, ai, sc["model"], bool(sigs), bool(sc.get("no_status")))
            rec["battles"] += 1
            rec["segments"][seg]["battles"] += 1
            used = sum(n for it, n in (res.get("items_used") or {}).items() if it in HEAL_ITEMS)
            rec["potions_used"][seg] += used
            rec["segments"][seg]["potions"] += used
            if band == "average":
                mu = res.get("moves_used") or {}
                rec["turns"] += sum(mu.values())
                rec["status_turns"] += sum(n for mv, n in mu.items() if g.moves.get(mv, {}).get("power", 1) == 0)
            won = res["winner"] == 0
            if fid and tries == 1:
                rec["fights"].append({"id": fid, "won": won, "lead": max(l for _s, l in before), "party": before, "turns": res["turns"],
                                      "hp_after": sum(m["hp"] for m in team) / len(team), "tp_after": sum(m["tp"] for m in team) / len(team)})
            if cls == "wild":
                if not won and all(m["hp"] <= 0 for m in team):
                    rec["blackouts"] += 1
                    rest("blackout")
                return won
            if won and not legacy:
                money[0] += ECON.prize(tclass or ECON.TRAINER_CLASS_DEFAULT.get(cls, "Youngster"), foes[-1]["level"])
            if won:
                return won
            if no_whiteout:
                if not legacy:                      # R/G-style join: HP and status come back, TP does not (slice-plan 3.4, Q96)
                    for m in team:
                        m["hp"] = 1.0
                return won
            rec["blackouts"] += 1
            rest("blackout")
        return won

    ctx = {"bag": bag, "erng": erng, "legacy": legacy, "mark": mark_sig}
    for i, st in enumerate(sc["steps"]):
        if "segment" in st:
            seg = st["segment"]
            if seg == "Nibi" and rec["balls_at_nibi"] is None:
                rec["balls_at_nibi"] = bag.get("POKE_BALL", 0)
        if "town" in st:
            town = st["town"]
        if "inn" in st:
            town = st["inn"]
            rest("inn")
        if "catch_slot" in st:
            size = st["catch_slot"]
        if "gift" in st and len(team) < 6 and rng.random() < st.get("take", 1.0):
            team.append(mark_sig(new_mon(g, st["gift"], st["level"], st.get("moves"))))
            rec["catches"].append(st["gift"])
        elif "gift" in st and st.get("sell") and not legacy:
            money[0] += ECON.SANDSHREW_SALE                      # Omura's offer (plan 4.2 row 6): the optional branch
        if "give" in st and not legacy and erng.random() < st.get("take", 1.0):
            bag[st["give"]] = bag.get(st["give"], 0) + st.get("n", 1)
        if "shop" in st and not legacy:
            _shop(bag, money, P)
        if "museum" in st and not legacy and erng.random() < st.get("take", 1.0):
            money[0] -= min(money[0], ECON.MUSEUM_TICKET)
        if "wild" in st or "grind" in st:
            table = st.get("_table") or last_table(sc, i)
            n = st.get("battles", 0)
            if "grind" in st:
                nxt = next((s_ for s_ in sc["steps"][i:] if s_.get("class") == "gym"), None)
                ace = max(m["level"] for m in foe_party(g, nxt, starter)) if nxt else 0
                target = st["until"] if isinstance(st.get("until"), int) else ace - P["grind_gap"]
                n = 0
                while max(m["level"] for m in team) < target and n < P["grind_max"]:
                    n += 1
                    _wild_battle(g, team, table, rng, size, P, rec, fight, ctx)
                continue
            for _ in range(n):
                _wild_battle(g, team, table, rng, size, P, rec, fight, ctx)
        if "fight" in st:
            foes = foe_party(g, st, starter)
            info = st["_by_starter"][starter]
            ar = 0 if sc.get("no_arena") else ARENA.ARENAS[st.get("arena", "NONE")]
            fight(foes, st.get("class", "trainer"), st["fight"], st.get("no_whiteout", False), ar, info["ai"], info["class"] or st.get("trainer_class"))
    rec["segments"] = dict(rec["segments"])
    rec["potions_used"] = dict(rec["potions_used"])
    rec["final"] = [(m["species"], m["level"]) for m in team]
    rec["money_end"], rec["balls_end"] = money[0], bag.get("POKE_BALL", 0)
    return rec


def _shop(bag, money, P):
    """The Nibi Market (plan 3.2): Potions up to the band's habit first, then Poke Balls, then one Antidote; prices from items.h."""
    for item, want in (("POTION", P["shop_potions"]), ("POKE_BALL", P["shop_balls"]), ("ANTIDOTE", P["shop_antidote"])):
        price = ECON.item_price(item)
        n = min(max(0, want - bag.get(item, 0)), money[0] // price if price else 0)
        bag[item] = bag.get(item, 0) + n
        money[0] -= n * price


def _wild_battle(g, team, table, rng, size, P, rec, fight, ctx):
    sp, lo, hi, _w = rng.choices(table, [t[3] for t in table])[0]
    lv = rng.randint(lo, hi)
    rec["wild"] += 1
    have = {g.base_form(m["species"]) for m in team}
    if len(team) < size and g.base_form(sp) not in have and rng.random() < P["catch"]:
        if ctx["legacy"]:
            team.append(new_mon(g, sp, lv))                   # caught (the catch fight itself is not simulated)
            rec["catches"].append(sp)
            return
        used, ok = ECON.balls_to_catch(ctx["erng"], ctx["bag"].get("POKE_BALL", 0))
        ctx["bag"]["POKE_BALL"] = ctx["bag"].get("POKE_BALL", 0) - used
        if ok:
            team.append(ctx["mark"](new_mon(g, sp, lv)))
            rec["catches"].append(sp)
            return                                            # no balls (or all thrown in vain): it is fought for EXP instead
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
        key = "%s/%s" % (band, starter)
        out["parties"][key] = {
            "blackouts": round(statistics.mean(r["blackouts"] for r in rs), 2),
            "battles": round(statistics.mean(r["battles"] for r in rs), 1),
            "final_levels": round(statistics.mean(max(l for _s, l in r["final"]) for r in rs), 1),
            "species": Counter(g for r in rs for g, _l in r["final"][1:]).most_common(8)}
    # report-only economy metrics (plan 7.3): TP rests before each gym, Potions per segment, money at each gym, balls at Nibi
    gyms = [st["fight"] for st in sc["steps"] if st.get("class") == "gym"]
    out["metrics"] = {}
    for (band, starter), rs in sorted(by.items()):
        n = len(rs)
        out["metrics"]["%s/%s" % (band, starter)] = {
            "tp_rests": {f: round(sum(r["tp_rests"].get(f, 0) for r in rs) / n, 2) for f in gyms},
            "tp_rest_share": {f: round(sum(r["tp_rests"].get(f, 0) >= 1 for r in rs) / n, 2) for f in gyms},
            "money_at": {f: round(sum(r["money_at"].get(f, 0) for r in rs) / n) for f in gyms},
            "potions_used": {s_: round(sum(r["potions_used"].get(s_, 0) for r in rs) / n, 2) for s_ in segs},
            "balls_at_nibi": round(sum((r["balls_at_nibi"] or 0) for r in rs) / n, 2),
            "balls_end": round(sum(r["balls_end"] for r in rs) / n, 2),
            "money_end": round(sum(r["money_end"] for r in rs) / n),
            "status_share": round(sum(r["status_turns"] for r in rs) / max(1, sum(r["turns"] for r in rs)), 3) if band == "average" else None}
    out["sources"] = sc.get("_sources", {})
    out["model"] = sc.get("model")
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


def simulate(sc, runs, bands=("average", "skilled"), starters=STARTERS, jobs=None):
    """All runs of a scenario: one per (band, starter, i); seeds depend only on those, so variants pair up."""
    jobs = jobs or os.cpu_count() or 1
    todo = [(sc, s, b, C.seed_of("journey", s, b, i)) for b in bands for s in starters for i in range(runs)]
    if jobs > 1:
        import multiprocessing as mp
        with mp.get_context("fork").Pool(jobs) as pool:
            return pool.map(run_one, todo, chunksize=8)
    return [run_one(j) for j in todo]


def add_scenario_args(p):
    p.add_argument("--model", choices=["slice", "legacy"], help="legacy replays the lab before the economy (parity runs); default: the scenario's")
    p.add_argument("--wild", choices=["json", "yaml"], default="json", help="wild tables: src/data/wild_encounters.json (YAML fallback) or the scenario's own")
    p.add_argument("--signature", choices=["off", "awake"], default="off", help="awaken every Signature line in the player's party (signature check)")
    p.add_argument("--arena", action="append", metavar="KNOB=VALUE", help="override an arena knob (minasim/arena.py)")
    p.add_argument("--no-arena", action="store_true", help="fight every gym without its arena")
    p.add_argument("--arena-use", type=float, default=0.5, help="chance per turn that an average player plays the arena")


def prepare(a, scenario=None):
    sc = load(scenario or a.scenario, model=a.model, wild=a.wild)
    sc["arena_tuning"] = dict(sc.get("arena_tuning") or {}, **ARENA.parse_overrides(a.arena))
    sc["no_arena"] = a.no_arena
    sc["arena_use"] = a.arena_use
    sc["signature"] = a.signature
    return sc


def main(argv=None):
    p = argparse.ArgumentParser(prog="minasim journey")
    p.add_argument("scenario")
    p.add_argument("--runs", type=int, default=200, help="runs per starter and band")
    p.add_argument("--bands", default="average,skilled")
    p.add_argument("--starters", default=",".join(STARTERS))
    p.add_argument("--jobs", type=int, default=os.cpu_count())
    p.add_argument("--json")
    add_scenario_args(p)
    a = p.parse_args(argv)
    sc = prepare(a)
    recs = simulate(sc, a.runs, a.bands.split(","), a.starters.split(","), a.jobs)
    s = summarise(sc, recs)
    print_summary(s)
    if a.json:
        with open(a.json, "w") as f:
            json.dump(s, f, indent=1)
    return s


if __name__ == "__main__":
    main()
