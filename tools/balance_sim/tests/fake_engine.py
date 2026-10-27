"""A tiny stand-in for minasim.engine with the same signature and simple damage math.

Used until/unless the native engine is available (MINASIM_FAKE=1 forces it). Not faithful to the ROM.
"""
import random
from dataclasses import dataclass
from typing import Optional

from minasim.gamedata import Game
from minasim import policies as P


@dataclass
class Mon:
    species: str
    level: int
    moves: Optional[list] = None
    item: Optional[str] = None
    ivs: Optional[list] = None
    nature: Optional[str] = None
    tp_spent: int = 0
    hp: Optional[int] = None          # extra: starting HP (None = full); asked of the real engine too


class _B:
    def __init__(self, g, m, side):
        self.g = g
        s = g.species[m.species]
        self.species, self.level = m.species, m.level
        self.max_hp = P.stat(g, m.species, m.level, "hp")
        self.hp = min(self.max_hp, m.hp) if m.hp is not None else self.max_hp
        self.moves = list(m.moves) if m.moves else g.level_moves(m.species, m.level)
        self.moves = [x for x in self.moves if x in g.moves] or ["TACKLE"]
        self.tp_max = 100
        self.tp = max(0, 100 - m.tp_spent)
        self.status = None
        self.stages = {k: 0 for k in ("atk", "def", "spa", "spd", "spe", "acc", "eva")}
        self.item = m.item
        self.special = None

    def view(self):
        return {"species": self.species, "level": self.level, "hp": self.hp, "max_hp": self.max_hp, "tp": self.tp,
                "tp_max": self.tp_max, "status": self.status, "stages": dict(self.stages)}


class Engine:
    def __init__(self, *a, **k):
        self.g = Game.get()

    def battle(self, seed=0, player=(), foe=(), trainer=None, player_policy="greedy", foe_policy="rom",
               log=False, bag=None, max_turns=200):
        g = self.g
        rng = random.Random(seed)
        pp = P.get_policy(player_policy) if player_policy != "rom" else P.greedy_policy
        fp = P.get_policy(foe_policy) if foe_policy != "rom" else P.greedy_policy
        sides = [[_B(g, m, 0) for m in player], [_B(g, m, 1) for m in foe]]
        act = [0, 0]
        for s in (0, 1):
            while act[s] < len(sides[s]) and sides[s][act[s]].hp <= 0:
                act[s] += 1
        items = dict(bag or {})
        used = {"moves": [{}, {}], "items": {}, "talk": 0, "special": 0, "struggles": 0, "min_tp": 100}
        events = []
        turn = 0
        winner = 2
        while turn < max_turns:
            if act[0] >= len(sides[0]):
                winner = 1
                break
            if act[1] >= len(sides[1]):
                winner = 0
                break
            acts = []
            decided = [act[0], act[1]]
            for s, pol in ((0, pp), (1, fp)):
                me, fo = sides[s][act[s]], sides[1 - s][act[1 - s]]
                legal = []
                for i, mv in enumerate(me.moves):
                    if me.tp >= g.tp_cost(mv):
                        legal.append({"type": "move", "slot": i})
                if not legal:
                    legal.append({"type": "move", "slot": 0})
                for j, b in enumerate(sides[s]):
                    if j != act[s] and b.hp > 0:
                        legal.append({"type": "switch", "slot": j})
                if s == 0:
                    for it, n in items.items():
                        if n > 0 and g.heal_amount(it):
                            legal.append({"type": "item", "item": it, "target": act[0]})
                st = {"side": s, "turn": turn, "seed": seed, "kind": "trainer" if len(foe) > 1 else "wild",
                      "me": dict(me.view(), moves=me.moves, tp_costs=[g.tp_cost(x) for x in me.moves], special=me.special),
                      "foe": fo.view(), "party": [dict(b.view(), alive=b.hp > 0, active=k == act[s]) for k, b in enumerate(sides[s])],
                      "items": dict(items) if s == 0 else {}, "legal": legal}
                a = pol(st)
                if a not in legal:
                    a = legal[0]
                acts.append(a)
            order = sorted((0, 1), key=lambda s: (acts[s]["type"] != "item" and acts[s]["type"] != "switch",
                                                  -P._speed(g, sides[s][act[s]].view()), rng.random()))
            for s in order:
                me, fo = sides[s][act[s]], sides[1 - s][act[1 - s]]
                if me.hp <= 0 or act[s] != decided[s] and acts[s]["type"] == "move":
                    continue
                a = acts[s]
                if a["type"] == "item":
                    me.hp = min(me.max_hp, me.hp + g.heal_amount(a["item"]))
                    items[a["item"]] -= 1
                    used["items"][a["item"]] = used["items"].get(a["item"], 0) + 1
                elif a["type"] == "switch":
                    act[s] = a["slot"]
                elif a["type"] == "move":
                    mv = me.moves[a["slot"]]
                    used["moves"][s][mv] = used["moves"][s].get(mv, 0) + 1
                    me.tp = max(0, me.tp - g.tp_cost(mv))
                    if s == 0:
                        used["min_tp"] = min(used["min_tp"], me.tp)
                    mm = g.moves[mv]
                    if rng.random() * 100 < (mm["acc"] or 100) and mm["power"]:
                        d = P.move_value(g, me.view(), fo.view(), mv) / 0.925 * fo.max_hp * (0.85 + 0.15 * rng.random())
                        if rng.random() < 1 / 16:
                            d *= 1.5
                        fo.hp -= max(1, int(d)) if d >= 0.5 else 0
                    # regen
                    me.tp = min(me.tp_max, me.tp + 3 + g.species[me.species]["spd"] // 32)
                for s2 in (0, 1):
                    if sides[s2][act[s2]].hp <= 0 and any(b.hp > 0 for b in sides[s2]):
                        sides[s2][act[s2]].hp = 0
                        nxt = next(j for j, b in enumerate(sides[s2]) if b.hp > 0)
                        act[s2] = nxt
            for s2 in (0, 1):
                if act[s2] < len(sides[s2]) and sides[s2][act[s2]].hp <= 0:
                    alive = [j for j, b in enumerate(sides[s2]) if b.hp > 0]
                    if alive:
                        act[s2] = alive[0]
                    else:
                        act[s2] = len(sides[s2])
            turn += 1
        res = {"winner": winner, "turns": turn,
               "player": [dict(b.view()) for b in sides[0]], "foe": [dict(b.view()) for b in sides[1]],
               "moves_used": used["moves"][0], "foe_moves_used": used["moves"][1], "items_used": used["items"],
               "talk_used": used["talk"], "special_used": used["special"], "struggles": used["struggles"],
               "min_tp": used["min_tp"]}
        if log:
            res["log"] = []
        return res


def run_batch(specs, jobs=1):
    e = Engine()
    return [e.battle(**s) for s in specs]
