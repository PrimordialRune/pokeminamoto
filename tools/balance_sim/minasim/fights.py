"""The fight catalogue: every fight the report evaluates, with its party and the stage it belongs to.

Sources, best first: `rom` (src/data/trainers.party, placed on a map), `design`
(tools/balance/scenarios/early_game.yaml, placeholders from Omar's Green notes), `placeholder`
(generated here: Gen-1 canon gyms and rivals, plus a few route trainers per stage). Placeholder fights are
labelled so the report can say how much of it rests on invented data; they drop out as real data lands.
"""
import os
import re

import yaml

from .gamedata import Game, ROOT
from .stages import Stages, GYMS

COUNTER = {"BULBASAUR": "CHARMANDER", "CHARMANDER": "SQUIRTLE", "SQUIRTLE": "BULBASAUR"}
# What the rival picks against the player's starter (the beating type)
RIVAL_OF = {"BULBASAUR": "CHARMANDER", "CHARMANDER": "SQUIRTLE", "SQUIRTLE": "BULBASAUR"}
STARTERS = ["BULBASAUR", "CHARMANDER", "SQUIRTLE"]

# Gen-1 canon (Red/Blue) parties used where the design has none yet. PLACEHOLDER.
CANON_GYMS = {
    1: ("Jiro", "boss", ["SPEAROW:8", "PIDGEY:9", "FARFETCHD:11"]),          # the design's Flying gym
    2: ("Brock", "gym", ["GEODUDE:11", "ONIX:14"]),
    3: ("Misty", "gym", ["STARYU:18", "STARMIE:21"]),
    4: ("Surge", "gym", ["VOLTORB:21", "PIKACHU:18", "RAICHU:24"]),
    5: ("Erika", "gym", ["VICTREEBEL:29", "TANGELA:24", "VILEPLUME:29"]),
    6: ("Koga", "gym", ["KOFFING:37", "MUK:39", "KOFFING:37", "WEEZING:43"]),
    7: ("Coleman", "gym", ["GROWLITHE:42", "PONYTA:40", "RAPIDASH:42", "ARCANINE:47"]),
    8: ("Gym 8", "gym", ["RHYHORN:45", "DUGTRIO:42", "NIDOQUEEN:44", "NIDOKING:45", "RHYDON:50"]),
}
# (name, stage index, party with RIVAL_STARTER evolving with level)
CANON_RIVALS = [
    ("Shigeru (route)", 5, ["PIDGEOTTO:18", "ABRA:15", "RATTATA:15", "RIVAL_STARTER:17"]),
    ("Shigeru (ship)", 7, ["PIDGEOTTO:19", "RATICATE:16", "KADABRA:18", "RIVAL_STARTER:20"]),
    ("Shigeru (tower)", 9, ["PIDGEOTTO:25", "GROWLITHE:23", "EXEGGCUTE:22", "KADABRA:20", "RIVAL_STARTER:25"]),
    ("Shigeru (silph)", 11, ["PIDGEOT:37", "GROWLITHE:35", "EXEGGCUTE:38", "ALAKAZAM:35", "RIVAL_STARTER:40"]),
    ("Shigeru (route 22)", 15, ["PIDGEOT:47", "GYARADOS:44", "ARCANINE:45", "ALAKAZAM:50", "RIVAL_STARTER:53"]),
    ("Shigeru (champion)", 17, ["PIDGEOT:61", "ALAKAZAM:59", "RHYDON:61", "ARCANINE:61", "EXEGGUTOR:61", "RIVAL_STARTER:63"]),
]
TRAINER_CLASSES = ["Youngster", "Lass", "Bug Catcher", "Hiker", "Sailor", "Camper", "Picnicker", "Bird Keeper"]


def parse_mon(game, spec, proxies=None):
    m = re.match(r"\s*([^:\[@]+):(\d+)(?:\[([^\]]*)\])?(?:@(\w+))?\s*$", spec)
    name = m.group(1).strip()
    if name.upper() == "RIVAL_STARTER":
        sp = "RIVAL_STARTER"
    else:
        key = re.sub(r"[^A-Z0-9]", "", name.upper())
        for a, t in (proxies or {}).items():
            if re.sub(r"[^A-Z0-9]", "", a.upper()) == key:
                name = t
        sp = game.try_resolve(name)
    if sp is None:
        return None
    moves = None
    if m.group(3):
        moves = [x for x in (game.try_resolve_move(v) for v in m.group(3).split("/")) if x]
    return {"species": sp, "level": int(m.group(2)), "moves": moves or None, "item": m.group(4)}


