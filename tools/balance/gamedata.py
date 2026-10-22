"""Read battle data straight from the decomp sources so the model never drifts from the ROM.

Parsed: base stats, types, growth rates, EXP yield and abilities (species_info.h), moves with
tpCost (battle_moves.h), level-up learnsets, level evolutions, the type chart (battle_main.c)
and Minamoto species names (docs/design/data/species.csv).
"""
import csv
import os
import re
import unicodedata

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

TYPES = ["NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL",
         "MYSTERY", "FIRE", "WATER", "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON", "DARK"]
# Gen 3 splits physical/special by type (IS_TYPE_PHYSICAL: everything below TYPE_MYSTERY)
PHYSICAL_TYPES = set(TYPES[:9])

GROWTH = {"MEDIUM_FAST", "ERRATIC", "FLUCTUATING", "MEDIUM_SLOW", "FAST", "SLOW"}


def _read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8") as f:
        return f.read()


def _blocks(text, prefix):
    """Yield (NAME, body) for each `[PREFIX_NAME] = { ... },` designated initializer."""
    for m in re.finditer(r"\[%s_(\w+)\]\s*=\s*\{" % prefix, text):
        depth, i = 1, m.end()
        while depth:
            c = text[i]
            depth += (c == "{") - (c == "}")
            i += 1
        yield m.group(1), text[m.end():i - 1]


def _field(body, name, default=None):
    m = re.search(r"\.%s\s*=\s*([^,\n]+)" % name, body)
    return m.group(1).strip() if m else default


