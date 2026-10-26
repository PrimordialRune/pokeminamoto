#!/usr/bin/env python3
"""Shrink a map mockup's tile count by merging near-identical 8x8 tiles.

Hand-drawn ground (path edges, grass tufts, tree overlaps) makes almost every
8x8 tile unique, which blows the 512-tile budget. This merges tiles bottom-up:
at each step the one cluster of tiles that is cheapest to redraw as another
kept tile (or a flip of it) is merged, until --max-tiles are left. The cost of
a merge is the damage summed over every place on the map it changes, so the
budget is spent where nobody will notice.

Damage per pixel: a pixel that changes material (grass, tree canopy, gravel,
or any other single color: outlines, fences, flowers) costs --structure-weight;
a pixel that stays the same material costs its RGB distance / 255. Tree
outlines, path edges and objects therefore keep their shape, and the merges
mostly swap grass and gravel dither.

Tiles that appear in the building sheet (REF.png) are never changed and never
used as a merge target (they go to the secondary tileset).

Usage:
  fit_tiles.py MAP.png REF.png OUT.png [--max-tiles 545] [--structure-weight 12]
Then check the budget with map2porytiles.py OUT.png.
"""

import argparse
import heapq

import numpy as np
from PIL import Image


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


MATERIALS = [
    ('6DC155', 'A7EA75', '49945B', '48A848'),            # grass
    ('106342', '205030', '348C51', '2B7B46', '303830'),  # tree canopy
    ('E7E6DB', 'B7B6B0', '8898B0'),                      # gravel
]
MATERIAL = {rgb(c): i for i, group in enumerate(MATERIALS) for c in group}


def flips(t):
    return [t, t[:, ::-1], t[::-1], t[::-1, ::-1]]


def material_map(t, other):
    """Material id per pixel; colors outside MATERIALS each get their own id."""
    return np.array([[MATERIAL.get(c, len(MATERIALS) + other.setdefault(c, len(other)))
                      for c in map(tuple, row)] for row in t.tolist()])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('ref')
    ap.add_argument('out')
    ap.add_argument('--max-tiles', type=int, default=545,
                    help='tiles left outside the building sheet, flips counted once')
    ap.add_argument('--structure-weight', type=float, default=12)
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

    # Unique tiles, a tile and its flips counted once (stored in canonical flip).
    index, tiles, count, pos = {}, [], [], []
    for y in range(0, H, 8):
        for x in range(0, W, 8):
            fl = flips(m[y:y + 8, x:x + 8])
            keys = [f.tobytes() for f in fl]
            k = min(keys)
            fi = keys.index(k)
            if k not in index:
                index[k] = len(tiles)
                tiles.append(fl[fi].copy())
                count.append(0)
            count[index[k]] += 1
            pos.append((y, x, index[k], fi))
    N = len(tiles)
    T = np.stack(tiles)
    count = np.array(count, float)
    fixed = np.array([any(f.tobytes() in frozen for f in flips(t)) for t in tiles])
    other = {}
    M = np.stack([material_map(t, other) for t in tiles])

    # D[a, b]: damage of drawing tile a as the best flip of tile b (flip in F).
    D = np.full((N, N), np.inf, np.float32)
    F = np.zeros((N, N), np.int8)
    Tf = [np.stack([flips(t)[f] for t in T]) for f in range(4)]
    Mf = [np.stack([flips(t)[f] for t in M]) for f in range(4)]
    for i in range(N):
        for f in range(4):
            color = np.sqrt(((Tf[f] - T[i][None]) ** 2).sum(-1)) / 255.0
            d = np.where(Mf[f] != M[i][None], a.structure_weight, color).sum((1, 2)).astype(np.float32)
            better = d < D[i]
            D[i][better] = d[better]
            F[i][better] = f
    D[:, fixed] = np.inf
    D[fixed, :] = np.inf
    np.fill_diagonal(D, np.inf)

    alive = np.ones(N, bool)
    members = {i: [i] for i in range(N)}
    rep = np.arange(N)

    def best_target(r):
        ms = members[r]
        cost = (count[ms][:, None] * D[ms]).sum(0)
        cost[~alive] = np.inf
        cost[r] = np.inf
        j = int(np.argmin(cost))
        return float(cost[j]), j

    heap = [(*best_target(r), r) for r in range(N) if not fixed[r]]
    heapq.heapify(heap)
    while alive.sum() - fixed.sum() > a.max_tiles and heap:
        cost, t, r = heapq.heappop(heap)
        if not alive[r]:
            continue
        fresh = best_target(r)
        if fresh != (cost, t):        # stale: the target merged away or grew
            heapq.heappush(heap, (*fresh, r))
            continue
        if not np.isfinite(cost):
            break
        members[t] += members.pop(r)
        alive[r] = False
        rep[members[t]] = t
        heapq.heappush(heap, (*best_target(t), t))

    o = m.copy()
    for y, x, i, fi in pos:
        r = rep[i]
        if r != i:
            # The map tile is flips(T[i])[fi]; T[i] is drawn as flips(T[r])[F[i, r]].
            o[y:y + 8, x:x + 8] = flips(flips(T[r])[F[i, r]])[fi]
    Image.fromarray(o.astype(np.uint8)).save(a.out)
    changed = int((o != m).any(-1).sum())
    print(f'{N} unique tiles -> {int(alive.sum())} ({int(fixed.sum())} from the building sheet), '
          f'{changed} pixels changed')


if __name__ == '__main__':
    main()
