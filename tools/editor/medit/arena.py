"""Arena knobs: the `gArenaTuning` initializer in src/arena_ecology.c (the table the Balance Lab sweeps).

Only the knob lines are parsed; every other line of the C file, and each knob's comment, is kept verbatim.
A knob whose value is unchanged keeps its original expression (`ARENA_BLOCK_STRING_SHOT | ...`).
"""
import re

from . import paths

SYMBOLS = {"FALSE": 0, "TRUE": 1, "ARENA_FOR_REST_OF_BATTLE": 0xFF, "ARENA_BLOCK_STRING_SHOT": 1,
           "ARENA_BLOCK_OBSTRUCT": 2, "ARENA_BLOCK_VINES": 4, "ARENA_BLOCK_SAND": 8}
BLOCK_NAMES = [("ARENA_BLOCK_STRING_SHOT", 1), ("ARENA_BLOCK_OBSTRUCT", 2), ("ARENA_BLOCK_VINES", 4), ("ARENA_BLOCK_SAND", 8)]
START_RE = re.compile(r"^const struct ArenaTuning gArenaTuning\s*=\s*$")
KNOB_RE = re.compile(r"^(\s*)\.(\w+)(\s*=\s*)(.*?),(\s*)(//.*)?$")
DECL_RE = re.compile(r"^\s*u16\s+(\w+);\s*(?://\s*(.*))?$")


def evaluate(expr):
    total = 0
    for part in expr.split("|"):
        part = part.strip()
        if part in SYMBOLS:
            total |= SYMBOLS[part]
        elif re.fullmatch(r"(0x[0-9A-Fa-f]+|\d+)", part):
            total |= int(part, 0)
        else:
            raise ValueError("cannot evaluate %r" % expr)
    return total


def expr_for(name, value, old_expr):
    """Spell `value` like the file does: TRUE/FALSE for flags, ARENA_BLOCK_* for the mask, else a number."""
    if old_expr in ("TRUE", "FALSE"):
        return "TRUE" if value else "FALSE"
    if name == "ventBlockers":
        parts = [n for n, bit in BLOCK_NAMES if value & bit]
        return " | ".join(parts) if parts and value == sum(b for n, b in BLOCK_NAMES if value & b) else str(value)
    if name == "ventBlockTurns" and value == 0xFF:
        return "ARENA_FOR_REST_OF_BATTLE"
    return str(value)


class ArenaKnobs:
    def __init__(self, store):
        self.store = store
        self.text = store.read(paths.ARENA)
        self.lines = self.text.split("\n")
        self.knobs = []                 # dicts: name, value, expr, line index
        start = next(i for i, l in enumerate(self.lines) if START_RE.match(l))
        i = start + 2
        while not self.lines[i].startswith("}"):
            m = KNOB_RE.match(self.lines[i])
            if m:
                self.knobs.append({"name": m.group(2), "expr": m.group(4), "value": evaluate(m.group(4)),
                                   "comment": (m.group(6) or "").lstrip("/ ").strip(), "line": i, "orig": m.group(4)})
            i += 1
        self.doc = self._header_docs()

    def _header_docs(self):
        """name -> the comment next to the field in include/arena_ecology.h (what the knob does)."""
        out = {}
        try:
            for l in self.store.read_repo("include/arena_ecology.h").split("\n"):
                m = DECL_RE.match(l)
                if m:
                    out[m.group(1)] = m.group(2) or ""
        except OSError:
            pass
        return out

    def to_json(self):
        return [{"name": k["name"], "value": k["value"], "comment": k["comment"], "doc": self.doc.get(k["name"], ""),
                 "default": evaluate(k["orig"])} for k in self.knobs]

    def values(self):
        return {k["name"]: k["value"] for k in self.knobs}

    def set_values(self, values):
        by = {k["name"]: k for k in self.knobs}
        for n, v in values.items():
            if n not in by:
                raise KeyError("unknown arena knob %s" % n)
            by[n]["value"] = int(v)

    def render(self):
        lines = list(self.lines)
        for k in self.knobs:
            if k["value"] == evaluate(k["orig"]):
                continue
            m = KNOB_RE.match(lines[k["line"]])
            new = expr_for(k["name"], k["value"], k["orig"])
            lines[k["line"]] = "%s.%s%s%s,%s%s" % (m.group(1), k["name"], m.group(3), new, m.group(5), m.group(6) or "")
        return "\n".join(lines)

    def save(self):
        return self.store.write(paths.ARENA, self.render())
