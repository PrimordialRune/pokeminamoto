#!/usr/bin/env python3
"""Flower-bush animation for a map mockup: the bush keeps its outline while its
blossoms bob and turn and the leaf highlights ripple, each flower on its own
beat. Pixels only move or swap with colors already in the bush (rule 8).

  --rect X0,Y0,X1,Y1   pixel box around one bush (inclusive-exclusive)

Writes OUT_MAP.png (unchanged mockup) and ANIM_DIR/{key,f0..f3}.png, and
prints the tile count. Tiles also used outside the rects stay still, and two
copies of one tile animate the same way, since they share one VRAM slot.

Usage:
  flower_anim.py MAP.png OUT_MAP.png ANIM_DIR --rect 335,315,355,328 ... [--preview p.gif]
"""

import argparse
import os

import numpy as np
from PIL import Image

FLIPS = [lambda t: t, lambda t: t[:, ::-1], lambda t: t[::-1], lambda t: t[::-1, ::-1]]
FRAMES = 4


def rgb(h):
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)])


PETAL = [rgb(c) for c in ('A63A1A', 'FF825E')]
HEART = [rgb('E7E6DB')]
LEAF_DARK = [rgb(c) for c in ('303830', '205030', '106342')]
LEAF_LIGHT = [rgb(c) for c in ('348C51', '2B7B46')]


def mask(a, cols):
    out = np.zeros(a.shape[:2], bool)
    for c in cols:
        out |= (a == c).all(-1)
    return out


def components(m):
    seen = np.zeros_like(m)
    comps = []
    for y, x in zip(*np.nonzero(m)):
        if seen[y, x]:
            continue
        todo, comp = [(y, x)], []
        seen[y, x] = True
        while todo:
            cy, cx = todo.pop()
            comp.append((cy, cx))
            for dy in (-1, 0, 1):
                for dx in (-1, 0, 1):
                    ny, nx = cy + dy, cx + dx
                    if 0 <= ny < m.shape[0] and 0 <= nx < m.shape[1] and m[ny, nx] and not seen[ny, nx]:
                        seen[ny, nx] = True
                        todo.append((ny, nx))
        comps.append(comp)
    return comps


