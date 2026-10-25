#!/usr/bin/env python3
"""Data consistency checks for CI: `python3 tools/check_data.py` (also run by `make check`).

Reads the decomp sources as text (reusing tools/balance/gamedata.py for species, moves, learnsets and
evolutions) and fails on:
  * Gen 1 species (1..151) with missing or implausible stats, types, growth rate, catch rate, learnset or evolution
  * Gen 1 moves (1..164) without a tpCost, with Gen 2/3 types, or bad accuracy
  * learnset moves that do not exist or are not Gen 1 moves; learnsets out of level order
  * Gen 2/3 species or moves in trainer parties, wild encounters, Battle Frontier / tent data, contest opponents
  * script and map references (TRAINER_/FLAG_/VAR_/ITEM_/SPECIES_/MOVE_/MAP_/SE_/MUS_) that are not defined anywhere
Use --list to print every check name, -v for details of passing checks.
"""
import argparse
import glob
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "balance"))
import gamedata  # noqa: E402

GEN1_TYPES = {"NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST",
              "FIRE", "WATER", "GRASS", "ELECTRIC", "PSYCHIC", "ICE", "DRAGON"}
CHECKS = []


def check(fn):
    CHECKS.append(fn)
    return fn


def read(rel):
    with open(os.path.join(ROOT, rel), encoding="utf-8", errors="replace") as f:
        return f.read()


def defines(path):
    """name -> value of every `#define NAME value` in a header."""
    out = {}
    for m in re.finditer(r"^#define\s+(\w+)\s+(.*)$", read(path), re.M):
        out[m.group(1)] = m.group(2).split("//")[0].strip()
    return out


class Ctx:
    def __init__(self):
        self.gd = gamedata.GameData()
        sp = defines("include/constants/species.h")
        self.species_id = {k[len("SPECIES_"):]: int(v) for k, v in sp.items()
                           if k.startswith("SPECIES_") and v.isdigit()}
        mv = defines("include/constants/moves.h")
        self.move_id = {k[len("MOVE_"):]: int(v) for k, v in mv.items() if k.startswith("MOVE_") and v.isdigit()}
        # The deleted Gen 2/3 species (constants/removed_species.h) still count as non-Gen 1 names
        removed = "include/constants/removed_species.h"
        if os.path.exists(os.path.join(ROOT, removed)):
            for k in defines(removed):
                if k.startswith("SPECIES_") and k != "SPECIES_REMOVED_START":
                    self.species_id.setdefault(k[len("SPECIES_"):], 0xF000)
        self.gen1_species = {n for n, i in self.species_id.items() if 1 <= i <= 151}
        self.gen1_moves = {n for n, i in self.move_id.items() if 1 <= i <= 164}
        self.gen1_moves.add("STRUGGLE")
        self.exempt_species = {"NONE", "EGG"}
        self.exempt_moves = {"NONE"}
        self.errors = []

    def err(self, check_name, msg):
        self.errors.append((check_name, msg))

    @staticmethod
    def key(check_name, msg):
        """Baseline key: the check plus the file the problem was found in (or the species/move it names)."""
        m = re.match(r"((?:src|data|include)/[\w./-]+?)(?::\d+)?[ :]", msg)
        if m:
            return f"{check_name} {m.group(1)}"
        m = re.match(r"((?:SPECIES|MOVE|TRAINER)_\w+)", msg)
        return f"{check_name} {m.group(1)}" if m else check_name


def species_ok(ctx, name):
    return name in ctx.gen1_species or name in ctx.exempt_species


# --- species -------------------------------------------------------------------------

@check
def species_base_data(ctx):
    n = "species_base_data"
    catch = {}
    for name, body in gamedata._blocks(read("src/data/pokemon/species_info.h"), "SPECIES"):
        cr = gamedata._field(body, "catchRate")
        if cr and cr.isdigit():
            catch[name] = int(cr)
    for name in sorted(ctx.gen1_species, key=lambda s: ctx.species_id[s]):
        s = ctx.gd.species.get(name)
        if s is None:
            ctx.err(n, f"SPECIES_{name} has no species_info entry")
            continue
        for stat in ("hp", "atk", "def", "spe", "spa", "spd"):
            if not 1 <= s[stat] <= 255:
                ctx.err(n, f"SPECIES_{name} base {stat} = {s[stat]}")
        if not set(s["types"]) <= GEN1_TYPES:
            ctx.err(n, f"SPECIES_{name} has non-Gen-1 type {s['types']}")
        if s["growth"] not in gamedata.GROWTH:
            ctx.err(n, f"SPECIES_{name} growth {s['growth']}")
        if not 1 <= s["exp_yield"] <= 255:
            ctx.err(n, f"SPECIES_{name} expYield {s['exp_yield']}")
        if not 1 <= catch.get(name, 0) <= 255:
            ctx.err(n, f"SPECIES_{name} catchRate {catch.get(name)}")
        bst = sum(s[k] for k in ("hp", "atk", "def", "spe", "spa", "spd"))
        if not 150 <= bst <= 780:
            ctx.err(n, f"SPECIES_{name} base stat total {bst} looks wrong")


