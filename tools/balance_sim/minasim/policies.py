"""Player and foe policies. A policy is `callable(state) -> action` (CONTRACT.md Engine API).

state = {"side", "turn", "seed", "kind": "wild|trainer",
         "me":  {"species","level","hp","max_hp","tp","tp_max","status","stages":{atk,def,spa,spd,spe,acc,eva},
                 "moves":[names], "tp_costs":[ints], "special": MOVE|None},
         "foe": {"species","level","hp","max_hp","status","stages"},
         "party": [{"species","level","hp","max_hp","tp","status","alive","active"}],
         "items": {"POTION": n}, "legal": [action dicts]}
Registered by name in POLICIES; `get_policy(name, params)` builds one. Parameters come from the policy
section of tools/balance_sim/targets.yaml (see `load_targets`).

random   uniform over legal moves                              (floor)
greedy   max expected damage, no items, switches only on faint (what ROM trainer AI roughly does)
average  an average ROM-hack player: type chart, strongest super-effective move, heals low, ~10% mistakes,
         never plans TP, never talks, no Special
skilled  1-2 ply lookahead, TP thrift, switches on bad matchups, Talk and Special when they pay
"""
import functools
import os
import random

import yaml

from .gamedata import Game, SIM_ROOT

PHYSICAL = {"NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL"}
STAGE_MUL = {-6: 2 / 8, -5: 2 / 7, -4: 2 / 6, -3: 2 / 5, -2: 2 / 4, -1: 2 / 3, 0: 1, 1: 3 / 2, 2: 2, 3: 5 / 2, 4: 3, 5: 7 / 2, 6: 4}
FIXED_DAMAGE = {"SONICBOOM": 20, "DRAGON_RAGE": 40}


@functools.lru_cache(maxsize=1)
def load_targets(path=None):
    p = path or os.path.join(SIM_ROOT, "targets.yaml")
    with open(p, encoding="utf-8") as f:
        return yaml.safe_load(f)


def policy_params(name):
    return dict(load_targets()["policies"].get(name, {}))


def stat(game, species, level, key, stage=0):
    b = game.species[species][key]
    if key == "hp":
        return (2 * b + 15) * level // 100 + level + 10
    v = (2 * b + 15) * level // 100 + 5
    return int(v * STAGE_MUL.get(max(-6, min(6, stage)), 1))


def effectiveness(game, mtype, species):
    return game.effectiveness(mtype, game.species[species]["types"])


