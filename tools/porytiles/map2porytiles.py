#!/usr/bin/env python3
"""Turn a flattened map mockup PNG into Porytiles source sheets plus a map.bin.

The map image is cut into 16x16 metatiles. Identical metatiles are merged.
Each metatile is sent to the primary or the secondary tileset, then written to
that tileset's bottom.png (middle.png and top.png stay empty, so everything is
one ground layer). Porytiles then does the palette packing when it compiles.

Primary vs secondary:
  --secondary-ref SHEET  a sheet with transparent background (like the building
                         sheet). A metatile goes to the secondary if any of its
                         8x8 tiles appears, opaque, in that sheet.
  --split-mask MASK      a PNG the same size as the map. Paint a 16x16 cell red
                         (255,0,0) to force it into the secondary, blue (0,0,255)
                         to force it into the primary. Other colors are ignored.
  With neither, everything goes to the primary.

Outputs, in --out:
  primary/{bottom,middle,top}.png, primary/attributes.csv
  secondary/{bottom,middle,top}.png, secondary/attributes.csv
  map.bin          layout for data/layouts/<Name>/map.bin (elevation 3, passable)
  report.md        tile, metatile and color budgets against pokeemerald limits
  overlay.png      the map with over-budget trouble marked (see report)

Behaviors:
  --behaviors CSV        x,y,MB_NAME per cell (behaviors.py writes it); two cells
                         that look the same but have different behaviors become
                         separate metatiles. Cells left out are MB_NORMAL.

Usage:
  map2porytiles.py MAP.png --out DIR [--secondary-ref SHEET.png] [--split-mask MASK.png]
                   [--behaviors CSV]
"""

import argparse
import os
import struct
import sys
from collections import Counter, defaultdict

from PIL import Image, ImageDraw

# pokeemerald include/fieldmap.h
NUM_TILES_IN_PRIMARY = 512
NUM_TILES_TOTAL = 1024
NUM_METATILES_IN_PRIMARY = 512
NUM_METATILES_TOTAL = 1024
NUM_PALS_IN_PRIMARY = 6
NUM_PALS_TOTAL = 14
MAX_MAP_DATA_SIZE = 10240
COLORS_PER_PAL = 15  # index 0 is transparent

MAGENTA = (255, 0, 255, 255)  # Porytiles default extrinsic transparency
SHEET_WIDTH_METATILES = 8     # Porytiles sheets are 128 px wide
DEFAULT_ELEVATION = 3


def rgb555(px):
    """Snap an RGBA pixel to what the GBA can show; None for transparent."""
    r, g, b, a = px
    if a == 0 or (r, g, b, a) == MAGENTA:
        return None
    return (r >> 3, g >> 3, b >> 3)


def load_rgba(path):
    im = Image.open(path).convert("RGBA")
    return im, im.load()


def subtile(px, x0, y0):
    return tuple(rgb555(px[x0 + x, y0 + y]) for y in range(8) for x in range(8))


def flip_canon(t):
    """Same key for a tile and its H/V/HV flips (the GBA flips subtiles for free)."""
    rows = [t[i * 8:(i + 1) * 8] for i in range(8)]
    h = tuple(c for r in rows for c in r[::-1])
    v = tuple(c for r in rows[::-1] for c in r)
    hv = tuple(c for r in rows[::-1] for c in r[::-1])
    return min((t, h, v, hv), key=repr)


def colors_of(t):
    return frozenset(c for c in t if c is not None)


def estimate_palettes(color_sets):
    """Greedy packing estimate. Porytiles' backtracking search does as well or better."""
    sets = sorted(set(s for s in color_sets if s), key=len, reverse=True)
    maximal = []
    for s in sets:
        if not any(s <= m for m in maximal):
            maximal.append(s)
    bins = []
    for s in maximal:
        best = None
        for i, b in enumerate(bins):
            u = b | s
            if len(u) <= COLORS_PER_PAL and (best is None or len(u) - len(b) < best[0]):
                best = (len(u) - len(b), i)
        if best:
            bins[best[1]] |= s
        else:
            bins.append(set(s))
    return bins