@check
def species_contiguous(ctx):
    ids = sorted(ctx.species_id[s] for s in ctx.gen1_species)
    if ids != list(range(1, 152)):
        ctx.err("species_contiguous", "Gen 1 species ids are not exactly 1..151")


@check
def species_learnsets(ctx):
    n = "species_learnsets"
    for name in sorted(ctx.gen1_species):
        ls = ctx.gd.learnsets.get(name)
        if not ls:
            ctx.err(n, f"SPECIES_{name} has no level-up learnset")
            continue
        if ls[0][0] > 1 and not any(l == 1 for l, _ in ls):
            ctx.err(n, f"SPECIES_{name} knows no move at level 1")
        last = 0
        for lvl, mv in ls:
            if not 1 <= lvl <= 100:
                ctx.err(n, f"SPECIES_{name} learns {mv} at level {lvl}")
            if lvl < last:
                ctx.err(n, f"SPECIES_{name} learnset not in level order at {mv}")
            last = lvl
            if mv not in ctx.move_id:
                ctx.err(n, f"SPECIES_{name} learns unknown move {mv}")
            elif mv not in ctx.gen1_moves:
                ctx.err(n, f"SPECIES_{name} learns Gen 2/3 move MOVE_{mv}")
            elif ctx.gd.moves.get(mv, {}).get("tp", 0) <= 0:
                ctx.err(n, f"SPECIES_{name} learns MOVE_{mv} which has no tpCost")


@check
def species_evolutions(ctx):
    n = "species_evolutions"
    for frm, (lvl, to) in ctx.gd.evolutions.items():
        if frm not in ctx.gen1_species:
            continue
        if to not in ctx.gen1_species:
            ctx.err(n, f"SPECIES_{frm} evolves into non-Gen-1 SPECIES_{to}")
        if not 2 <= lvl <= 100:
            ctx.err(n, f"SPECIES_{frm} evolves at level {lvl}")
        elif ctx.species_id.get(to, 0) <= ctx.species_id.get(frm, 0) and frm not in ("EEVEE",):
            ctx.err(n, f"SPECIES_{frm} evolves into lower-numbered SPECIES_{to}")


# --- moves ---------------------------------------------------------------------------

@check
def moves_tp_and_types(ctx):
    n = "moves_tp_and_types"
    for name in sorted(ctx.gen1_moves, key=lambda m: ctx.move_id[m]):
        m = ctx.gd.moves.get(name)
        if m is None:
            ctx.err(n, f"MOVE_{name} has no battle_moves entry")
            continue
        if name == "STRUGGLE":
            if m["tp"] != 0:
                ctx.err(n, f"MOVE_STRUGGLE tpCost {m['tp']} should be 0")
            continue
        if not 1 <= m["tp"] <= 100:
            ctx.err(n, f"MOVE_{name} tpCost {m['tp']} outside 1..100 (TP_MAX)")
        if m["type"] not in GEN1_TYPES:
            ctx.err(n, f"MOVE_{name} has non-Gen-1 type {m['type']}")
        if not 0 <= m["acc"] <= 100:
            ctx.err(n, f"MOVE_{name} accuracy {m['acc']}")
        if not 0 <= m["power"] <= 255:
            ctx.err(n, f"MOVE_{name} power {m['power']}")
        if m["power"] == 0 and m["tp"] > 50:
            ctx.err(n, f"MOVE_{name} is a status move costing {m['tp']} TP")


# --- trainers, wild encounters, other species/move lists ----------------------------

def scan_species_moves(ctx, check_name, path, species_re=r"SPECIES_(\w+)", move_re=r"MOVE_(\w+)"):
    text = read(path)
    for m in re.finditer(species_re, text):
        sp = m.group(1)
        if sp not in ctx.species_id:
            continue    # SPECIES_UNOWN_B style helpers, macros
        if not species_ok(ctx, sp):
            line = text.count("\n", 0, m.start()) + 1
            ctx.err(check_name, f"{path}:{line} SPECIES_{sp} is not a Gen 1 species")
    for m in re.finditer(move_re, text):
        mv = m.group(1)
        if mv in ctx.move_id and mv not in ctx.gen1_moves and mv not in ctx.exempt_moves:
            line = text.count("\n", 0, m.start()) + 1
            ctx.err(check_name, f"{path}:{line} MOVE_{mv} is not a Gen 1 move")


