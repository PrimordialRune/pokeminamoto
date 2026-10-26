#!/usr/bin/env python3
"""Chimney smoke for a map mockup: puffs that rise from a chimney, swell and
thin out, drawn with colors the map already has (style guide rule 8).

Every frame shows smoke, so the animated tiles stay unique and don't clash
with the plain copies of the same ground or fence tiles elsewhere.

  --mouth X,Y        top centre of the chimney opening (pixels)
  --box X0,Y0,X1,Y1  tile-aligned area the smoke may cover (inclusive-exclusive);
                     keep it small, each 8x8 tile in it costs a tile
  --frames N         loop length (default 8; must divide 16)

Writes OUT_MAP.png (frame 0 drawn in) and ANIM_DIR/{key,f0..}.png.

Usage:
  smoke_anim.py MAP.png OUT_MAP.png ANIM_DIR --mouth 121,41 --box 112,16,128,40
"""

import argparse
import os

import numpy as np
from PIL import Image

LIGHT = np.array([0xE7, 0xE6, 0xDB])
MID = np.array([0xB7, 0xB6, 0xB0])
PUFFS = 3


def disc(img, cx, cy, r, color, box):
    x0, y0, x1, y1 = box
    for y in range(int(cy - r), int(cy + r) + 1):
        for x in range(int(cx - r), int(cx + r) + 1):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r + 0.5 and x0 <= x < x1 and y0 <= y < y1:
                img[y, x] = color


def frame(m, k, n, mouth, box):
    f = m.copy()
    mx, my = mouth
    height = my - box[1] - 3
    # Oldest puff first, so younger ones draw on top.
    for p in sorted(range(PUFFS), key=lambda p: -((k + p * n // PUFFS) % n)):
        age = ((k + p * n // PUFFS) % n) / n          # 0 at the mouth, 1 at the top
        cy = my - 2 - age * height
        cx = mx + 2.5 * age ** 1.5 + 0.8 * np.sin(age * 5 + p)
        r = 1 + 2.6 * np.sin(np.pi * min(age * 1.3, 1)) if age < 0.77 else 2.6 * (1 - age) / 0.23 + 0.6
        disc(f, cx, cy, r, MID, box)
        if age < 0.6:
            disc(f, cx - 0.5, cy - 0.5, max(r - 1, 0.5), LIGHT, box)
    return f


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('out_map')
    ap.add_argument('anim_dir')
    ap.add_argument('--mouth', required=True)
    ap.add_argument('--box', required=True)
    ap.add_argument('--frames', type=int, default=8)
    ap.add_argument('--preview')
    a = ap.parse_args()

    mouth = tuple(int(v) for v in a.mouth.split(','))
    box = tuple(int(v) for v in a.box.split(','))
    assert all(v % 8 == 0 for v in box), '--box must be tile-aligned'
    m = np.array(Image.open(a.map).convert('RGB'))
    F = [frame(m, k, a.frames, mouth, box) for k in range(a.frames)]

    tiles = []
    for ty in range(box[1], box[3], 8):
        for tx in range(box[0], box[2], 8):
            seq = [f[ty:ty + 8, tx:tx + 8] for f in F]
            if any((s != seq[0]).any() for s in seq):
                tiles.append(seq)
            else:
                for f in F[1:]:
                    f[ty:ty + 8, tx:tx + 8] = seq[0]
    os.makedirs(a.anim_dir, exist_ok=True)
    n = len(tiles)
    for k in range(a.frames + 1):
        img = np.zeros((8, 8 * n, 3), np.uint8)
        for i, seq in enumerate(tiles):
            img[:, i * 8:i * 8 + 8] = seq[0 if k == a.frames else k]
        Image.fromarray(img).save(os.path.join(a.anim_dir, 'key.png' if k == a.frames else f'f{k}.png'))
    Image.fromarray(F[0]).save(a.out_map)
    if a.preview:
        x0, y0, x1, y1 = box
        crop = [Image.fromarray(f[y0 - 16:y1 + 24, x0 - 24:x1 + 24]) for f in F]
        crop = [c.resize((c.width * 4, c.height * 4), Image.NEAREST) for c in crop]
        crop[0].save(a.preview, save_all=True, append_images=crop[1:], duration=267, loop=0)
    print(f'smoke: {n} animated tiles, {a.frames} frames')


if __name__ == '__main__':
    main()