def exp_for_level(growth, n):
    """Total EXP to reach level n, same curves as src/data/pokemon/experience_tables.h."""
    if n <= 1:
        return 0
    if growth == "MEDIUM_FAST":
        return n ** 3
    if growth == "FAST":
        return (4 * n ** 3) // 5
    if growth == "SLOW":
        return (5 * n ** 3) // 4
    if growth == "MEDIUM_SLOW":
        return (6 * n ** 3) // 5 - 15 * n * n + 100 * n - 140
    if growth == "ERRATIC":
        if n <= 50:
            return n ** 3 * (100 - n) // 50
        if n <= 68:
            return n ** 3 * (150 - n) // 100
        if n <= 98:
            return n ** 3 * ((1911 - 10 * n) // 3) // 500
        return n ** 3 * (160 - n) // 100
    if growth == "FLUCTUATING":
        if n <= 15:
            return n ** 3 * ((n + 1) // 3 + 24) // 50
        if n <= 36:
            return n ** 3 * (n + 14) // 50
        return n ** 3 * (n // 2 + 32) // 50
    raise ValueError(growth)


def _norm(name):
    name = name.replace("♂", "M").replace("♀", "F")
    name = unicodedata.normalize("NFKD", name)
    return re.sub(r"[^A-Z0-9]", "", name.upper())


class GameData:
    def __init__(self):
        self.species = {}
        self.moves = {}
        self.learnsets = {}
        self.evolutions = {}
        self.type_chart = {}
        self.aliases = {}
        self._load_species()
        self._load_moves()
        self._load_learnsets()
        self._load_evolutions()
        self._load_type_chart()
        self._load_aliases()
        self._load_ranks()
        self._load_families()

    def _load_species(self):
        for name, body in _blocks(_read("src/data/pokemon/species_info.h"), "SPECIES"):
            hp = _field(body, "baseHP")
            if hp is None or not hp.isdigit():
                continue
            types = re.search(r"\.types\s*=\s*\{\s*TYPE_(\w+)\s*,\s*TYPE_(\w+)", body).groups()
            ability = re.search(r"\.abilities\s*=\s*\{\s*ABILITY_(\w+)", body)
            self.species[name] = {
                "name": name,
                "hp": int(hp),
                "atk": int(_field(body, "baseAttack")),
                "def": int(_field(body, "baseDefense")),
                "spe": int(_field(body, "baseSpeed")),
                "spa": int(_field(body, "baseSpAttack")),
                "spd": int(_field(body, "baseSpDefense")),
                "types": tuple(dict.fromkeys(types)),
                "exp_yield": int(_field(body, "expYield")),
                "growth": _field(body, "growthRate").replace("GROWTH_", ""),
                "ability": ability.group(1) if ability else "NONE",
            }

    def _load_moves(self):
        for name, body in _blocks(_read("src/data/battle_moves.h"), "MOVE"):
            if _field(body, "effect") is None:
                continue
            self.moves[name] = {
                "name": name,
                "effect": _field(body, "effect").replace("EFFECT_", ""),
                "power": int(_field(body, "power", "0")),
                "type": _field(body, "type").replace("TYPE_", ""),
                "acc": int(_field(body, "accuracy", "0")),
                "tp": int(_field(body, "tpCost", "0")),
                "pp": int(_field(body, "pp", "0")),
                "chance": int(_field(body, "secondaryEffectChance", "0")),
                "priority": int(_field(body, "priority", "0")),
            }

    def _load_learnsets(self):
        arrays = {}
        text = _read("src/data/pokemon/level_up_learnsets.h")
        for m in re.finditer(r"static const u16 (\w+)\[\] = \{(.*?)\};", text, re.S):
            arrays[m.group(1)] = [(int(l), mv) for l, mv in re.findall(r"LEVEL_UP_MOVE\(\s*(\d+),\s*MOVE_(\w+)\)", m.group(2))]
        for sp, arr in re.findall(r"\[SPECIES_(\w+)\]\s*=\s*(\w+)", _read("src/data/pokemon/level_up_learnset_pointers.h")):
            if sp != "NONE" and arr in arrays:
                self.learnsets[sp] = arrays[arr]

    def _load_evolutions(self):
        for sp, body in _blocks(_read("src/data/pokemon/evolution.h"), "SPECIES"):
            for lvl, target in re.findall(r"EVO_LEVEL\w*,\s*(\d+),\s*SPECIES_(\w+)", body):
                self.evolutions.setdefault(sp, (int(lvl), target))

    def _load_type_chart(self):
        text = _read("src/battle_main.c")
        body = text[text.index("gTypeEffectiveness["):]
        body = body[:body.index("TYPE_FORESIGHT")]
        mul = {"NO_EFFECT": 0, "NOT_EFFECTIVE": 5, "SUPER_EFFECTIVE": 20}
        for atk, dfn, m in re.findall(r"TYPE_(\w+),\s*TYPE_(\w+),\s*TYPE_MUL_(\w+)", body):
            self.type_chart[(atk, dfn)] = mul[m]

    # Minamoto names the Notion species table lacks (it has 180 rows with betas, not every ROM species)
    EXTRA_ALIASES = {"ONISUZUME": "SPEAROW"}

    def _load_aliases(self):
        for sp in self.species:
            self.aliases[_norm(sp)] = sp
        for k, v in self.EXTRA_ALIASES.items():
            self.aliases[_norm(k)] = v
        path = os.path.join(ROOT, "docs/design/data/species.csv")
        if not os.path.exists(path):
            return
        with open(path, encoding="utf-8") as f:
            for row in csv.DictReader(f):
                eng = self.aliases.get(_norm(row["localized_name"]))
                if eng:
                    self.aliases.setdefault(_norm(row["pocket_monster"].strip("[]")), eng)

    def _load_ranks(self):
        """Move rank 1-3: Notion's stars for NORMAL/ELEMENTAL family moves, else a power tier.

        Notion only ranks the 63 family moves; everything else is listed at one star, so other
        moves get a tier by power (<=45 one star, <=75 two, above that three) as a stand-in.
        """
        notion = {}
        path = os.path.join(ROOT, "docs/design/data/moves.csv")
        if os.path.exists(path):
            with open(path, encoding="utf-8") as f:
                for row in csv.DictReader(f):
                    if row["category"] in ("NORMAL", "ELEMENTAL") and row["rank"]:
                        notion[row["name"].upper().replace(" ", "_").replace("-", "_")] = len(row["rank"])
        for name, m in self.moves.items():
            if name in notion:
                m["rank"], m["rank_source"] = notion[name], "notion"
            else:
                p = m["power"]
                m["rank"], m["rank_source"] = (1 if p <= 45 else 2 if p <= 75 else 3), "power"

    def _load_families(self):
        """Move families with ranks (Notion NORMAL subcategories and ELEMENTAL types) for the C+ proposal.

        A rank the ROM doesn't have becomes a plain damaging move with Notion's power, type and accuracy.
        Family costs follow the PP-tier idea by rank: one star keeps the ROM move's PP (35 if new),
        two stars count as PP 15, three stars as PP 5.
        """
        self.families = {}       # family -> [move for rank 1, 2, 3]
        self.family_of = {}      # move -> family
        path = os.path.join(ROOT, "docs/design/data/moves.csv")
        if not os.path.exists(path):
            return
        with open(path, encoding="utf-8") as f:
            rows = [r for r in csv.DictReader(f) if r["category"] in ("NORMAL", "ELEMENTAL") and r["rank"]]
        for r in rows:
            fam = r["subcategory"] if r["category"] == "NORMAL" else r["type"]
            if not fam or not r["power"] or r["power"] == "0":
                continue
            rank = len(r["rank"])
            key = r["name"].upper().replace(" ", "_").replace("-", "_")
            rom = self.moves.get(key)
            if rom is None or (rom["power"] == 0) != (int(r["power"]) == 0):
                key = "FAM_" + key if rom is not None else key
                self.moves[key] = {"name": key, "effect": "HIT", "power": int(r["power"]), "type": r["type"],
                                   "acc": int(r["accuracy"] or 100), "tp": 0, "pp": 35, "chance": 0, "priority": 0,
                                   "rank": rank, "rank_source": "notion", "synthetic": True}
            m = self.moves[key]
            m["family_pp"] = m["pp"] if rank == 1 else (15 if rank == 2 else 5)
            self.families.setdefault(fam, [None, None, None])[rank - 1] = key
            self.family_of[key] = fam

    def slot_family(self, move):
        """Family a learned ROM move occupies: its own Notion family, else its type's elemental family."""
        if move in self.family_of:
            return self.family_of[move]
        m = self.moves[move]
        if m["power"] > 0 and m["type"] in self.families and m["type"] != "NORMAL":
            return m["type"]
        return None

    def evo_stage(self, species):
        """(stage, final): stage 1 basic, 2 middle, 3 final of a three-stage line."""
        pre = {to: frm for frm, (_, to) in self.evolutions.items()}
        stage, sp = 1, species
        while sp in pre:
            stage, sp = stage + 1, pre[sp]
        return stage, species not in self.evolutions

    def resolve(self, name):
        """Species constant from an English constant, English name or Minamoto name (KAMONEGI)."""
        key = _norm(name)
        if key not in self.aliases:
            raise KeyError("unknown species %r" % name)
        return self.aliases[key]

    def move(self, name):
        key = name.upper().replace(" ", "_").replace("-", "_")
        if key not in self.moves:
            raise KeyError("unknown move %r" % name)
        return self.moves[key]

    def effectiveness(self, move_type, def_types):
        """Type multiplier x10 per defending type, as the engine applies it (e.g. 20 * 5 / 10)."""
        mult = 1.0
        for t in def_types:
            mult *= self.type_chart.get((move_type, t), 10) / 10
        return mult

    def moves_at(self, species, level):
        """Last four moves learned by level-up at or below `level` (what a wild/trainer mon knows)."""
        known = []
        for lvl, mv in self.learnsets.get(species, []):
            if lvl <= level and mv not in known:
                known.append(mv)
        return known[-4:]

    def intelligence(self, species):
        """Placeholder INT: base Sp. Def, same as GetSpeciesIntelligence() in src/battle_util.c."""
        return self.species[species]["spd"]
