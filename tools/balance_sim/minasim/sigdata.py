"""Which species have a real Signature in the repo's data (src/data/pokemon/special_moves.h), for the gate's Signature check.

The file is read with a small preprocessor pass: `#ifdef X ... #endif` blocks count only when X is `#define`d (uncommented)
in include/config.h, so the default build lists the Charmander line and Farfetch'd, and a STARTER_SIGNATURES_PROPOSAL build
adds Bulbasaur's and Squirtle's. The host library must be rebuilt (`make balance-lib`) for the engine to match.
"""
import re

from .gamedata import _read, Game


def config_defines():
    out = set()
    for line in _read("include/config.h").splitlines():
        m = re.match(r"\s*#\s*define\s+(\w+)", line)
        if m:
            out.add(m.group(1))
    return out


def signatures(defines=None):
    """{SPECIES const: MOVE const} of every species whose Special names a move."""
    defines = config_defines() if defines is None else defines
    macros, cur, out = {}, None, {}
    stack = []                                   # active flags of nested #if blocks
    for raw in _read("src/data/pokemon/special_moves.h").splitlines():
        line = raw.strip()
        m = re.match(r"#\s*(ifdef|ifndef)\s+(\w+)", line)
        if m:
            stack.append((m.group(2) in defines) == (m.group(1) == "ifdef"))
            continue
        if re.match(r"#\s*else", line) and stack:
            stack[-1] = not stack[-1]
            continue
        if re.match(r"#\s*endif", line) and stack:
            stack.pop()
            continue
        if not all(stack):
            continue
        m = re.match(r"#define\s+(\w+)\b", line)
        if m:
            cur = m.group(1)
            macros[cur] = ""
        if cur is not None:
            macros[cur] += line
            if not raw.rstrip().endswith("\\"):
                cur = None
            continue
        m = re.match(r"\[SPECIES_(\w+)\]\s*=\s*(\w+)\s*,", line)
        if m and m.group(2) in macros:
            mv = re.search(r"\.move\s*=\s*MOVE_(\w+)", macros[m.group(2)])
            if mv:
                out[m.group(1)] = mv.group(1)
            continue
        m = re.match(r"\[SPECIES_(\w+)\]\s*=\s*\{.*?\.move\s*=\s*MOVE_(\w+)", line)
        if m:
            out[m.group(1)] = m.group(2)
    return out


def base_species():
    """Base forms of the lines that have a Signature (an awakened mon keeps its seal open through evolution)."""
    g = Game.get()
    return {g.base_form(sp) for sp in signatures() if sp in g.species}
