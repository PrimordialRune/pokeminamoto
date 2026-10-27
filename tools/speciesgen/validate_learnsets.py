#!/usr/bin/env python3
"""Validate the level-up learnsets and the move names (studio data workstream).

    python3 tools/speciesgen/validate_learnsets.py [--base REF] [--endgame FILE] [-v]

Rules (docs of the endgame pass, studio/systems/learnsets-endgame.md):
  errors
    * every move named by a learnset exists in include/constants/moves.h and is a Gen 1 move (ids 1..164:
      no Gen 2/3 move, no Signature, no Struggle)
    * levels are in ascending order (1..100), at most 20 entries (MAX_LEVEL_UP_MOVES)
    * the Hyper Beam level is 41 or more; Fire Blast, Blizzard, Thunder, Solar Beam and Hydro Pump 32 or more
      (TP holds the strong moves back, the level gate is kept)
    * every species has a level-1 move, and every evolution target exists
    * Magnemite and Magneton are pure Electric (Gen 1)
    * move names fit MOVE_NAME_LENGTH (12) in the game charmap (every move of gMovesInfo, switch-on moves too)
  endgame set only (the species in --endgame, default studio/systems/learnsets-endgame.json)
    * the record's levelUpMoves equal the file's list (minus the MOVE_ prefix); no move twice in a list
    * a real attack by level 10 (direct hit of power 35+, or Night Shade, Dragon Rage, Sonic Boom, Comet Punch,
      Barrage, or a multi-hit move such as Double Kick), except Abra, Magikarp, Ditto and Mr. Mime
  warnings
    * no own-type attack by level 15 (the pass says "almost everyone")
  --base REF: the species not in the endgame set must be unchanged against that git ref in their learnset
  and evolution keys (the PR #26 chapter-1 species).
Exit status 1 if there is any error. Levels are PLACEHOLDER for the thread "Design movesets and encounters".
"""
import argparse
import glob
import json
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DEFAULT_ENDGAME = "/mnt/project-files/studio/systems/learnsets-endgame.json"
MAX_LEVEL_UP_MOVES = 20
MOVE_NAME_LENGTH = 12
GEN1_LAST = 164  # MOVE_STRUGGLE is 165 and not a learnset move
LATE = {"HYPER_BEAM": 41, "FIRE_BLAST": 32, "BLIZZARD": 32, "THUNDER": 32, "SOLAR_BEAM": 32, "HYDRO_PUMP": 32}
FIXED_DAMAGE = {"NIGHT_SHADE", "DRAGON_RAGE", "SONIC_BOOM", "COMET_PUNCH", "BARRAGE"}
MULTI_HIT_EFFECTS = {"EFFECT_MULTI_HIT", "EFFECT_DOUBLE_HIT", "EFFECT_TWINEEDLE"}  # two or more hits: Double Kick counts
EXEMPT_EARLY = {"ABRA", "MAGIKARP", "DITTO", "MR_MIME"}

# charmap.txt glyph count: every printable character is one byte, so the length is the character count
errors, warnings = [], []


def err(msg):
    errors.append(msg)


def warn(msg):
    warnings.append(msg)


def load_moves():
    ids = {}
    for m in re.finditer(r"#define MOVE_(\w+)\s+(\d+)\s*$", open(os.path.join(ROOT, "include/constants/moves.h")).read(), re.M):
        ids[m.group(1)] = int(m.group(2))
    info = {}
    text = open(os.path.join(ROOT, "src/data/moves_info.h")).read()
    for m in re.finditer(r"\[MOVE_(\w+)\]\s*=\s*\{(.*?)\n    \},", text, re.S):
        body = m.group(2)
        g = lambda k: (re.search(r"\.%s\s*=\s*([^,]+)," % k, body) or [None, None])[1]
        name = re.search(r'\.name\s*=\s*_\("((?:[^"\\]|\\.)*)"\)', body)
        info[m.group(1)] = {"power": int(g("power") or 0), "type": (g("type") or "").replace("TYPE_", ""),
                            "name": name.group(1) if name else None, "effect": g("effect")}
    return ids, info


def name_length(name):
    # escapes such as {PKMN} or \\n count as one character each in the game
    return len(re.sub(r"\{[^}]*\}|\\.", "X", name))


def check_names(ids, info):
    # the switch-on moves live inside #if blocks in moves.h and moves_info.h; they are parsed like the others
    for key, d in info.items():
        if d["name"] is None:
            err("move %s has no name" % key)
        elif name_length(d["name"]) > MOVE_NAME_LENGTH:
            err("move %s: name '%s' is %d characters, MOVE_NAME_LENGTH is %d"
                % (key, d["name"], name_length(d["name"]), MOVE_NAME_LENGTH))
        if key not in ids:
            err("move %s is in moves_info.h but has no MOVE_ constant" % key)
    for bad in ("SHELL_BASH",):
        if bad in ids:
            err("MOVE_%s exists: Omar's names are GREEDY PLANT and SKULL BASH" % bad)
    for key, d in info.items():
        if d["name"] and "SHELL BASH" in d["name"].upper():
            err("move %s is displayed as SHELL BASH: use Omar's SKULL BASH" % key)


