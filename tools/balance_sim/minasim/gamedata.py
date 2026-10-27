"""Game data for minasim, read from the repo sources (never copied).

Species, moves, learnsets, evolutions, the type chart, growth curves and Minamoto names come from
tools/balance/gamedata.py (one parser for those); this module adds trainers, wild tables, items,
the map graph and the design CSVs.
"""
import csv
import glob
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SIM_ROOT = os.path.dirname(HERE)                       # tools/balance_sim
ROOT = os.path.abspath(os.path.join(SIM_ROOT, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "balance"))
from gamedata import GameData as _BaseData, exp_for_level, _norm  # noqa: E402
sys.path.pop(0)


def _read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as f:
        return f.read()


def title(const):
    return const.replace("_", " ").title()


# New Minamoto species with no ROM species yet, played as their nearest ROM species (alias table; the report
# lists each proxy use in `lint`). Add a row when species.csv / overworld.csv gain a name the ROM lacks.
PROXIES = {"KOBAT": "ZUBAT", "GYOPIN": "GOLDEEN", "MIKON": "VULPIX", "ZUDUCK": "GOLDUCK"}


class Game(_BaseData):
    """Everything the analysis reads. Build once per process: `Game.get()`."""
    _inst = None
    EXTRA_ALIASES = dict(_BaseData.EXTRA_ALIASES, **PROXIES)

    @classmethod
    def get(cls):
        if cls._inst is None:
            cls._inst = cls()
        return cls._inst

    def __init__(self):
        super().__init__()
        self.exp_for_level = exp_for_level
        self.minamoto = {}          # SPECIES const -> Minamoto name
        self._load_minamoto()
        self.items = self._load_items()
        self.maps = self._load_maps()
        self.overworld = self._load_overworld()
        self.wild = self._load_wild()
        self.party_trainers = self._load_party_trainers()
        self.lint = []              # (kind, subject, message)

    # ------------------------------------------------------------ names
    def _load_minamoto(self):
        path = os.path.join(ROOT, "docs/design/data/species.csv")
        with open(path, encoding="utf-8") as f:
            for row in csv.DictReader(f):
                eng = self.aliases.get(_norm(row["localized_name"]))
                if eng:
                    self.minamoto.setdefault(eng, row["pocket_monster"].strip("[]"))
        self.minamoto.setdefault("SPEAROW", "ONISUZUME")

    def species_label(self, sp):
        return "%s (%s)" % (title(sp), self.minamoto[sp]) if sp in self.minamoto else title(sp)

    def try_resolve(self, name):
        try:
            return self.resolve(name)
        except KeyError:
            return None

    # ------------------------------------------------------------ items
    def _load_items(self):
        out = {}
        text = _read("src/data/items.h")
        for m in re.finditer(r"\[ITEM_(\w+)\]\s*=\s*\{", text):
            i, depth = m.end(), 1
            while depth:
                depth += (text[i] == "{") - (text[i] == "}")
                i += 1
            body = text[m.end():i - 1]
            f = lambda k: (re.search(r"\.%s\s*=\s*([^,\n]+)" % k, body) or [None, None])[1]  # noqa: E731
            price = f("price")
            if price is None:
                continue
            out[m.group(1)] = {
                "name": m.group(1), "price": int(price),
                "param": int(f("holdEffectParam") or 0),
                "hold": (f("holdEffect") or "").replace("HOLD_EFFECT_", ""),
                "battle": (f("battleUsage") or "").replace("ITEM_B_USE_", ""),
            }
        return out

    HEAL_AMOUNT = {"POTION": 20, "SUPER_POTION": 50, "HYPER_POTION": 200, "MAX_POTION": 999,
                   "FULL_RESTORE": 999, "FRESH_WATER": 50, "SODA_POP": 60, "LEMONADE": 80, "MOOMOO_MILK": 100}

    def heal_amount(self, item):
        return self.HEAL_AMOUNT.get(item, 0)

    # ------------------------------------------------------------ maps
    def _load_maps(self):
        out = {}
        for path in glob.glob(os.path.join(ROOT, "data", "maps", "*", "map.json")):
            with open(path, encoding="utf-8") as f:
                d = json.load(f)
            out[d["id"]] = {
                "id": d["id"], "dir": os.path.basename(os.path.dirname(path)), "name": d.get("name", ""),
                "type": d.get("map_type", ""),
                "connections": [c["map"] for c in d.get("connections", []) or []],
                "warps": [w.get("dest_map") for w in d.get("warp_events", []) or []],
                "trainers": [o.get("script") for o in d.get("object_events", []) or []
                             if o.get("trainer_type", "TRAINER_TYPE_NONE") != "TRAINER_TYPE_NONE"],
            }
        return out

    def neighbours(self, map_id):
        m = self.maps.get(map_id)
        if not m:
            return []
        return [x for x in m["connections"] + m["warps"] if x in self.maps]

    # ------------------------------------------------------------ overworld.csv
    def _load_overworld(self):
        rows = []
        with open(os.path.join(ROOT, "docs/design/data/overworld.csv"), encoding="utf-8") as f:
            for r in csv.DictReader(f):
                r["order"] = float(r["order_id"])
                r["species_list"] = [s.strip() for s in r["encounters"].split(";") if s.strip()]
                r["water_list"] = [s.strip() for s in r["water_encounters"].split(";") if s.strip()]
                rows.append(r)
        rows.sort(key=lambda r: r["order"])
        return rows

    ROW_ALIASES = {"Vermillion City": "MAP_VERMILION_CITY", "Digda Cave": "MAP_DIGLETTS_CAVE_NORTH_ENTRANCE",
                   "Cycling Road": "MAP_ROUTE17", "Route 21": "MAP_ROUTE21_NORTH",
                   "Twin Islands": "MAP_SEAFOAM_ISLANDS_1F"}

    def map_for_row(self, row):
        """The MAP_ id an overworld.csv row names, or None. Matches the English name."""
        label = row["localized_name"] or row["name"]
        if label in self.ROW_ALIASES:
            return self.ROW_ALIASES[label]
        want = _norm(label)
        cands = {}
        for mid, m in self.maps.items():
            cands[_norm(m["name"])] = mid
        if want in cands:
            return cands[want]
        # Cave / multi-floor: first floor
        for suffix in ("1F", "B1F"):
            if want + suffix.upper() in cands:
                return cands[want + suffix.upper()]
        for k, mid in sorted(cands.items()):
            if k.startswith(want) and re.match(r"^(1F|B1F|ENTRANCE|EXTERIOR)", k[len(want):] or "1F"):
                return mid
        return None

    # ------------------------------------------------------------ wild
    def _load_wild(self):
        out = {}
        with open(os.path.join(ROOT, "src/data/wild_encounters.json"), encoding="utf-8") as f:
            d = json.load(f)
        for g in d["wild_encounter_groups"]:
            for e in g.get("encounters", []):
                mons = []
                for kind in ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons"):
                    blk = e.get(kind)
                    if not blk or kind != "land_mons":
                        continue
                    for mn in blk["mons"]:
                        mons.append((mn["species"].replace("SPECIES_", ""), mn["min_level"], mn["max_level"]))
                if mons:
                    out[e["map"]] = mons
        return out

    # ------------------------------------------------------------ trainers.party
    def _load_party_trainers(self):
        text = _read("src/data/trainers.party")
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        out = {}
        for block in re.split(r"^=== (TRAINER_\w+) ===\s*$", text, flags=re.M)[1:]:
            pass
        parts = re.split(r"^=== (TRAINER_\w+) ===\s*$", text, flags=re.M)
        for tid, body in zip(parts[1::2], parts[2::2]):
            if tid == "TRAINER_NONE":
                continue
            head, _, rest = body.partition("\n\n")
            hd = {}
            for line in head.splitlines():
                if ":" in line:
                    k, v = line.split(":", 1)
                    hd[k.strip()] = v.strip()
            party = []
            for mon in re.split(r"\n\s*\n", rest.strip()):
                lines = [l for l in mon.strip().splitlines() if l.strip()]
                if not lines:
                    continue
                m = re.match(r"^(.+?)(?: \(([MF])\))?(?: @ (.+))?$", lines[0].strip())
                name = re.sub(r"\s*\(.*?\)", "", m.group(1)).strip()
                sp = self.try_resolve(name)
                lvl = 5
                moves = []
                for l in lines[1:]:
                    l = l.strip()
                    if l.startswith("Level:"):
                        lvl = int(l.split(":")[1])
                    elif l.startswith("- "):
                        mv = self.try_resolve_move(l[2:])
                        if mv:
                            moves.append(mv)
                if sp:
                    party.append({"species": sp, "level": lvl, "moves": moves or None,
                                  "item": (m.group(3) or "").upper().replace(" ", "_").replace("'", "") or None})
            if party:
                out[tid] = {"id": tid, "name": hd.get("Name", tid), "class": hd.get("Class", ""),
                            "items": [x.strip() for x in hd.get("Items", "").split("/") if x.strip()],
                            "ai": [re.sub(r"[^A-Z0-9]+", "_", x.strip().upper()) for x in hd.get("AI", "").split("/") if x.strip()],
                            "double": hd.get("Double Battle", "No") == "Yes", "party": party}
        return out

    def try_resolve_move(self, name):
        key = name.upper().replace(" ", "_").replace("-", "_").replace("'", "")
        return key if key in self.moves else None

    # ------------------------------------------------------------ derived helpers
    def evolve_to(self, species, level):
        """Species a mon of `level` would be after level evolutions."""
        evo = self.evolutions.get(species)
        while evo and level >= evo[0] and evo[1] in self.species:
            species = evo[1]
            evo = self.evolutions.get(species)
        return species

    def base_form(self, species):
        pre = {to: frm for frm, (_, to) in self.evolutions.items()}
        while species in pre:
            species = pre[species]
        return species

    def bst(self, species):
        s = self.species[species]
        return s["hp"] + s["atk"] + s["def"] + s["spe"] + s["spa"] + s["spd"]

    def level_moves(self, species, level, n=4):
        return self.moves_at(species, level)[-n:]

    def tp_cost(self, move):
        return self.moves[move]["tp"]