def party_constant(prefix, word):
    """A trainers.party name as its constant, the way tools/trainerproc spells it."""
    if word.startswith(prefix + "_"):
        return word[len(prefix) + 1:]
    word = word.replace("\u2642", " M").replace("\u2640", " F").replace("'", "").replace("%", "")
    return re.sub(r"[^A-Z0-9]+", "_", word.upper()).strip("_")


@check
def trainer_parties(ctx):
    n = "trainer_parties"
    path = "src/data/trainers.party"
    text = re.sub(r"/\*.*?\*/", lambda m: "\n" * m.group(0).count("\n"), read(path), flags=re.S)
    opp = set(re.findall(r"^#define[ \t]+(TRAINER_\w+)[ \t]+\d+", read("include/constants/opponents.h"), re.M))
    seen = set()
    trainer = None
    in_party = False
    for lineno, line in enumerate(text.splitlines(), 1):
        line = line.strip()
        m = re.match(r"^===\s*(\w+)\s*===$", line)
        if m:
            trainer, in_party = m.group(1), False
            if trainer in seen:
                ctx.err(n, f"{path}:{lineno} {trainer} is defined twice")
            seen.add(trainer)
            if trainer not in opp:
                ctx.err(n, f"{path}:{lineno} {trainer} is not in opponents.h")
            continue
        if not line or trainer is None:
            in_party = in_party or (not line and trainer is not None)
            continue
        if line.startswith("- "):
            mv = party_constant("MOVE", line[2:].strip())
            if mv not in ctx.gen1_moves:
                ctx.err(n, f"{path}:{lineno} MOVE_{mv} is not a Gen 1 move")
            continue
        km = re.match(r"^([A-Za-z][A-Za-z ]*):\s*(.*)$", line)
        if km or re.match(r"^\w+ Nature$", line):
            if km and km.group(1) == "Level" and not (km.group(2).isdigit() and 1 <= int(km.group(2)) <= 100):
                ctx.err(n, f"{path}:{lineno} level {km.group(2)}")
            continue
        if in_party:   # a Pokemon header: [Nickname (]Species[)] [(M)/(F)] [@ Item]
            head = line.split("@")[0].strip()
            parts = re.findall(r"\(([^()]*)\)", head)
            sp = parts[0] if parts and len(parts[0]) > 1 else re.sub(r"\s*\(.*$", "", head)
            sp = party_constant("SPECIES", sp)
            if not species_ok(ctx, sp) or sp in ctx.exempt_species:
                ctx.err(n, f"{path}:{lineno} SPECIES_{sp} is not a Gen 1 species")
    for t in sorted(opp - seen - {"TRAINER_NONE"}):
        ctx.err(n, f"{path}: {t} has no entry")


@check
def wild_encounters(ctx):
    n = "wild_encounters"
    path = "src/data/wild_encounters.json"
    data = json.loads(read(path))
    maps_known = {os.path.basename(p) for p in glob.glob(os.path.join(ROOT, "data/maps/*"))}
    for group in data["wild_encounter_groups"]:
        for enc in group["encounters"]:
            where = enc.get("base_label", enc.get("map"))
            for kind in ("land_mons", "water_mons", "rock_smash_mons", "fishing_mons"):
                if kind not in enc:
                    continue
                blk = enc[kind]
                if not 0 <= blk.get("encounter_rate", 0) <= 100:
                    ctx.err(n, f"{where} {kind} encounter_rate {blk.get('encounter_rate')}")
                for mon in blk["mons"]:
                    sp = mon["species"][len("SPECIES_"):]
                    if sp not in ctx.gen1_species:
                        ctx.err(n, f"{where} {kind}: SPECIES_{sp} is not a Gen 1 species")
                    if not 1 <= mon["min_level"] <= mon["max_level"] <= 100:
                        ctx.err(n, f"{where} {kind}: levels {mon['min_level']}..{mon['max_level']}")
            mp = enc["map"][len("MAP_"):]
            camel = "".join(w.capitalize() if w.isupper() else w for w in mp.split("_"))
            if not any(re.sub(r"[^a-z0-9]", "", d.lower()) == re.sub(r"[^a-z0-9]", "", mp.lower()) for d in maps_known):
                ctx.err(n, f"{where}: map {enc['map']} has no data/maps folder")