def animate_bush(b, seed):
    """b: RGB crop of one bush. Returns FRAMES crops."""
    flower = mask(b, PETAL + HEART)
    leaf = mask(b, LEAF_DARK + LEAF_LIGHT)
    light = mask(b, LEAF_LIGHT)
    canopy = leaf | flower
    # Interior: canopy pixels whose 4 neighbours are canopy too (the outline stays put).
    inner = canopy.copy()
    inner[1:] &= canopy[:-1]; inner[:-1] &= canopy[1:]
    inner[:, 1:] &= canopy[:, :-1]; inner[:, :-1] &= canopy[:, 1:]
    inner[0] = inner[-1] = False; inner[:, 0] = inner[:, -1] = False
    fill = LEAF_DARK[1]
    flowers = components(flower)
    frames = []
    for k in range(FRAMES):
        f = b.copy()
        # Leaf highlights ripple: one pixel right, back, one left, back.
        dx = (0, 1, 0, -1)[k]
        if dx:
            for y, x in zip(*np.nonzero(light & inner)):
                if inner[y, x + dx] and not flower[y, x + dx]:
                    f[y, x] = fill
            for y, x in zip(*np.nonzero(light & inner)):
                if inner[y, x + dx] and not flower[y, x + dx]:
                    f[y, x + dx] = b[y, x]
        # Blossoms: each bobs up a pixel on its own beat, and turns (petals
        # mirror across the heart) half a beat later.
        for i, comp in enumerate(flowers):
            # Two groups half a loop apart; frame 0 (the key frame) stays untouched.
            beat = (k + 2 * ((i + seed) % 2)) % FRAMES
            ys = [p[0] for p in comp]; xs = [p[1] for p in comp]
            if beat == 1 and all(inner[y - 1, x] or flower[y - 1, x] for y, x in comp):
                for y, x in comp:
                    f[y, x] = fill
                for y, x in comp:
                    f[y - 1, x] = b[y, x]
            elif beat == 3:
                cx = (min(xs) + max(xs)) / 2
                for y, x in comp:
                    mx = int(round(2 * cx - x))
                    if flower[y, mx] or inner[y, mx]:
                        f[y, mx] = b[y, x]
        # The crown leans with the wind: the top rows of the bush shift a pixel
        # (frames 1 and 3, opposite ways), the lower body stays rooted.
        lean = (0, 1, 0, -1)[k]
        if lean:
            rows = np.nonzero(canopy.any(1))[0]
            top = rows[0] if len(rows) else 0
            for y in range(top, min(top + 4, f.shape[0])):
                src = f[y].copy()
                shifted = np.roll(src, lean, 0)
                # Only canopy pixels move; the grass around stays where it is.
                row_canopy = canopy[y] | np.roll(canopy[y], lean)
                f[y][row_canopy] = np.where(np.roll(canopy[y], lean)[row_canopy, None], shifted[row_canopy], b[y][row_canopy])
        frames.append(f)
    return frames


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('out_map')
    ap.add_argument('anim_dir')
    ap.add_argument('--rect', action='append', default=[])
    ap.add_argument('--preview')
    a = ap.parse_args()

    m = np.array(Image.open(a.map).convert('RGB'))
    H, W, _ = m.shape
    F = np.stack([m] * FRAMES)
    moved = np.zeros((H, W), bool)
    boxes = []
    for i, r in enumerate(a.rect):
        x0, y0, x1, y1 = (int(v) for v in r.split(','))
        boxes.append((x0, y0, x1, y1))
        for k, fr in enumerate(animate_bush(m[y0:y1, x0:x1], i)):
            F[k, y0:y1, x0:x1] = fr
        moved[y0:y1, x0:x1] = True

    key = lambda t: min(FLIPS[i](t).tobytes() for i in range(4))
    outside = {key(m[ty:ty + 8, tx:tx + 8]) for ty in range(0, H, 8) for tx in range(0, W, 8)
               if not moved[ty:ty + 8, tx:tx + 8].any()}
    seqs, tiles = {}, []
    for ty in range(0, H, 8):
        for tx in range(0, W, 8):
            seq = F[:, ty:ty + 8, tx:tx + 8]
            if not moved[ty:ty + 8, tx:tx + 8].any() or (seq == seq[0]).all():
                continue
            if key(m[ty:ty + 8, tx:tx + 8]) in outside:
                F[:, ty:ty + 8, tx:tx + 8] = m[ty:ty + 8, tx:tx + 8]
                continue
            kb, i = min((FLIPS[i](seq[0]).tobytes(), i) for i in range(4))
            cs = np.stack([FLIPS[i](s) for s in seq]).tobytes()
            if kb in seqs:
                canon = np.frombuffer(seqs[kb], np.uint8).reshape(seq.shape)
                F[:, ty:ty + 8, tx:tx + 8] = np.stack([FLIPS[i](c) for c in canon])
                continue
            seqs[kb] = cs
            tiles.append(seq.copy())

    os.makedirs(a.anim_dir, exist_ok=True)
    n = len(tiles)
    for k in range(FRAMES + 1):
        img = np.zeros((8, 8 * n, 3), np.uint8)
        for i, seq in enumerate(tiles):
            img[:, i * 8:i * 8 + 8] = seq[0 if k == FRAMES else k]
        Image.fromarray(img).save(os.path.join(a.anim_dir, 'key.png' if k == FRAMES else f'f{k}.png'))
    Image.fromarray(m).save(a.out_map)
    if a.preview:
        x0, y0, x1, y1 = boxes[0]
        crops = [Image.fromarray(f[y0 - 4:y1 + 4, x0 - 4:x1 + 4]).resize(((x1 - x0 + 8) * 8, (y1 - y0 + 8) * 8), Image.NEAREST)
                 for f in F]
        crops[0].save(a.preview, save_all=True, append_images=crops[1:], duration=267, loop=0)
    print(f'flowers: {n} animated tiles, {FRAMES} frames')


if __name__ == '__main__':
    main()
