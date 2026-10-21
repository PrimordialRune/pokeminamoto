#!/usr/bin/env python3
"""Shrink a map mockup's tile count by merging near-identical 8x8 tiles.

Hand-drawn ground (path edges, grass tufts, tree overlaps) makes almost every
8x8 tile unique, which blows the 512-tile budget. This replaces each tile with
an already-kept tile (or a flip of one) when the two are close enough, most
used tiles first, so the map keeps its look while reusing tiles.

  - tiles that appear in the building sheet (--secondary-ref) are never changed;
  - pure gravel tiles (only the three path colors) use --gravel-threshold;
  - ground tiles (grass, gravel, trees) use --ground-threshold;
  - everything else (fences, signs, flowers) uses --object-threshold.

Distance = sum over the 64 pixels of RGB distance / 255, so a threshold of 9
allows about nine fully wrong pixels or many slightly-off ones.

Usage:
  fit_tiles.py MAP.png REF.png OUT.png [--ground-threshold 9] [--object-threshold 2]
               [--gravel-threshold 2]
Then check the budget with map2porytiles.py OUT.png.
"""

import argparse

import numpy as np
from PIL import Image


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


GRAVEL = {rgb(c) for c in ('8898B0', 'B7B6B0', 'E7E6DB')}
GROUND = GRAVEL | {rgb(c) for c in ('6DC155', 'A7EA75', '49945B', '106342', '205030',
                                    '303830', '348C51', '2B7B46')}


def flips(t):
    return [t, t[:, ::-1], t[::-1], t[::-1, ::-1]]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('ref')
    ap.add_argument('out')
    ap.add_argument('--ground-threshold', type=float, default=9)
    ap.add_argument('--object-threshold', type=float, default=2)
    ap.add_argument('--gravel-threshold', type=float, default=2)
    a = ap.parse_args()

    m = np.array(Image.open(a.map).convert('RGB')).astype(np.int32)
    s = np.array(Image.open(a.ref).convert('RGBA'))
    H, W, _ = m.shape

    frozen = set()
    for y in range(0, s.shape[0], 8):
        for x in range(0, s.shape[1], 8):
            t = s[y:y + 8, x:x + 8]
            if (t[:, :, 3] > 0).any():
                frozen.add(t[:, :, :3].astype(np.int32).tobytes())

    tiles, pos = {}, []
    for y in range(0, H, 8):
        for x in range(0, W, 8):
            t = m[y:y + 8, x:x + 8]
            k = t.tobytes()
            tiles.setdefault(k, [t, 0])[1] += 1
            pos.append((y, x, k))

    reps, repk, mapping = [], [], {}
    for k in sorted(tiles, key=lambda k: -tiles[k][1]):
        t = tiles[k][0]
        if k in frozen or not reps:
            reps.append(t); repk.append(k); mapping[k] = (k, 0)
            continue
        R = np.stack(reps)
        best = (1e9, None, None)
        for fi, f in enumerate(flips(t)):
            d = np.sqrt(((R - f[None]) ** 2).sum(-1)).sum((1, 2)) / 255.0
            i = int(d.argmin())
            if d[i] < best[0]:
                best = (d[i], i, fi)
        cols = set(map(tuple, t.reshape(-1, 3).tolist()))
        lim = (a.gravel_threshold if cols <= GRAVEL else
               a.ground_threshold if cols <= GROUND else a.object_threshold)
        if best[0] <= lim and repk[best[1]] not in frozen:
            mapping[k] = (repk[best[1]], best[2])
        else:
            reps.append(t); repk.append(k); mapping[k] = (k, 0)

    o = m.copy()
    for y, x, k in pos:
        rk, fi = mapping[k]
        if rk != k:
            o[y:y + 8, x:x + 8] = flips(tiles[rk][0])[fi]
    Image.fromarray(o.astype(np.uint8)).save(a.out)
    print(f'{len(tiles)} unique tiles -> {len(reps)}')


if __name__ == '__main__':
    main()
