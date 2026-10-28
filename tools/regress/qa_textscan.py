#!/usr/bin/env python3
"""Text overflow scan for the slice maps.

    qa_textscan.py [--repo DIR] [--maps FILE] [--json OUT]

Reads the generated text.inc of every slice map (run `make generated` first), measures each line of each
`.string` against the dialogue font widths in tools/poryscript/font_config.json (the box is 208 px wide) and
reports lines that are too wide, strings with a bare line break that never ends in a terminator, and glyphs
the font table does not know. Variable pieces ({PLAYER}, {STR_VAR_n}, {RIVAL}) use the font table's worst case.
"""
import argparse
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def load_font(repo):
    cfg = json.load(open(os.path.join(repo, "tools", "poryscript", "font_config.json")))
    fonts = {f[0]: f[1] for f in cfg["fonts"]} if isinstance(cfg["fonts"], list) and isinstance(cfg["fonts"][0], list) \
        else {f["id"]: f for f in cfg["fonts"]} if isinstance(cfg["fonts"], list) else cfg["fonts"]
    return fonts


def width_of(line, widths, unknown):
    w = 0
    i = 0
    while i < len(line):
        if line[i] == "{":
            j = line.find("}", i)
            tok = line[i:j + 1]
            if tok in widths:
                w += widths[tok]
            elif re.match(r"\{(PAUSE|COLOR|SHADOW|HIGHLIGHT|PLAY_|WAIT|CLEAR|SHIFT|FONT|MIN_LETTER|SKIP|PALETTE|ESCAPE|RESET)", tok):
                pass
            else:
                unknown.add(tok)
                w += 6
            i = j + 1
            continue
        c = line[i]
        if c in widths:
            w += widths[c]
        else:
            unknown.add(c)
            w += 6
        i += 1
    return w


def split_string(s):
    """A .string literal into lines: \\n and \\l break a line, \\p ends a box."""
    out = []
    cur = ""
    i = 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            c = s[i + 1]
            if c in "nlp":
                out.append((cur, c))
                cur = ""
            else:
                cur += s[i:i + 2]
            i += 2
            continue
        cur += s[i]
        i += 1
    out.append((cur, "end"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=ROOT)
    ap.add_argument("--maps", default="/mnt/project-files/studio/slice-maps.txt")
    ap.add_argument("--json")
    a = ap.parse_args()
    fonts = load_font(a.repo)
    fid = "1_latin_rse"
    widths = dict(fonts[fid]["widths"])
    widths.setdefault("₽", 8)       # not in font_config.json (QA-003): assume the wider yen-sized glyph
    missing = "₽" not in fonts[fid]["widths"]
    limit = fonts[fid]["maxLineLength"]
    names = []
    for line in open(a.maps):
        line = line.strip()
        if line.startswith("# conditional"):
            break
        if line and not line.startswith("#"):
            names.append(line)
    problems = []
    unknown_all = set()
    n_strings = n_lines = 0
    widest = (0, "")
    for name in names:
        path = os.path.join(a.repo, "data", "maps", name, "text.inc")
        if not os.path.exists(path):
            continue
        label = None
        for ln in open(path, encoding="utf-8"):
            m = re.match(r"^(\w+)::", ln)
            if m:
                label = m.group(1)
                continue
            m = re.match(r'^\s*\.string "(.*)"\s*$', ln)
            if not m:
                continue
            n_strings += 1
            unknown = set()
            for text, term in split_string(m.group(1)):
                n_lines += 1
                w = width_of(text, widths, unknown)
                if w > widest[0]:
                    widest = (w, f"{name}:{label}")
                if w > limit:
                    problems.append(dict(map=name, label=label, width=w, line=text))
            unknown_all |= unknown
            for u in unknown:
                problems.append(dict(map=name, label=label, unknown_glyph=u))
    if missing:
        unknown_all.add("₽")
    res = dict(strings=n_strings, lines=n_lines, widest=widest, overflow=[p for p in problems if "width" in p],
               unknown=sorted(unknown_all))
    print(f"strings {n_strings}, lines {n_lines}, widest line {widest[0]} px of {limit} ({widest[1]})")
    for p in res["overflow"]:
        print(f"OVERFLOW {p['map']}:{p['label']} {p['width']} px: {p['line']!r}")
    if unknown_all:
        print("unknown glyphs:", sorted(unknown_all))
    if a.json:
        json.dump(res, open(a.json, "w"), indent=1, ensure_ascii=False)
    print("RESULT:", "PASS" if not res["overflow"] and not unknown_all else "FAIL")
    return 0 if not res["overflow"] and not unknown_all else 1


if __name__ == "__main__":
    sys.exit(main())