class Catalog:
    def __init__(self, game=None, stages=None):
        self.game = game or Game.get()
        self.st = stages or Stages(self.game)
        self.lint = []
        self.fights = []
        self.wild = {i: [] for i in range(18)}      # stage -> [(species, lo, hi, weight, source)]
        self.gym_ace = {}
        self.ref_level = {}
        self._rom()
        self._scenario()
        self._canon()
        self._levels()
        self._wild()
        self._route_trainers()
        for f in self.fights:
            f["stage_id"] = "s%02d" % f["stage"]
        self.fights.sort(key=lambda f: (f["stage"], {"wild": 0, "trainer": 1, "rival": 2, "gym": 3, "boss": 3}[f["class"]], f["id"]))

    # ------------------------------------------------------------ helpers
    def _add(self, **kw):
        kw.setdefault("map", None)
        kw.setdefault("rival", any(m["species"] == "RIVAL_STARTER" for m in kw["party"]))
        kw.setdefault("gym_no", None)
        self.fights.append(kw)
        return kw

    def level_stage(self, level):
        """Stage whose reference level is nearest (fallback placement for unplaced ROM trainers)."""
        return min(range(18), key=lambda i: abs(self.ref_level.get(i, i * 3 + 5) - level))

    # ------------------------------------------------------------ sources
    def _rom(self):
        g = self.game
        placed = {}
        for mid, m in g.maps.items():
            for s in m["trainers"]:
                placed[s] = mid
        for tid, t in g.party_trainers.items():
            cls = t["class"]
            if tid.startswith("TRAINER_MAY_"):
                continue                                   # same fights as Brendan's
            if tid.startswith("TRAINER_BRENDAN_ROUTE_103"):
                if tid.endswith("MUDKIP"):                 # one rival fight; its starter varies with the player's
                    self._add(id="rival:route103", name="Shigeru (lab)", **{"class": "rival"}, stage=0,
                              party=[{"species": "RIVAL_STARTER", "level": 5, "moves": None, "item": None}],
                              source="rom", trainer_id=tid, map="MAP_ROUTE103")
                continue
            if cls == "Leader":
                self.gym_rom = t
                continue                                   # filled in _canon as gym 2 (Brock slot)
            self.lint.append(("trainer_unplaced", tid, "%s (%s) is in trainers.party but on no map: not evaluated" % (t["name"], cls)))

    def _scenario(self):
        g = self.game
        path = os.path.join(ROOT, "tools/balance/scenarios/early_game.yaml")
        if not os.path.exists(path):
            return
        sc = yaml.safe_load(open(path, encoding="utf-8"))
        proxies = sc.get("proxies", {})
        row = self.st.row_by_name(sc["start"])
        here = row
        n = 0
        for step in sc["steps"]:
            if "area" in step:
                r = self.st.row_by_name(step["area"])
                if r:
                    here = r
                    st = self.st.of_row(r)
                    for w in step.get("wild", []):
                        sp, lv, wt = w.split(":")
                        lo, _, hi = lv.partition("-")
                        spc = self._res(sp, proxies)
                        if spc:
                            self.wild[st].append((spc, int(lo), int(hi or lo), float(wt), "design"))
                else:
                    self.lint.append(("scenario_area", step["area"], "area not found in overworld.csv"))
            elif "heal" in step:
                r = self.st.row_by_name(step["heal"].replace(" Pokemon Center", ""))
                here = r or here
            elif "trainer" in step or "boss" in step:
                t = step.get("trainer") or step.get("boss")
                if step.get("trainer", {}).get("first_battle"):
                    continue                                # the ROM has the lab rival
                party = [parse_mon(g, s, proxies) for s in t["team"]]
                if any(p is None for p in party):
                    self.lint.append(("scenario_species", t["name"], "unknown species skipped"))
                party = [p for p in party if p]
                if not party:
                    continue
                st = self.st.of_row(here)
                n += 1
                if "boss" in step:
                    gn = step["boss"]["badge"]
                    self._add(id="gym:%d" % gn, name=t["name"].split(" (")[0], **{"class": "gym"}, stage=self.st.gym_stage(gn),
                              party=party, source="design", gym_no=gn)
                else:
                    cls = "rival" if "Shigeru" in t["name"] else "trainer"
                    self._add(id="%s:sc%02d" % (cls, n), name=t["name"], **{"class": cls}, stage=st,
                              party=party, source="design", map=g.map_for_row(here))

    def _res(self, name, proxies):
        key = re.sub(r"[^A-Z0-9]", "", name.upper().replace("♂", "M").replace("♀", "F"))
        for a, t in proxies.items():
            if re.sub(r"[^A-Z0-9]", "", a.upper()) == key:
                name = t
        sp = self.game.try_resolve(name)
        if sp is None:
            self.lint.append(("species", name, "unknown species in an encounter list"))
        return sp

    def _canon(self):
        have = {f["gym_no"] for f in self.fights if f["class"] == "gym"}
        for no, (name, cls, team) in CANON_GYMS.items():
            if no in have:
                continue
            party = [parse_mon(self.game, s) for s in team]
            party = [p for p in party if p]
            if no == 2 and getattr(self, "gym_rom", None):
                party = [dict(p) for p in self.gym_rom["party"]]      # Roxanne's ROM party stands in for gym 2
                self._add(id="gym:2", name="Brock", **{"class": "gym"}, stage=self.st.gym_stage(2), party=party,
                          source="rom", gym_no=2, trainer_id=self.gym_rom and "TRAINER_ROXANNE_1")
                continue
            self._add(id="gym:%d" % no, name=name, **{"class": "gym" if cls == "gym" else "boss"},
                      stage=self.st.gym_stage(no), party=party, source="placeholder", gym_no=no)
        have_names = {f["name"] for f in self.fights if f["class"] == "rival"}
        sc_rivals = [f for f in self.fights if f["class"] == "rival" and f["source"] == "design"]
        for i, (name, stage, team) in enumerate(CANON_RIVALS):
            if any(abs(f["stage"] - stage) <= 1 for f in sc_rivals):
                continue
            party = [parse_mon(self.game, s) for s in team]
            self._add(id="rival:ph%d" % i, name=name, **{"class": "rival"}, stage=stage, party=[p for p in party if p],
                      source="placeholder")

    # ------------------------------------------------------------ levels
    def _levels(self):
        for f in self.fights:
            if f["class"] in ("gym", "boss"):
                self.gym_ace[f["gym_no"]] = max(m["level"] for m in f["party"] if m["species"] != "RIVAL_STARTER")
        prev = 4
        for b in range(9):
            nxt = self.gym_ace.get(b + 1, min(100, prev + 6))
            for late in (0, 1):
                self.ref_level[2 * b + late] = round(prev + (nxt - prev) * (0.7 if late else 0.3)) if b < 8 else round(prev + 5 + 5 * late)
            prev = nxt
        # stage level ranges for the report
        for s in self.st.stages:
            lv = [m["level"] for f in self.fights if f["stage"] == s["index"] for m in f["party"] if m["species"] != "RIVAL_STARTER"]
            r = self.ref_level[s["index"]]
            s["level_range"] = [min(lv + [r]), max(lv + [r])]

    # ------------------------------------------------------------ wild and generated
    def _wild(self):
        g = self.game
        for mid, mons in g.wild.items():
            st = self.st.of_map(mid)
            if st is None:
                continue
            for sp, lo, hi in mons:
                self.wild[st].append((sp, lo, hi, 1.0, "rom"))
        for r in g.overworld:
            st = self.st.of_row(r)
            if any(src in ("rom", "design") for *_x, src in self.wild[st]) and r["order"] <= 6:
                pass
            have_rows = {w[0] for w in self.wild[st]}
            lv = self.ref_level[st]
            for name in r["species_list"] + r["water_list"]:
                sp = self._res(name, {"KOBAT": "ZUBAT", "GYOPIN": "GOLDEEN"})
                if sp and sp not in have_rows and not self._scenario_covered(r, st):
                    self.wild[st].append((sp, max(2, lv - 4), max(3, lv - 1), 1.0, "placeholder"))
        # wild fights: up to 8 species per stage, at their mid level
        for st in range(18):
            seen = {}
            for sp, lo, hi, w, src in self.wild[st]:
                seen.setdefault(sp, (lo, hi, w, src))
            ranked = sorted(seen.items(), key=lambda kv: -kv[1][2])[:8]
            for sp, (lo, hi, w, src) in ranked:
                lv = (lo + hi + 1) // 2
                self._add(id="wild:s%02d:%s" % (st, sp), name="Wild %s" % sp.title(), **{"class": "wild"}, stage=st,
                          party=[{"species": sp, "level": lv, "moves": None, "item": None}], source=src)

    def _scenario_covered(self, row, st):
        return False

    def _route_trainers(self):
        """A few generated route trainers per stage when the design has fewer than 3 there."""
        import random
        rng = random.Random(7)
        for st in range(18):
            have = [f for f in self.fights if f["stage"] == st and f["class"] == "trainer"]
            pool = [(sp, lo, hi) for sp, lo, hi, w, src in self.wild[st]]
            fam = sorted({self.game.base_form(sp) for sp, *_ in pool})
            if not fam:
                continue
            lv = self.ref_level[st]
            for k in range(max(0, 3 - len(have))):
                n = 1 + (k + st) % 3 if st >= 2 else 1 + k % 2
                party = []
                for j in range(n):
                    sp = self.game.evolve_to(rng.choice(fam), lv)
                    party.append({"species": sp, "level": max(2, lv + rng.choice([-1, 0, 0, 1]) - (n > 2)), "moves": None, "item": None})
                self._add(id="trainer:ph%02d_%d" % (st, k), name="%s %d" % (TRAINER_CLASSES[(st + k) % 8], k + 1),
                          **{"class": "trainer"}, stage=st, party=party, source="placeholder")

    # ------------------------------------------------------------ queries
    def by_class(self, cls):
        return [f for f in self.fights if f["class"] == cls]

    def party_for(self, fight, starter):
        """Concrete party for a player starter: RIVAL_STARTER resolves to the counter, evolved to its level."""
        g = self.game
        out = []
        for m in fight["party"]:
            m = dict(m)
            if m["species"] == "RIVAL_STARTER":
                m["species"] = g.evolve_to(RIVAL_OF[starter], m["level"])
            out.append(m)
        return out

    def stats(self):
        n = len(self.fights)
        return {"fights": n, "rom": sum(f["source"] == "rom" for f in self.fights),
                "design": sum(f["source"] == "design" for f in self.fights),
                "placeholder": sum(f["source"] == "placeholder" for f in self.fights)}
