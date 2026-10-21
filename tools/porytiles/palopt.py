#!/usr/bin/env python3
"""Find the fewest palettes a primary + secondary tileset pair can use, and write them.

Porytiles' own packers are heuristics. This solves the same problem exactly
with OR-Tools CP-SAT, then writes the palettes as JASC .pal files. Compile with
`porytiles compile-tileset --pals-edit-mode locked` afterwards and Porytiles
assigns every tile to these palettes instead of packing its own.

With --suggest-merges it also lists colors that, folded into a near neighbor,
would free a whole palette. Those are art decisions, so it only reports them.

What it minimizes, in order:
  1. palettes used by the primary (shared by every town, so the scarcest);
  2. palettes used by the secondary;
  3. colors repeated across palettes, so the free slots are bunched together.

Secondary tiles may use primary palettes, as they can in game.

Usage:
  palopt.py PRIMARY_SRC SECONDARY_SRC [--primary-out DIR] [--secondary-out DIR]
            [--keep-free-primary N] [--time SECONDS]

PRIMARY_SRC and SECONDARY_SRC are Porytiles source folders (bottom/middle/top.png).
Without --*-out it only reports. Pass a secondary folder that doesn't exist to
optimize a primary alone.

Needs: pip install ortools pillow
"""

import argparse
import os
import sys

from PIL import Image

try:
    from ortools.sat.python import cp_model
except ImportError:
    sys.exit("palopt.py needs OR-Tools: pip install ortools")

NUM_PALS_IN_PRIMARY = 6
NUM_PALS_TOTAL = 13
COLORS_PER_PAL = 15
MAGENTA = (255, 0, 255)