def write_sheets(metatiles, src_px, outdir, behaviors=()):
    os.makedirs(outdir, exist_ok=True)
    rows = max(1, (len(metatiles) + SHEET_WIDTH_METATILES - 1) // SHEET_WIDTH_METATILES)
    size = (SHEET_WIDTH_METATILES * 16, rows * 16)
    bottom = Image.new("RGBA", size, MAGENTA)
    bpx = bottom.load()
    for i, (mx, my) in enumerate(metatiles):
        ox, oy = (i % SHEET_WIDTH_METATILES) * 16, (i // SHEET_WIDTH_METATILES) * 16
        for y in range(16):
            for x in range(16):
                p = src_px[mx * 16 + x, my * 16 + y]
                bpx[ox + x, oy + y] = p if p[3] else MAGENTA
    bottom.save(os.path.join(outdir, "bottom.png"))
    for layer in ("middle", "top"):
        Image.new("RGBA", size, MAGENTA).save(os.path.join(outdir, layer + ".png"))
    with open(os.path.join(outdir, "attributes.csv"), "w") as f:
        f.write("id,behavior\n")
        for i, b in enumerate(behaviors):
            if b != "MB_NORMAL":
                f.write(f"{i},{b}\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("map")
    ap.add_argument("--out", required=True)
    ap.add_argument("--secondary-ref")
    ap.add_argument("--split-mask")
    ap.add_argument("--behaviors")
    args = ap.parse_args()

    cell_behavior = {}
    if args.behaviors:
        for line in open(args.behaviors).read().split()[1:]:
            x, y, b = line.split(",")
            cell_behavior[(int(x), int(y))] = b

    im, px = load_rgba(args.map)
    W, H = im.size
    if W % 16 or H % 16:
        sys.exit(f"{args.map}: {W}x{H} is not a multiple of 16")
    mw, mh = W // 16, H // 16

    ref_tiles = set()
    if args.secondary_ref:
        rim, rpx = load_rgba(args.secondary_ref)
        for ty in range(rim.size[1] // 8):
            for tx in range(rim.size[0] // 8):
                t = subtile(rpx, tx * 8, ty * 8)
                if all(c is not None for c in t) and len(colors_of(t)) > 1:
                    ref_tiles.add(flip_canon(t))
    mask_px = None
    if args.split_mask:
        mim, mask_px = load_rgba(args.split_mask)
        if mim.size != im.size:
            sys.exit("split mask must be the same size as the map")

    def forced(mx, my):
        if mask_px is None:
            return None
        cells = Counter(mask_px[mx * 16 + x, my * 16 + y][:3] for y in range(16) for x in range(16))
        if cells[(255, 0, 0)]:
            return "secondary"
        if cells[(0, 0, 255)]:
            return "primary"
        return None

    # Unique metatiles, in reading order of first appearance.
    meta_key = {}
    meta_first = []
    cell_meta = {}
    for my in range(mh):
        for mx in range(mw):
            subs = tuple(subtile(px, mx * 16 + sx * 8, my * 16 + sy * 8) for sy in (0, 1) for sx in (0, 1))
            key = (subs, cell_behavior.get((mx, my), "MB_NORMAL"))
            if key not in meta_key:
                meta_key[key] = len(meta_first)
                meta_first.append(((mx, my), subs))
            cell_meta[(mx, my)] = meta_key[key]

    # Primary/secondary decision per unique metatile. A metatile used in several
    # places goes to the secondary if any of its cells is forced there.
    side = {}
    for (mx, my), mid in cell_meta.items():
        f = forced(mx, my)
        if f:
            if side.get(mid) != "secondary":
                side[mid] = f
    for mid, ((mx, my), subs) in enumerate(meta_first):
        if mid in side:
            continue
        side[mid] = "secondary" if any(flip_canon(t) in ref_tiles for t in subs) else "primary"

    # Edges of buildings (roof against grass, fences, mailboxes) don't match the
    # reference sheet exactly. Send a cell next to a secondary cell there too if
    # it uses a color the open ground away from buildings never uses.
    if ref_tiles:
        cell_colors = {c: set().union(*(colors_of(t) for t in meta_first[mid][1])) for c, mid in cell_meta.items()}
        sec_cells = {c for c, mid in cell_meta.items() if side[mid] == "secondary"}
        near = {(x + dx, y + dy) for x, y in sec_cells for dx in (-1, 0, 1) for dy in (-1, 0, 1)}
        ground = set().union(*(cols for c, cols in cell_colors.items() if c not in near))
        for c, mid in cell_meta.items():
            if c in near and forced(*c) is None and cell_colors[c] - ground:
                side[mid] = "secondary"

    groups = {"primary": [], "secondary": []}
    local_id = {}
    for mid, (pos, subs) in enumerate(meta_first):
        local_id[mid] = len(groups[side[mid]])
        groups[side[mid]].append(mid)

    # Tile budgets. Secondary metatiles can reuse primary tiles, so only tiles
    # not already in the primary count against the secondary.
    tiles = {"primary": {}, "secondary": {}}
    for s in ("primary", "secondary"):
        for mid in groups[s]:
            for t in meta_first[mid][1]:
                k = flip_canon(t)
                if colors_of(t) and not (s == "secondary" and k in tiles["primary"]):
                    tiles[s].setdefault(k, colors_of(t))
    # Porytiles reserves tile 0 (transparent) in the primary.
    tile_count = {"primary": len(tiles["primary"]) + 1, "secondary": len(tiles["secondary"])}
    tile_cap = {"primary": NUM_TILES_IN_PRIMARY, "secondary": NUM_TILES_TOTAL - NUM_TILES_IN_PRIMARY}
    meta_cap = {"primary": NUM_METATILES_IN_PRIMARY, "secondary": NUM_METATILES_TOTAL - NUM_METATILES_IN_PRIMARY}
    pal_cap = {"primary": NUM_PALS_IN_PRIMARY, "secondary": NUM_PALS_TOTAL - NUM_PALS_IN_PRIMARY}

    # How often each tile is used on the map, to find one-off tiles.
    use = Counter()
    for (mx, my), mid in cell_meta.items():
        for t in meta_first[mid][1]:
            use[flip_canon(t)] += 1

    over_colors = [(meta_first[mid][0], i) for mid in range(len(meta_first))
                   for i, t in enumerate(meta_first[mid][1]) if len(colors_of(t)) > COLORS_PER_PAL]

    pal_est = {s: estimate_palettes(tiles[s].values()) for s in tiles}
    all_colors = {s: set().union(*tiles[s].values()) if tiles[s] else set() for s in tiles}

    os.makedirs(args.out, exist_ok=True)
    for s in ("primary", "secondary"):
        write_sheets([meta_first[mid][0] for mid in groups[s]], px, os.path.join(args.out, s),
                     [cell_behavior.get(meta_first[mid][0], "MB_NORMAL") for mid in groups[s]])

    with open(os.path.join(args.out, "map.bin"), "wb") as f:
        for my in range(mh):
            for mx in range(mw):
                mid = cell_meta[(mx, my)]
                gid = local_id[mid] + (0 if side[mid] == "primary" else NUM_METATILES_IN_PRIMARY)
                f.write(struct.pack("<H", gid | (DEFAULT_ELEVATION << 12)))

    # Overlay: red = tile used once on the map, yellow = cell sent to the secondary.
    ov = im.convert("RGB")
    d = ImageDraw.Draw(ov, "RGBA")
    for (mx, my), mid in cell_meta.items():
        if side[mid] == "secondary":
            d.rectangle([mx * 16, my * 16, mx * 16 + 15, my * 16 + 15], outline=(255, 220, 0, 255))
        for i, t in enumerate(meta_first[mid][1]):
            if colors_of(t) and use[flip_canon(t)] == 1:
                x0, y0 = mx * 16 + (i % 2) * 8, my * 16 + (i // 2) * 8
                d.rectangle([x0, y0, x0 + 7, y0 + 7], fill=(255, 0, 0, 110))
    ov.save(os.path.join(args.out, "overlay.png"))

    def mark(n, cap):
        return "OK" if n <= cap else f"**OVER by {n - cap}**"

    map_data = (mw + 15) * (mh + 14)
    once = {s: sum(1 for k in tiles[s] if use[k] == 1) for s in tiles}
    lines = [
        f"# Tileset budget: {os.path.basename(args.map)}",
        "",
        f"Map {W}x{H} px = {mw}x{mh} metatiles. Map data {map_data} of {MAX_MAP_DATA_SIZE}: {mark(map_data, MAX_MAP_DATA_SIZE)}.",
        f"Unique metatiles {len(meta_first)}. Tiles with more than {COLORS_PER_PAL} colors: {len(over_colors)}.",
        "",
        "| | Primary | Secondary |",
        "|---|---|---|",
        f"| Metatiles | {len(groups['primary'])} / {meta_cap['primary']} {mark(len(groups['primary']), meta_cap['primary'])} "
        f"| {len(groups['secondary'])} / {meta_cap['secondary']} {mark(len(groups['secondary']), meta_cap['secondary'])} |",
        f"| 8x8 tiles (flips merged) | {tile_count['primary']} / {tile_cap['primary']} {mark(tile_count['primary'], tile_cap['primary'])} "
        f"| {tile_count['secondary']} / {tile_cap['secondary']} {mark(tile_count['secondary'], tile_cap['secondary'])} |",
        f"| ...of which used only once on the map | {once['primary']} | {once['secondary']} |",
        f"| Colors (RGB555) | {len(all_colors['primary'])} | {len(all_colors['secondary'])} |",
        f"| Palettes, greedy estimate | {len(pal_est['primary'])} / {pal_cap['primary']} {mark(len(pal_est['primary']), pal_cap['primary'])} "
        f"| {len(pal_est['secondary'])} / {pal_cap['secondary']} {mark(len(pal_est['secondary']), pal_cap['secondary'])} |",
        "",
        "The palette row is a rough greedy estimate; Porytiles' search usually needs fewer.",
        "Secondary counts leave out tiles the primary already has, since the secondary can use them.",
        "",
        "overlay.png: red squares are 8x8 tiles that appear only once on the map (they cost a tile each);",
        "yellow outlines are cells sent to the secondary tileset.",
    ]
    if over_colors:
        lines += ["", "Tiles with too many colors (metatile cell, subtile 0-3):"]
        lines += [f"- {pos} subtile {i}" for pos, i in over_colors[:50]]
    with open(os.path.join(args.out, "report.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
