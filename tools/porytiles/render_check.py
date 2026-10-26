#!/usr/bin/env python3
"""Draw a map the way the GBA would, from compiled tileset files, and compare it to the mockup.

Reads tiles.png, palettes/*.pal and metatiles.bin from the primary and secondary
porytiles_bin folders plus a map.bin, renders the map (bottom then top layer)
and checks every pixel against the original PNG at GBA color depth.

Usage:
  render_check.py PRIMARY_BIN SECONDARY_BIN MAP.bin WIDTH MOCKUP.png [--out render.png]
"""

import argparse
import os
import struct
import sys

from PIL import Image

NUM_TILES_IN_PRIMARY = 512
NUM_METATILES_IN_PRIMARY = 512
NUM_PALS_IN_PRIMARY = 6
NUM_PALS_TOTAL = 14


def read_pal(path):
    lines = open(path).read().split()
    vals = list(map(int, lines[3:3 + 48]))
    return [tuple(vals[i * 3:i * 3 + 3]) for i in range(16)]


def load_tiles(path):
    im = Image.open(path)
    w, h = im.size
    data = im.tobytes()
    tiles = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            tiles.append([data[(ty * 8 + y) * w + tx * 8 + x] & 0xF for y in range(8) for x in range(8)])
    return tiles


def load_metatiles(path):
    raw = open(path, "rb").read()
    entries = struct.unpack(f"<{len(raw) // 2}H", raw)
    return [entries[i:i + 8] for i in range(0, len(entries), 8)]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("primary_bin")
    ap.add_argument("secondary_bin")
    ap.add_argument("map_bin")
    ap.add_argument("width", type=int)
    ap.add_argument("mockup")
    ap.add_argument("--out")
    ap.add_argument("--num-tiles-in-primary", type=int, default=NUM_TILES_IN_PRIMARY)
    args = ap.parse_args()

    pals = [read_pal(os.path.join(args.primary_bin, "palettes", f"{p:02d}.pal")) for p in range(NUM_PALS_IN_PRIMARY)]
    pals += [read_pal(os.path.join(args.secondary_bin, "palettes", f"{p:02d}.pal"))
             for p in range(NUM_PALS_IN_PRIMARY, NUM_PALS_TOTAL)]
    tiles = load_tiles(os.path.join(args.primary_bin, "tiles.png"))
    tiles += [[0] * 64] * (args.num_tiles_in_primary - len(tiles))
    tiles += load_tiles(os.path.join(args.secondary_bin, "tiles.png"))
    metas = load_metatiles(os.path.join(args.primary_bin, "metatiles.bin"))
    metas += [(0,) * 8] * (NUM_METATILES_IN_PRIMARY - len(metas))
    metas += load_metatiles(os.path.join(args.secondary_bin, "metatiles.bin"))

    raw = open(args.map_bin, "rb").read()
    cells = struct.unpack(f"<{len(raw) // 2}H", raw)
    w = args.width
    h = len(cells) // w
    out = Image.new("RGB", (w * 16, h * 16), (0, 0, 0))
    px = out.load()
    for i, cell in enumerate(cells):
        mx, my = i % w, i // w
        meta = metas[cell & 0x3FF]
        for layer in (0, 1):
            for q in range(4):
                e = meta[layer * 4 + q]
                tile = tiles[e & 0x3FF]
                hf, vf, pal = e >> 10 & 1, e >> 11 & 1, e >> 12
                for y in range(8):
                    for x in range(8):
                        sx, sy = (7 - x if hf else x), (7 - y if vf else y)
                        c = tile[sy * 8 + sx]
                        if c == 0 and layer == 1:
                            continue
                        px[mx * 16 + (q % 2) * 8 + x, my * 16 + (q // 2) * 8 + y] = pals[pal][c]

    ref = Image.open(args.mockup).convert("RGB")
    rpx = ref.load()
    bad = sum(1 for y in range(h * 16) for x in range(w * 16)
              if tuple(v >> 3 for v in px[x, y]) != tuple(v >> 3 for v in rpx[x, y]))
    if args.out:
        out.save(args.out)
    print(f"{w}x{h} metatiles rendered; {bad} pixels differ from the mockup")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