def read_sheets(src):
    """Color sets of every 8x8 tile in a Porytiles source folder (RGB555 keys)."""
    sets, eight_bit = [], {}
    for layer in ("bottom", "middle", "top"):
        path = os.path.join(src, layer + ".png")
        if not os.path.exists(path):
            continue
        im = Image.open(path).convert("RGBA")
        px = im.load()
        for ty in range(im.size[1] // 8):
            for tx in range(im.size[0] // 8):
                s = set()
                for y in range(8):
                    for x in range(8):
                        r, g, b, a = px[tx * 8 + x, ty * 8 + y]
                        if a == 0 or (r, g, b) == MAGENTA:
                            continue
                        k = (r >> 3, g >> 3, b >> 3)
                        s.add(k)
                        eight_bit.setdefault(k, (r, g, b))
                if s:
                    sets.append(frozenset(s))
    return sets, eight_bit


def maximal(sets):
    """Drop color sets contained in another: they fit wherever the bigger one goes."""
    out = []
    for s in sorted(set(sets), key=len, reverse=True):
        if not any(s <= t for t in out):
            out.append(s)
    return out


def solve(prim, sec, keep_free, seconds, tidy=True):
    n_prim = NUM_PALS_IN_PRIMARY - keep_free
    n_sec = NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY
    sec = [s for s in sec if not any(s <= p for p in prim)]
    colors = sorted(set().union(*prim, *sec)) if (prim or sec) else []
    pals = list(range(n_prim)) + list(range(NUM_PALS_IN_PRIMARY, NUM_PALS_IN_PRIMARY + n_sec))

    md = cp_model.CpModel()
    y = {(c, p): md.NewBoolVar("") for c in colors for p in pals}
    used = {p: md.NewBoolVar("") for p in pals}

    def place(sets, allowed):
        for s in sets:
            xs = []
            for p in allowed:
                x = md.NewBoolVar("")
                xs.append(x)
                for c in s:
                    md.AddImplication(x, y[(c, p)])
            md.AddExactlyOne(xs)

    place(prim, pals[:n_prim])
    place(sec, pals)
    for p in pals:
        md.Add(sum(y[(c, p)] for c in colors) <= COLORS_PER_PAL)
        for c in colors:
            md.AddImplication(y[(c, p)], used[p])
    # Fill palettes in order, so the unused ones are the last ones.
    for group in (pals[:n_prim], pals[n_prim:]):
        for a, b in zip(group, group[1:]):
            md.AddImplication(used[b], used[a])

    big = len(colors) * len(pals) + 1
    md.Minimize(big * big * sum(used[p] for p in pals[:n_prim])
                + big * sum(used[p] for p in pals[n_prim:])
                + (sum(y.values()) if tidy else 0))

    sv = cp_model.CpSolver()
    sv.parameters.max_time_in_seconds = seconds
    sv.parameters.num_workers = 8
    status = sv.Solve(md)
    if status not in (cp_model.OPTIMAL, cp_model.FEASIBLE):
        return None, sv.StatusName(status)
    result = {p: sorted(c for c in colors if sv.Value(y[(c, p)])) for p in pals}
    return {p: cs for p, cs in result.items() if cs}, sv.StatusName(status)


def count(pals):
    n_p = sum(1 for p in pals if p < NUM_PALS_IN_PRIMARY)
    return n_p, len(pals) - n_p


def suggest_merges(prim_sets, sec_sets, eight_bit, keep_free, base, seconds):
    """Try folding each color into its two nearest colors; list merges that free a palette."""
    colors = sorted(set().union(*prim_sets, *sec_sets))

    def dist(a, b):
        return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5

    found = []
    for c in colors:
        for o in sorted((o for o in colors if o != c), key=lambda o: dist(c, o))[:2]:
            def fold(sets):
                return maximal([frozenset(o if x == c else x for x in s) for s in sets])
            pals, _ = solve(fold(prim_sets), fold(sec_sets), keep_free, seconds, tidy=False)
            if pals and sum(count(pals)) < sum(base):
                found.append((count(pals), dist(c, o), c, o))
    print("\nColor merges that free a palette (primary, secondary after the merge):")
    if not found:
        print("  none")
    for (n_p, n_s), d, c, o in sorted(found, key=lambda t: (sum(t[0]), t[1])):
        print(f"  {n_p}+{n_s}  {eight_bit[c]} -> {eight_bit[o]}  (distance {d:.1f} in 5-bit steps)")


def write_pal(path, colors, eight_bit):
    rows = [MAGENTA] + [eight_bit[c] for c in colors]
    rows += [(0, 0, 0)] * (16 - len(rows))
    with open(path, "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n16\n")
        for r, g, b in rows:
            f.write(f"{r} {g} {b}\n")


def write_hints(path, pals, eight_bit, wanted):
    """Porytiles config with one palette hint per solved palette."""
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)
    lines = ["# Written by palopt.py. Each hint is one palette of the optimal layout.",
             "tileset:", "  palettes:", "    packing:", "      hints_enabled: true", "      hints:"]
    for p in sorted(pals):
        if p not in wanted:
            continue
        lines.append(f'        - name: "pal{p:02d}"')
        lines.append("          colors:")
        lines += [f"            - [ {r}, {g}, {b} ]" for r, g, b in (eight_bit[c] for c in pals[p])]
    with open(path, "w") as f:
        f.write("\n".join(lines) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("primary_src")
    ap.add_argument("secondary_src")
    ap.add_argument("--primary-out", help="porytiles_bin/palettes folder of the primary")
    ap.add_argument("--secondary-out", help="porytiles_bin/palettes folder of the secondary")
    ap.add_argument("--primary-config", help="porytiles/tilesets/<primary>/config.yaml to write palette hints to")
    ap.add_argument("--secondary-config", help="porytiles/tilesets/<secondary>/config.yaml to write palette hints to")
    ap.add_argument("--keep-free-primary", type=int, default=0,
                    help="leave this many primary palettes untouched for later towns")
    ap.add_argument("--time", type=float, default=120)
    ap.add_argument("--suggest-merges", action="store_true",
                    help="also list single color merges that would free a palette (slow: minutes)")
    args = ap.parse_args()

    prim_sets, eb1 = read_sheets(args.primary_src)
    sec_sets, eb2 = (read_sheets(args.secondary_src) if os.path.isdir(args.secondary_src) else ([], {}))
    eight_bit = {**eb2, **eb1}
    prim, sec = maximal(prim_sets), maximal(sec_sets)

    pals, status = solve(prim, sec, args.keep_free_primary, args.time)
    if pals is None:
        sys.exit(f"No palette layout fits ({status}). Too many colors per tile group; see report.md.")

    n_p, n_s = count(pals)
    print(f"{status}: primary uses {n_p} of {NUM_PALS_IN_PRIMARY} palettes, "
          f"secondary uses {n_s} of {NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY}.")
    if status != "OPTIMAL":
        print("(time ran out before it was proven minimal; rerun with a larger --time)")
    for p, cs in sorted(pals.items()):
        print(f"  {p:02d} {'primary  ' if p < NUM_PALS_IN_PRIMARY else 'secondary'} {len(cs):2d}/15 colors")

    if args.suggest_merges:
        suggest_merges(prim, sec, eight_bit, args.keep_free_primary, count(pals), 10)

    # A Porytiles secondary keeps a copy of its primary's palettes in 00-05.
    prim_range = range(NUM_PALS_IN_PRIMARY - args.keep_free_primary)
    if args.primary_out:
        for p in prim_range:
            write_pal(os.path.join(args.primary_out, f"{p:02d}.pal"), pals.get(p, []), eight_bit)
    if args.primary_config:
        write_hints(args.primary_config, pals, eight_bit, range(NUM_PALS_IN_PRIMARY))
    if args.secondary_config:
        write_hints(args.secondary_config, pals, eight_bit, range(NUM_PALS_IN_PRIMARY, NUM_PALS_TOTAL))
    if args.secondary_out:
        for p in list(prim_range) + list(range(NUM_PALS_IN_PRIMARY, NUM_PALS_TOTAL)):
            write_pal(os.path.join(args.secondary_out, f"{p:02d}.pal"), pals.get(p, []), eight_bit)


if __name__ == "__main__":
    main()
