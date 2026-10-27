"""Plausible player teams per fight and band ("run draws").

A draw = (starter, catch picks, levels, movesets, bag), sampled from the band's parameters in
targets.yaml. Average: first commons from the routes behind the player, last-4-learned moves, plays at the
foe's top level minus 1. Skilled: best coverage per TP, plays at the foe's top level. The draw is returned as
plain dicts (`mon_kwargs`) so it pickles into worker processes; `to_mons()` makes engine Mons.
"""
import math
import random

from .gamedata import Game
from .policies import move_value, load_targets, stat
from .fights import STARTERS


class TeamBuilder:
    def __init__(self, catalog, targets=None):
        self.cat = catalog
        self.g = catalog.game
        self.t = targets or load_targets()
        self._pool_cache = {}

    # -------------------------------------------------------- the catch pool
    def catch_pool(self, stage):
        """{base species: weight} of everything catchable up to `stage` (evolved lines included)."""
        if stage in self._pool_cache:
            return self._pool_cache[stage]
        pool = {}
        for st in range(stage + 1):
            for sp, lo, hi, w, src in self.cat.wild[st]:
                b = self.g.base_form(sp)
                pool[b] = pool.get(b, 0) + w
        self._pool_cache[stage] = pool
        return pool

    def foe_top(self, fight, starter):
        return max(m["level"] for m in self.cat.party_for(fight, starter))

    # -------------------------------------------------------- movesets
    def best_moves(self, species, level, stage, n=4):
        """Skilled moveset: greedy set cover of the stage's species pool, per TP."""
        g = self.g
        known = []
        for lv, mv in g.learnsets.get(species, []):
            if lv <= level and mv not in known:
                known.append(mv)
        me = {"species": species, "level": level, "max_hp": 1, "hp": 1}
        defenders = []
        for sp in list(self.catch_pool(stage))[:40]:
            sp = g.evolve_to(sp, level)
            defenders.append({"species": sp, "level": level, "max_hp": stat(g, sp, level, "hp"), "hp": 1})
        if not defenders or not known:
            return None
        cand = [m for m in known if g.moves[m]["power"] > 0]
        chosen, best = [], [0.0] * len(defenders)
        for _ in range(min(n, len(cand))):
            def gain(mv):
                tp = g.tp_cost(mv)
                return sum(max(b, move_value(g, me, d, mv)) - b for b, d in zip(best, defenders)) / (1.0 + tp / 40.0)
            mv = max((m for m in cand if m not in chosen), key=gain, default=None)
            if mv is None:
                break
            chosen.append(mv)
            best = [max(b, move_value(g, me, d, mv)) for b, d in zip(best, defenders)]
        # keep one utility move the engine can use (status) only if slots remain; the policy mostly ignores them
        return chosen or None

    # -------------------------------------------------------- the draw
    def player_team(self, fight, band, starter, rng, with_bag=True):
        """-> {"mons": [mon kwargs], "bag": {...}} for this fight, band and starter."""
        g, par = self.g, self.t["policies"][band]
        stage = fight["stage"]
        badges = stage // 2
        top = self.foe_top(fight, starter)
        if fight["class"] == "wild":                    # wild mons are met below the stage's trainer line
            top = max(top, self.cat.ref_level[stage])
        lead_lv = max(2, top + par["level_offset"] + rng.choice([-1, 0, 0, 1]))
        if fight["class"] == "wild":
            size = 1
        else:
            size = int(max(1, min(6, round(par["party_size_base"] + par["party_per_badge"] * badges))))
            if stage == 0:
                size = 1
        species = [g.evolve_to(starter, lead_lv)]
        pool = dict(self.catch_pool(stage))
        for b in {g.base_form(starter)}:
            pool.pop(b, None)
        picks = []
        while len(picks) < size - 1 and pool:
            keys = list(pool)
            if band == "skilled":
                # prefer high stat totals, with randomness (a skilled player's picks are not identical)
                w = [pool[k] * (g.bst(g.evolve_to(k, lead_lv)) / 400.0) ** 3 for k in keys]
            else:
                w = [pool[k] for k in keys]
            k = rng.choices(keys, w)[0]
            pool.pop(k)
            picks.append(k)
        levels = [lead_lv]
        mons = []
        for j, b in enumerate(picks):
            lv = max(3, lead_lv + par["bench_offset"] - (j % 2))
            species.append(g.evolve_to(b, lv))
            levels.append(lv)
        for sp, lv in zip(species, levels):
            moves = self.best_moves(sp, lv, stage) if band == "skilled" else None
            mons.append({"species": sp, "level": lv, "moves": moves, "item": None,
                         "awakened": rng.random() < par.get("special_awake_prob", 0.0)})
        bag = {}
        if with_bag and fight["class"] != "wild" or (with_bag and band == "skilled"):
            n = int(par["potions_base"] + par["potions_per_badge"] * badges)
            bag["POTION"] = max(0, n - n // 3 * (badges >= 2) - n // 4 * (badges >= 5))
            if badges >= 2:
                bag["SUPER_POTION"] = n // 3
            if badges >= 5:
                bag["HYPER_POTION"] = n // 4
        return {"mons": mons, "bag": {k: v for k, v in bag.items() if v}}

    def foe_mons(self, fight, starter):
        return [dict(m) for m in self.cat.party_for(fight, starter)]


def to_mons(mon_dicts, Mon):
    return [Mon(m["species"], m["level"], moves=m.get("moves"), item=m.get("item")) for m in mon_dicts]