@check
def other_species_tables(ctx):
    n = "other_species_tables"
    files = sorted(glob.glob(os.path.join(ROOT, "src/data/battle_frontier/*.h")))
    # tutor_learnsets.h keeps its Gen 2/3 rows on purpose (array-index compatibility, CLAUDE.md), so it is not scanned
    files += [os.path.join(ROOT, p) for p in ("src/data/contest_opponents.h", "src/data/pokemon/egg_moves.h")]
    for path in files:
        if os.path.exists(path):
            scan_species_moves(ctx, n, os.path.relpath(path, ROOT))


@check
def evolution_table(ctx):
    n = "evolution_table"
    path = "src/data/pokemon/evolution.h"
    text = read(path)
    for m in re.finditer(r"\[SPECIES_(\w+)\]\s*=\s*\{(.*?)\},?\s*$", text, re.M):
        if m.group(1) in ctx.gen1_species:
            for to in re.findall(r"SPECIES_(\w+)", m.group(2)):
                if to not in ctx.gen1_species:
                    ctx.err(n, f"{path}: SPECIES_{m.group(1)} evolves into SPECIES_{to}")


# --- scripts and maps ----------------------------------------------------------------

REF_PREFIXES = ("TRAINER", "FLAG", "VAR", "ITEM", "SPECIES", "MOVE", "MAP", "SE", "MUS", "LAYOUT", "OBJ_EVENT_GFX",
                "MOVEMENT_TYPE", "TRAINER_TYPE", "MAPSEC")


def all_defined_names():
    names = set()
    for path in glob.glob(os.path.join(ROOT, "include/constants/*.h")):
        text = open(path, encoding="utf-8", errors="replace").read()
        names.update(re.findall(r"^\s*#define\s+(\w+)", text, re.M))
        for body in re.findall(r"enum[^{;]*\{(.*?)\};", text, re.S):
            names.update(re.findall(r"^\s*([A-Z][A-Z0-9_]+)\s*(?:=|,|$)", body, re.M))
    # TMs and HMs are built from the FOREACH_TM / FOREACH_HM X-macros
    tm = open(os.path.join(ROOT, "include/constants/tms_hms.h"), encoding="utf-8").read()
    for kind, macro in (("TM", "FOREACH_TM"), ("HM", "FOREACH_HM")):
        m = re.search(r"#define %s\(F\)(.*?)\n\n" % macro, tm, re.S)
        for mv in re.findall(r"F\((\w+)\)", m.group(1) if m else ""):
            names.add("ITEM_%s_%s" % (kind, mv))
            names.add("%s_%s" % (kind, mv))
    # map constants come from the map jsons
    for path in glob.glob(os.path.join(ROOT, "data/maps/*/map.json")):
        j = json.load(open(path))
        names.add(j["id"])
        names.add(j.get("layout", ""))
    layouts = os.path.join(ROOT, "data/layouts/layouts.json")
    if os.path.exists(layouts):
        for lay in json.load(open(layouts))["layouts"]:
            names.add(lay["id"])
    return names


# Names that are built by macros or generated, never written out in a header
GENERATED = re.compile(r"^(MAP_GROUP|MAP_NUM)\b|_COUNT$|_END$|_START$|^FLAG_(HIDE|TEMP)_|^SPECIES_.*_COUNT")


@check
def script_references(ctx):
    n = "script_references"
    defined = all_defined_names()
    # Dead Emerald systems still name removed maps; removed_maps.h defines them
    files = sorted(glob.glob(os.path.join(ROOT, "data/maps/*/scripts.pory")))
    files += sorted(glob.glob(os.path.join(ROOT, "data/scripts/*.pory")))
    files += sorted(glob.glob(os.path.join(ROOT, "data/maps/*/map.json")))
    pat = re.compile(r"\b((?:%s)_[A-Z0-9_]+)\b" % "|".join(REF_PREFIXES))
    for path in files:
        rel = os.path.relpath(path, ROOT)
        text = open(path, encoding="utf-8", errors="replace").read()
        text = re.sub(r"//[^\n]*", "", text)
        seen = set()
        for m in pat.finditer(text):
            name = m.group(1)
            if name in defined or name in seen or GENERATED.search(name):
                continue
            seen.add(name)
            line = text.count("\n", 0, m.start()) + 1
            ctx.err(n, f"{rel}:{line} {name} is not defined in include/constants")
        for sp in re.findall(r"SPECIES_(\w+)", text):
            if sp in ctx.species_id and not species_ok(ctx, sp):
                ctx.err(n, f"{rel}: SPECIES_{sp} is not a Gen 1 species")
        for mv in re.findall(r"MOVE_(\w+)", text):
            if mv in ctx.move_id and mv not in ctx.gen1_moves and mv not in ctx.exempt_moves:
                ctx.err(n, f"{rel}: MOVE_{mv} is not a Gen 1 move")