def load_species():
    out = {}
    for p in sorted(glob.glob(os.path.join(ROOT, "src/data/species/*.json"))):
        j = json.load(open(p))
        out[j["species"]] = (p, j)
    return out


def check_learnset(sp, j, ids, info, endgame):
    moves = j.get("levelUpMoves", [])
    if not moves:
        err("%s: empty levelUpMoves" % sp)
        return
    if len(moves) > MAX_LEVEL_UP_MOVES:
        err("%s: %d entries, the engine cap is %d" % (sp, len(moves), MAX_LEVEL_UP_MOVES))
    seen, last = set(), 0
    for lvl, mv in moves:
        if mv not in ids:
            err("%s: move %s does not exist" % (sp, mv))
            continue
        if not 1 <= ids[mv] <= GEN1_LAST:
            err("%s: %s (id %d) is not a Gen 1 move" % (sp, mv, ids[mv]))
        if not 1 <= lvl <= 100:
            err("%s: %s at level %d" % (sp, mv, lvl))
        if lvl < last:
            err("%s: %s at level %d comes after level %d (level order)" % (sp, mv, lvl, last))
        last = max(last, lvl)
        if mv in seen and sp in endgame:  # the chapter-1 lists (PR #26) repeat moves on purpose, R/G style
            err("%s: %s appears twice" % (sp, mv))
        seen.add(mv)
        if mv in LATE and lvl < LATE[mv]:
            err("%s: %s at level %d, the gate is %d" % (sp, mv, lvl, LATE[mv]))
    if moves[0][0] != 1:
        err("%s: no level-1 move" % sp)
    if sp not in endgame:
        return
    want = [[l, m[5:] if m.startswith("MOVE_") else m] for l, m in endgame[sp]]
    if moves != want:
        err("%s: levelUpMoves differ from the endgame file" % sp)
    if sp not in EXEMPT_EARLY:
        early = [m for l, m in moves if l <= 10 and m in info and (info[m]["power"] >= 35 or m in FIXED_DAMAGE or info[m]["effect"] in MULTI_HIT_EFFECTS)]
        if not early:
            err("%s: no real attack by level 10" % sp)
    types = set(j["types"])
    own = [m for l, m in moves if l <= 15 and m in info and info[m]["power"] > 0 and info[m]["type"] in types]
    if not own and sp not in EXEMPT_EARLY:
        warn("%s: no own-type attack by level 15" % sp)


def check_base(base, species, endgame):
    for sp, (p, j) in species.items():
        if sp in endgame or sp in ("NONE", "EGG"):
            continue
        rel = os.path.relpath(p, ROOT)
        try:
            old = json.loads(subprocess.check_output(["git", "-C", ROOT, "show", "%s:%s" % (base, rel)],
                                                     stderr=subprocess.DEVNULL, text=True))
        except subprocess.CalledProcessError:
            continue
        for key in ("levelUpMoves", "evolutions"):
            if old.get(key) != j.get(key):
                err("%s is not in the endgame set but its %s changed against %s" % (sp, key, base))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--endgame", default=DEFAULT_ENDGAME)
    ap.add_argument("--base", help="git ref to compare the untouched species against")
    ap.add_argument("-v", action="store_true")
    a = ap.parse_args()
    ids, info = load_moves()
    species = load_species()
    endgame = json.load(open(a.endgame)) if os.path.exists(a.endgame) else {}
    if not endgame:
        warn("endgame file %s not found: only the general rules ran" % a.endgame)
    check_names(ids, info)
    names = set(species)
    for sp, (p, j) in species.items():
        if sp in ("NONE", "EGG"):
            continue
        check_learnset(sp, j, ids, info, endgame)
        for ev in j.get("evolutions", []):
            if ev["into"] not in names:
                err("%s: evolution into %s, no such species" % (sp, ev["into"]))
    for sp in ("MAGNEMITE", "MAGNETON"):
        if species[sp][1]["types"] != ["ELECTRIC", "ELECTRIC"]:
            err("%s must be pure Electric (Gen 1), has %s" % (sp, species[sp][1]["types"]))
    for sp in endgame:
        if sp not in species:
            err("endgame file names %s, no such species" % sp)
    if a.base:
        check_base(a.base, species, endgame)
    for w in warnings:
        print("warn:", w)
    for e in errors:
        print("ERROR:", e)
    print("validate_learnsets: %d species, %d endgame, %d errors, %d warnings"
          % (len(species) - 2, len(endgame), len(errors), len(warnings)))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
