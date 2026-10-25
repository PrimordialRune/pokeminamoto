#!/usr/bin/env python3
"""Sway animation for plants (crops, flower bushes) in a map mockup.

Each --rect is a block of plants. In every frame the upper rows of each plant
lean one pixel left or right; the lower rows stay rooted. Pixels only move, so
no color is added (style guide rule 8). Tiles inside a rect that hold no leaf
pixel stay still, so plain grass elsewhere on the map doesn't start moving.

  --rect NAME:X0,Y0,X1,Y1[:PERIOD]   pixel box (inclusive-exclusive); PERIOD is
                                    the plant height in pixels (rows repeat
                                    every PERIOD, the top half leans). Several
                                    rects can share a NAME; each NAME becomes
                                    one animation folder.
  --leans NAME:0,1,0,-1             lean of each frame for that NAME
                                    (default 0,1,0,-1). Frame 0 must be 0.

Identical tiles share one VRAM slot, so a tile that appears twice inside the
rects sways the same way in both places (the first one seen wins), and a tile
also used outside the rects stays still.

Writes OUT_MAP.png (the mockup, unchanged) and
ANIM_DIR/NAME/{key,f0,f1,...}.png, then prints the tile count per animation.

Usage:
  sway_anim.py MAP.png OUT_MAP.png ANIM_DIR --rect crops:336,32,416,96:16 \
      --rect flowers:188,330,208,346:14 --leans flowers:0,1,0,-1
"""

import argparse
import os
from collections import defaultdict

import numpy as np
from PIL import Image

FLIPS = [lambda t: t, lambda t: t[:, ::-1], lambda t: t[::-1], lambda t: t[::-1, ::-1]]


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


# A tile sways only if it holds leaves (the darker foliage steps); plain grass,
# path and soil tiles stay put.
PLANT = [rgb(c) for c in ('303830', '205030', '106342', '348C51', '2B7B46')]


def parse_rect(s):
    name, box, *rest = s.split(':')
    x0, y0, x1, y1 = (int(v) for v in box.split(','))
    return name, (x0, y0, x1, y1), int(rest[0]) if rest else 16


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('out_map')
    ap.add_argument('anim_dir')
    ap.add_argument('--rect', action='append', default=[], type=parse_rect)
    ap.add_argument('--leans', action='append', default=[])
    a = ap.parse_args()

    m = np.array(Image.open(a.map).convert('RGB'))
    H, W, _ = m.shape
    plant = np.zeros((H, W), bool)
    for c in PLANT:
        plant |= (m == c).all(-1)

    leans = defaultdict(lambda: [0, 1, 0, -1])
    for s in a.leans:
        name, vals = s.split(':')
        leans[name] = [int(v) for v in vals.split(',')]
    groups = defaultdict(list)
    for name, box, period in a.rect:
        groups[name].append((box, period))

    out = m.copy()
    for name, rects in groups.items():
        L = leans[name]
        assert L[0] == 0, 'frame 0 must not lean (it is the key frame)'
        F = np.stack([m] * len(L))
        moved = np.zeros((H, W), bool)
        for (x0, y0, x1, y1), period in rects:
            for k, lean in enumerate(L):
                if not lean:
                    continue
                for y in range(y0, y1):
                    if (y - y0) % period >= period // 2:
                        continue
                    F[k, y, x0:x1] = m[y, x0 - lean:x1 - lean]
            moved[y0:y1, x0:x1] = True
        # Tiles also used outside the rects (fences, tree edges) stay still: they
        # share a VRAM slot with their copies, which would sway too.
        key = lambda t: min(FLIPS[i](t).tobytes() for i in range(4))
        outside = set()
        for ty in range(0, H, 8):
            for tx in range(0, W, 8):
                if not moved[ty:ty + 8, tx:tx + 8].any():
                    outside.add(key(m[ty:ty + 8, tx:tx + 8]))
        # Keep tiles without plant pixels still, and everything outside the rects.
        anim_tiles = []
        seqs = {}
        for ty in range(0, H, 8):
            for tx in range(0, W, 8):
                sl = (slice(ty, ty + 8), slice(tx, tx + 8))
                if not (moved[sl].any() and plant[sl].any()) or key(m[sl]) in outside:
                    F[:, ty:ty + 8, tx:tx + 8] = m[sl]
                    continue
                seq = F[:, ty:ty + 8, tx:tx + 8]
                if (seq == seq[0]).all():
                    continue
                kb, i = min((FLIPS[i](seq[0]).tobytes(), i) for i in range(4))
                cs = np.stack([FLIPS[i](s) for s in seq]).tobytes()
                if kb in seqs:
                    # Same tile as one already seen: it must sway the same way,
                    # since both use one tile in VRAM.
                    canon = np.frombuffer(seqs[kb], np.uint8).reshape(seq.shape)
                    F[:, ty:ty + 8, tx:tx + 8] = np.stack([FLIPS[i](c) for c in canon])
                    continue
                seqs[kb] = cs
                anim_tiles.append(seq.copy())

        # Animated tiles must not also be used, unmoving, elsewhere on the map.
        static = set()
        for ty in range(0, H, 8):
            for tx in range(0, W, 8):
                seq = F[:, ty:ty + 8, tx:tx + 8]
                if (seq == seq[0]).all():
                    static.add(min(FLIPS[i](seq[0]).tobytes() for i in range(4)))
        clash = [t for t in anim_tiles if min(FLIPS[i](t[0]).tobytes() for i in range(4)) in static]
        if clash:
            print(f'{name}: warning: {len(clash)} animated tiles are also used outside the rects and will sway there too')

        d = os.path.join(a.anim_dir, name)
        os.makedirs(d, exist_ok=True)
        n = len(anim_tiles)
        for k in range(len(L) + 1):
            img = np.zeros((8, 8 * n, 3), np.uint8)
            for i, seq in enumerate(anim_tiles):
                img[:, i * 8:i * 8 + 8] = seq[0 if k == len(L) else k]
            Image.fromarray(img).save(os.path.join(d, 'key.png' if k == len(L) else f'f{k}.png'))
        print(f'{name}: {n} animated tiles, {len(L)} frames')
    Image.fromarray(out).save(a.out_map)


if __name__ == '__main__':
    main()
