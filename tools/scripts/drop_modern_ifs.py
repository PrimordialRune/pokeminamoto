#!/usr/bin/env python3
"""Remove the classic-agbcc branches guarded by MODERN, keeping the modern code.

The classic build is gone, so MODERN is always 1. This rewrites, in src/ include/ asm/
data/*.s (and libagbsyscall):

    #if MODERN ... #else A #endif            -> body only
    #if !MODERN A #else B #endif             -> B
    #if MODERN || X                          -> body only (always true)
    #if MODERN && X                          -> #if X
    #if X / #elif MODERN ... (not supported, reported)
    .if MODERN ... .endif                    -> body only (assembler)
    (MODERN ? a : b)                         -> a

Re-runnable and idempotent: run it again after merging branches that may have added new
MODERN conditionals.   tools/scripts/drop_modern_ifs.py [--check] [paths...]
--check only counts and exits 1 if anything is left to rewrite.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_DIRS = ["src", "include", "asm", "data", "libagbsyscall"]
EXTS = (".c", ".h", ".s", ".inc")

IF_RE = re.compile(r"^(\s*)#\s*if\s+(.*?)\s*(//.*|/\*.*\*/)?\s*$")
IF_ANY = re.compile(r"^\s*#\s*(if|ifdef|ifndef)\b")
ELIF_RE = re.compile(r"^(\s*)#\s*elif\b(.*)$")
ELSE_RE = re.compile(r"^\s*#\s*else\b")
ENDIF_RE = re.compile(r"^\s*#\s*endif\b")
ASM_IF = re.compile(r"^\s*\.if\s+MODERN\b")
ASM_ELSE = re.compile(r"^\s*\.else\b")
ASM_ENDIF = re.compile(r"^\s*\.endif\b")
TERNARY = re.compile(r"\(MODERN\s*\?\s*([^:()]+?)\s*:\s*([^()]+?)\)")


class Unsupported(Exception):
    pass


def simplify(cond):
    """Return ('true',) ('false',) or ('keep', newcond) for a condition mentioning MODERN."""
    c = cond.strip()
    if c == "MODERN":
        return ("true",)
    if c == "!MODERN":
        return ("false",)
    m = re.fullmatch(r"MODERN\s*\|\|\s*(.+)", c)
    if m:
        return ("true",)
    m = re.fullmatch(r"MODERN\s*&&\s*(.+)", c)
    if m:
        return ("keep", m.group(1))
    raise Unsupported(cond)


def process_c(lines):
    """Process a C-preprocessor file. Returns (new_lines, count)."""
    out, count, i, n = [], 0, 0, len(lines)

    def find_branches(start):
        # lines[start] is a #if. Return list of (kind, index) for its #elif/#else and the #endif index.
        depth, marks, j = 0, [], start + 1
        while j < n:
            l = lines[j]
            if IF_ANY.match(l):
                depth += 1
            elif ENDIF_RE.match(l):
                if depth == 0:
                    return marks, j
                depth -= 1
            elif depth == 0 and (ELSE_RE.match(l) or ELIF_RE.match(l)):
                marks.append(j)
            j += 1
        raise Unsupported("unterminated #if at line %d" % (start + 1))

    def block(lo, hi):
        nonlocal count
        res, k = [], lo
        while k < hi:
            l = lines[k]
            m = IF_RE.match(l)
            if m and re.search(r"\bMODERN\b", m.group(2)):
                count += 1
                marks, end = find_branches(k)
                res_kind = simplify(m.group(2))
                # segments: [(header_index, body_lo, body_hi)]
                heads = [k] + marks
                segs = []
                for idx, h in enumerate(heads):
                    segs.append((h, h + 1, (heads[idx + 1] if idx + 1 < len(heads) else end)))
                if res_kind[0] == "true":
                    res += block(segs[0][1], segs[0][2])
                elif res_kind[0] == "false":
                    if len(segs) == 1:
                        pass
                    else:
                        h, blo, bhi = segs[1]
                        if ELSE_RE.match(lines[h]):
                            res += block(blo, bhi)
                        else:  # #elif X -> #if X
                            em = ELIF_RE.match(lines[h])
                            res.append("%s#if%s\n" % (em.group(1), em.group(2)))
                            rest = lines[:]  # re-run on a synthetic sub-range
                            res += block(blo, end)
                            res.append(lines[end])
                            k = end + 1
                            continue
                else:
                    res.append("%s#if %s\n" % (m.group(1), res_kind[1]))
                    res += block(segs[0][1], segs[0][2])
                    for h, blo, bhi in segs[1:]:
                        res.append(lines[h])
                        res += block(blo, bhi)
                    res.append(lines[end])
                k = end + 1
                continue
            if re.match(r"^\s*#\s*(elif|if|ifdef|ifndef)\b.*\bMODERN\b", l):
                raise Unsupported("line %d: %s" % (k + 1, l.strip()))
            res.append(l)
            k += 1
        return res

    out = block(0, n)
    return out, count


def process_s(lines):
    out, count, i = [], 0, 0
    stack = []  # for nested .if: each entry True if it is a MODERN if we are unwrapping
    skip = False
    for l in lines:
        if ASM_IF.match(l):
            stack.append("m")
            count += 1
            continue
        if re.match(r"^\s*\.if", l):
            stack.append("o")
        elif ASM_ELSE.match(l) and stack and stack[-1] == "m":
            skip = True
            continue
        elif ASM_ENDIF.match(l) and stack:
            if stack.pop() == "m":
                skip = False
                continue
        if not skip:
            out.append(l)
    return out, count


def process(path):
    with open(path, encoding="utf-8", errors="surrogateescape", newline="") as f:
        text = f.read()
    lines = text.splitlines(keepends=True)
    if path.endswith((".c", ".h")):
        new, n = process_c(lines)
        t = 0
        def sub(m):
            nonlocal t
            t += 1
            return m.group(1)
        joined = TERNARY.sub(sub, "".join(new))
        n += t
    else:
        new, n = process_s(lines)
        joined = "".join(new)
    return text, joined, n


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    check = "--check" in sys.argv
    paths = args or [os.path.join(ROOT, d) for d in DEFAULT_DIRS]
    total = 0
    files = []
    for p in paths:
        if os.path.isfile(p):
            files.append(p)
            continue
        for dp, dn, fn in os.walk(p):
            for f in fn:
                if f.endswith(EXTS):
                    files.append(os.path.join(dp, f))
    for path in sorted(files):
        try:
            with open(path, encoding="utf-8", errors="surrogateescape") as f:
                if "MODERN" not in f.read():
                    continue
        except OSError:
            continue
        old, new, n = process(path)
        if n == 0:
            continue
        total += n
        print("%3d  %s" % (n, os.path.relpath(path, ROOT)))
        if not check and old != new:
            with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
                f.write(new)
    print("total MODERN conditionals %s: %d" % ("found" if check else "removed", total))
    return 1 if (check and total) else 0


if __name__ == "__main__":
    sys.exit(main())