def move_value(game, me, foe, move, stages=True):
    """Expected fraction of the foe's max HP that `move` takes off, accuracy and STAB included (no crit/roll)."""
    m = game.moves.get(move)
    if m is None:                                   # Struggle slot / empty slot
        return 0.0
    pw = m["power"]
    if pw == 0:
        fx = FIXED_DAMAGE.get(move)
        return fx / max(1, foe["max_hp"]) if fx else 0.0
    t = m["type"]
    phys = t in PHYSICAL
    ak, dk = ("atk", "def") if phys else ("spa", "spd")
    ms, fs = me.get("stages") or {}, foe.get("stages") or {}
    a = stat(game, me["species"], me["level"], ak, ms.get(ak, 0) if stages else 0)
    d = stat(game, foe["species"], foe["level"], dk, fs.get(dk, 0) if stages else 0)
    base = ((2 * me["level"] // 5 + 2) * pw * a // max(1, d)) // 50 + 2
    stab = 1.5 if t in game.species[me["species"]]["types"] else 1.0
    eff = effectiveness(game, t, foe["species"])
    acc = (m["acc"] or 100) / 100.0
    dmg = base * stab * eff * 0.925 * acc
    if m["effect"] == "EXPLOSION":                  # self-KO: only worth it when it finishes the foe
        if dmg / max(1, foe["max_hp"]) < foe.get("hp", 1) / max(1, foe["max_hp"]):
            dmg *= 0.25
    if me.get("status") == "BRN" and phys:
        dmg *= 0.5
    return dmg / max(1, foe["max_hp"])


def best_threat(game, a, b, hp_aware=False):
    """Best expected damage fraction (of b's max HP) species `a` can do to `b` with its likely moves."""
    mv = a.get("moves") or game.level_moves(a["species"], a["level"])
    return max([move_value(game, a, b, x) for x in mv] or [0.0])


def _speed(game, mon):
    return stat(game, mon["species"], mon["level"], "spe", (mon.get("stages") or {}).get("spe", 0)) // (4 if mon.get("status") == "PAR" else 1)


def _mv(me, a):
    return a.get("move") or me["moves"][a["slot"]]


def _legal(state, typ):
    return [a for a in state["legal"] if a["type"] == typ]


def _rng(state, salt=0):
    return random.Random((state.get("seed", 0) * 1000003 + state["turn"] * 131 + state["side"] * 7 + salt) & 0xFFFFFFFF)


def _moves(state):
    return [a for a in state["legal"] if a["type"] == "move"]


def _best_switch(game, state, require_hp=0.0):
    """Bench mon with the best matchup vs the foe (used on faint and by skilled on bad matchups)."""
    foe = state["foe"]
    best, score = None, -9
    for a in _legal(state, "switch"):
        m = state["party"][a["slot"]]
        if not m["alive"] or m["hp"] / m["max_hp"] < require_hp:
            continue
        me = {"species": m["species"], "level": m["level"], "max_hp": m["max_hp"], "hp": m["hp"]}
        out = best_threat(game, me, foe)
        inn = best_threat(game, foe, me)
        s = out - 1.2 * inn
        if s > score:
            best, score = a, s
    return best


def _heal_action(game, state, below):
    me = state["me"]
    if me["hp"] / me["max_hp"] >= below:
        return None
    heals = [a for a in _legal(state, "item") if game.heal_amount(a["item"]) > 0]
    if not heals:
        return None
    missing = me["max_hp"] - me["hp"]
    heals.sort(key=lambda a: game.heal_amount(a["item"]))
    for a in heals:                                   # smallest potion that covers the missing HP
        if game.heal_amount(a["item"]) >= missing:
            return a
    return heals[-1]


# ------------------------------------------------------------------ policies
def random_policy(state, params=None):
    g = Game.get()
    ms = _moves(state)
    if ms:
        return _rng(state).choice(ms)
    sw = _legal(state, "switch")
    return sw[0] if sw else state["legal"][0]


def greedy_policy(state, params=None):
    g = Game.get()
    ms = _moves(state)
    if not ms:
        sw = _best_switch(g, state)
        return sw or state["legal"][0]
    me, foe = state["me"], state["foe"]
    return max(ms, key=lambda a: move_value(g, me, foe, _mv(me, a)))


def average_policy(state, params=None):
    p = params or policy_params("average")
    g = Game.get()
    ms = _moves(state)
    rng = _rng(state, 3)
    if not ms:                                        # forced switch: usually the member with the type advantage
        sw = _legal(state, "switch")
        if sw and rng.random() < p.get("switch_matchup_prob", 0.7):
            return _best_switch(g, state) or sw[0]
        return sw[0] if sw else state["legal"][0]
    rng = _rng(state)
    if rng.random() < p.get("mistake_rate", 0.1):
        return rng.choice(state["legal"] if rng.random() < 0.2 else ms)
    heal = _heal_action(g, state, p.get("heal_below_hp", 0.3))
    if heal:
        return heal
    me, foe = state["me"], state["foe"]
    # nothing left to heal with and nearly dead: swap in a healthier member instead of feeding the foe a KO
    if _legal(state, "switch") and me["hp"] / me["max_hp"] < p.get("switch_below_hp", 0.25) and rng.random() < 0.6:
        sw = _best_switch(g, state, 0.5)
        if sw:
            return sw
    # strongest-looking move: power x effectiveness x STAB x accuracy, ignores TP cost; avoids immune moves
    return max(ms, key=lambda a: move_value(g, me, foe, _mv(me, a)) + 1e-4 * g.moves.get(_mv(me, a), {"power": 0})["power"])


_SEEDED = set()


def skilled_policy(state, params=None):
    p = params or policy_params("skilled")
    g = Game.get()
    me, foe = state["me"], state["foe"]
    ms = _moves(state)
    if not ms:
        return _best_switch(g, state) or state["legal"][0]
    foe_threat = best_threat(g, foe, me)                      # fraction of my max HP per foe hit
    hp_frac = me["hp"] / me["max_hp"]
    foe_frac = foe["hp"] / foe["max_hp"]
    scored = []
    for a in ms:
        mv = _mv(me, a)
        dv = move_value(g, me, foe, mv)                       # of foe max HP
        kills = dv >= foe_frac * 0.95
        cost = me["tp_costs"][a["slot"]] if me.get("tp_costs") and a["slot"] < len(me["tp_costs"]) else g.tp_cost(mv)
        # value: damage dealt capped at what is left, plus a KO bonus; TP thrift when the moves are close
        v = min(dv, foe_frac) + (0.5 if kills else 0.0) - p.get("tp_weight", 0.12) * cost / max(1, me["tp_max"]) * (0.5 if not kills else 1.5)
        if mv not in g.moves or g.moves[mv]["power"] == 0:
            v = 0.02 - 0.12 * cost / max(1, me["tp_max"])
        scored.append((v, dv, a, mv))
    scored.sort(key=lambda x: -x[0])
    best = scored[0]
    # lookahead: how many turns for each side to finish the other?
    my_turns = foe_frac / max(1e-3, best[1])
    foe_turns = hp_frac / max(1e-3, foe_threat)
    i_first = _speed(g, me) >= _speed(g, foe)
    losing = foe_turns < my_turns - (0 if i_first else 0) and not (i_first and my_turns <= foe_turns)
    # heal when I would die next turn and cannot finish first
    if hp_frac < p.get("heal_below_hp", 0.35) and not (best[1] >= foe_frac and i_first):
        h = _heal_action(g, state, 1.0)
        if h and g.heal_amount(h["item"]) / me["max_hp"] > foe_threat * 0.8:
            return h
    # switch on a clearly bad matchup (lookahead 2)
    if p.get("lookahead", 2) >= 2 and losing and foe_turns <= 2.2 and best[1] < 0.5:
        sw = _best_switch(g, state, 0.4)
        if sw:
            m = state["party"][sw["slot"]]
            out = best_threat(g, {"species": m["species"], "level": m["level"], "max_hp": m["max_hp"]}, foe)
            if out > best[1] * 1.4:
                return sw
    # Talk (free action, once per turn): Unnerve makes the trainer Panic (random moves) and failing it makes it
    # Resolute; Provoke makes it Resolute (moves first, double TP). talk_mode decides when (tuned by the talk signal).
    mode = p.get("talk_mode", "smart") if p.get("uses_talk") else "none"
    if mode != "none" and state["turn"] == 0 and state.get("kind") == "trainer":
        talks = {t["kind"]: t for t in _legal(state, "talk")}
        if talks:
            if mode == "unnerve" or (mode == "smart" and (losing or my_turns > 2.5)):
                return talks.get("unnerve") or best[2]
            if mode == "provoke":
                return talks.get("provoke") or best[2]
    # sleep or paralyse a healthy foe first when the fight will take a while (status moves score ~0 above);
    # Leech Seed once per foe (the state has no seeded flag, so the policy remembers what it seeded this battle)
    if p.get("uses_status", True) and my_turns > 2.0 and foe_frac > 0.5:
        key = (state.get("seed"), foe["species"], foe["max_hp"])
        for a in ms:
            if _mv(me, a) == "LEECH_SEED" and key not in _SEEDED and "GRASS" not in g.species[foe["species"]]["types"] \
                    and hp_frac > 0.5:
                if len(_SEEDED) > 4096:
                    _SEEDED.clear()
                _SEEDED.add(key)
                return a
    if p.get("uses_status", True) and not foe.get("status") and my_turns > 2.0 and foe_frac > 0.5:
        for a in ms:
            m = g.moves.get(_mv(me, a))
            if m and m["power"] == 0 and m["effect"] in ("SLEEP", "PARALYZE") and (m["acc"] or 100) >= 70 \
                    and not (m["type"] == "ELECTRIC" and "GROUND" in g.species[foe["species"]]["types"]) \
                    and g.effectiveness(m["type"], g.species[foe["species"]]["types"]) > 0:
                return a
    # Special when it beats the best normal move
    if p.get("uses_special"):
        sp = _legal(state, "special")
        if sp and me.get("special"):
            sv = move_value(g, me, foe, me["special"])
            if sv >= best[1] * 1.05 or sv >= foe_frac * 0.95:
                return sp[0]
    return best[2]


POLICIES = {"random": random_policy, "greedy": greedy_policy, "average": average_policy, "skilled": skilled_policy}
# rom = the ROM's own AI scripts, resolved by the engine; listed so callers can validate names
ENGINE_POLICIES = {"rom"}


def get_policy(name, params=None):
    """Name -> callable(state). `params` overrides the targets.yaml section for average/skilled."""
    if callable(name):
        return name
    if name in ENGINE_POLICIES:
        return name
    base = POLICIES[name]
    if params is None:
        return base
    return lambda state: base(state, params)


# ablation variants used by the talk / special signals
def _variant(base, **over):
    def f(state, params=None):
        return base(state, dict(policy_params("skilled"), **over))
    return f


POLICIES["skilled_notalk"] = _variant(skilled_policy, uses_talk=False)
POLICIES["skilled_unnerve"] = _variant(skilled_policy, uses_talk=True, talk_mode="unnerve")
POLICIES["skilled_provoke"] = _variant(skilled_policy, uses_talk=True, talk_mode="provoke")
POLICIES["skilled_smart"] = _variant(skilled_policy, uses_talk=True, talk_mode="smart")
POLICIES["skilled_nospecial"] = _variant(skilled_policy, uses_special=False)
POLICIES["skilled_plain"] = _variant(skilled_policy, uses_talk=False, uses_special=False)
POLICIES["skilled_nolookahead"] = _variant(skilled_policy, lookahead=1)