@check
def map_json_integrity(ctx):
    n = "map_json_integrity"
    maps = {}
    for path in glob.glob(os.path.join(ROOT, "data/maps/*/map.json")):
        j = json.load(open(path))
        maps[j["id"]] = os.path.relpath(path, ROOT)
    removed = defines("include/constants/removed_maps.h") if os.path.exists(os.path.join(ROOT, "include/constants/removed_maps.h")) else {}
    for mid, rel in sorted(maps.items()):
        j = json.load(open(os.path.join(ROOT, rel)))
        for w in j.get("warp_events", []):
            if w["dest_map"] not in maps and w["dest_map"] not in removed and w["dest_map"] != "MAP_DYNAMIC":
                ctx.err(n, f"{rel}: warp to unknown map {w['dest_map']}")
        for c in j.get("connections") or []:
            if c["map"] not in maps and c["map"] not in removed:
                ctx.err(n, f"{rel}: connection to unknown map {c['map']}")
        scripts = os.path.join(os.path.dirname(os.path.join(ROOT, rel)), "scripts.pory")
        if os.path.exists(scripts):
            text = open(scripts, encoding="utf-8").read()
            for ev in j.get("object_events", []):
                s = ev.get("script", "0x0")
                if s not in ("0x0", "NULL") and not re.search(r"\b%s\b" % re.escape(s), text) and \
                        s not in read("data/scripts/shared_scripts_names.txt") if os.path.exists(os.path.join(ROOT, "data/scripts/shared_scripts_names.txt")) else False:
                    ctx.err(n, f"{rel}: object script {s} not found")


KNOWN = os.path.join(ROOT, "tools", "check_data_known.json")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list", action="store_true", help="list the check names and exit")
    ap.add_argument("--only", help="run only this check")
    ap.add_argument("--update-known", action="store_true", help="rewrite tools/check_data_known.json from the current problems")
    ap.add_argument("--all", action="store_true", help="show every problem, including the known ones")
    args = ap.parse_args()
    if args.list:
        for c in CHECKS:
            print(c.__name__)
        return 0
    ctx = Ctx()
    ran = [c for c in CHECKS if not args.only or c.__name__ == args.only]
    per_check = {}
    for c in ran:
        before = len(ctx.errors)
        c(ctx)
        per_check[c.__name__] = ctx.errors[before:]

    counts, examples = {}, {}
    for name, msg in ctx.errors:
        k = Ctx.key(name, msg)
        counts[k] = counts.get(k, 0) + 1
        examples.setdefault(k, []).append(msg)

    if args.update_known:
        with open(KNOWN, "w") as f:
            json.dump(dict(sorted(counts.items())), f, indent=1)
            f.write("\n")
        print(f"wrote {len(counts)} known entries to {os.path.relpath(KNOWN, ROOT)}")
        return 0

    known = json.load(open(KNOWN)) if os.path.exists(KNOWN) else {}
    new_failures = 0
    for c in ran:
        keys = {Ctx.key(n, m) for n, m in per_check[c.__name__]}
        over = [k for k in keys if counts[k] > known.get(k, 0)]
        status = "FAIL" if over else ("known" if keys else "ok  ")
        extra = f"  ({sum(counts[k] for k in keys)} known problems)" if keys and not over else ""
        print(f"  {status}  {c.__name__}{extra}")
        new_failures += bool(over)
        for k in sorted(keys):
            if counts[k] > known.get(k, 0) or args.all:
                tag = "NEW" if counts[k] > known.get(k, 0) else "known"
                print(f"      [{tag}] {k}: {counts[k]} (allowed {known.get(k, 0)})")
                for msg in examples[k][:5]:
                    print(f"          {msg}")
    fixed = [k for k, v in known.items() if counts.get(k, 0) < v and (not args.only or k.split()[0] == args.only)]
    for k in fixed:
        print(f"  note: {k} is down to {counts.get(k, 0)} (allowed {known[k]}); run --update-known to lower the baseline")
    print(f"{len(ran) - new_failures} of {len(ran)} checks passed, {new_failures} failed; "
          f"known problems are listed in tools/check_data_known.json")
    return 1 if new_failures else 0


if __name__ == "__main__":
    sys.exit(main())
