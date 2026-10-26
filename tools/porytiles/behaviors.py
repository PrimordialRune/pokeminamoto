#!/usr/bin/env python3
"""Metatile behaviors for a map rebuilt from a mockup: keep the ones set by hand
in Porymap and give thin fences edge collision.

Behaviors belong to metatiles, so a rebuild that recompiles the tilesets would
reset them. This reads the committed tilesets and layout before the rebuild and
carries each cell's behavior over to the new map when the cell looks the same
in the mockup (OLD_RENDER.png comes from render_check.py --out).

Fences on an 8px grid fill only part of a 16x16 cell. A cell whose only object
is a thin fence (--fence-color pixels plus their outline) gets an edge instead
of a solid block:
  vertical fence in the left/right 3 px       -> MB_IMPASSABLE_WEST / _EAST
The cell is then made walkable (listed in --open-out). Horizontal fences are
left alone: the bottom edge of a house looks the same to this test, and a
fence in the bottom half of a cell would need the top layer to stand behind.
Only cells that are new or changed get this, unless --all is given, so a cell
set back to blocked in Porymap stays that way.

Writes BEHAVIORS.csv (x,y,MB_NAME; cells left out are MB_NORMAL) for
map2porytiles.py --behaviors.

Usage:
  behaviors.py MAP.png OLD_RENDER.png PRIMARY_BIN SECONDARY_BIN LAYOUT.bin WIDTH BEHAVIORS.csv
               [--open-out OPEN.txt] [--all] [--preview edges.png]
"""

import argparse
import os
import re
import struct

import numpy as np
from PIL import Image, ImageDraw

NUM_METATILES_IN_PRIMARY = 512
HERE = os.path.dirname(os.path.abspath(__file__))
BEHAVIOR_H = os.path.join(HERE, '..', '..', 'include', 'constants', 'metatile_behaviors.h')


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


GROUND = [rgb(c) for c in ('6DC155', 'A7EA75', '49945B', '48A848', 'B7B6B0', 'E7E6DB', '8898B0', '2B7B46', '887850')]
# A fence is drawn in its grey plus these outline colors.
FENCE_LINE = [rgb(c) for c in ('301108', '585080')]


def behavior_names():
    body = open(BEHAVIOR_H).read().split('enum', 1)[1]
    return re.findall(r'^\s*(MB_\w+)', body, re.M)


def read_attrs(path):
    if not os.path.exists(path):
        return []
    raw = open(path, 'rb').read()
    return list(struct.unpack(f'<{len(raw) // 2}H', raw))


EDGE_PX = 3


def fence_edge(fence, other):
    """Edge behavior for one cell, or None when it should stay as it is."""
    if not fence.any() or other.any():
        return None
    ys, xs = np.nonzero(fence)
    y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
    # Only a fence on the cell's edge: one further in sits under the player's
    # sprite when he stands in the cell, so it stays a solid block.
    if x1 - x0 <= 5 and y1 - y0 >= 13:
        if x1 <= EDGE_PX - 1:
            return 'MB_IMPASSABLE_WEST'
        if x0 >= 16 - EDGE_PX:
            return 'MB_IMPASSABLE_EAST'
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('old_render')
    ap.add_argument('primary_bin')
    ap.add_argument('secondary_bin')
    ap.add_argument('layout')
    ap.add_argument('width', type=int)
    ap.add_argument('out')
    ap.add_argument('--open-out')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--fence-color', default='999890')
    ap.add_argument('--preview')
    a = ap.parse_args()

    names = behavior_names()
    m = np.array(Image.open(a.map).convert('RGB')).astype(int)
    H, W, _ = m.shape
    mw, mh = W // 16, H // 16
    old = None
    if os.path.exists(a.old_render):
        o = np.array(Image.open(a.old_render).convert('RGB')).astype(int)
        if o.shape == m.shape:
            old = o
    attrs = read_attrs(os.path.join(a.primary_bin, 'metatile_attributes.bin'))
    attrs += [0] * (NUM_METATILES_IN_PRIMARY - len(attrs))
    attrs += read_attrs(os.path.join(a.secondary_bin, 'metatile_attributes.bin'))
    cells = None
    if os.path.exists(a.layout):
        raw = open(a.layout, 'rb').read()
        if len(raw) == mw * mh * 2:
            cells = struct.unpack(f'<{mw * mh}H', raw)

    ground = np.zeros((H, W), bool)
    for c in GROUND:
        ground |= (m == c).all(-1)
    fence = (m == rgb(a.fence_color)).all(-1)
    # The fence: its grey and outline pixels connected to the grey, plus the
    # light ground-colored pixels inside the outline (the white of the posts).
    line = fence.copy()
    for c in FENCE_LINE:
        line |= (m == c).all(-1)
    fence_all = fence.copy()
    todo = list(zip(*np.nonzero(fence)))
    while todo:
        y, x = todo.pop()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                ny, nx = y + dy, x + dx
                if 0 <= ny < H and 0 <= nx < W and line[ny, nx] and not fence_all[ny, nx]:
                    fence_all[ny, nx] = True
                    todo.append((ny, nx))
    grow = fence_all.copy()
    grow[1:] |= fence_all[:-1]; grow[:-1] |= fence_all[1:]
    grow[:, 1:] |= fence_all[:, :-1]; grow[:, :-1] |= fence_all[:, 1:]
    fence_all |= grow & ~ground
    other = ~ground & ~fence_all

    # (Shift by >> 3 compares at GBA color depth, like render_check.py.)
    same = lambda x, y: old is not None and ((old[y * 16:y * 16 + 16, x * 16:x * 16 + 16] >> 3)
                                             == (m[y * 16:y * 16 + 16, x * 16:x * 16 + 16] >> 3)).all()
    rows, opened, kept, auto = [], [], 0, 0
    for y in range(mh):
        for x in range(mw):
            unchanged = cells is not None and same(x, y)
            if unchanged:
                mid = cells[y * mw + x] & 0x3FF
                b = attrs[mid] & 0xFF if mid < len(attrs) else 0
                if b:
                    rows.append((x, y, names[b]))
                    kept += 1
                if not a.all:
                    continue
            sl = np.s_[y * 16:y * 16 + 16, x * 16:x * 16 + 16]
            e = fence_edge(fence_all[sl], other[sl])
            if e:
                rows = [r for r in rows if r[:2] != (x, y)]
                rows.append((x, y, e))
                opened.append((x, y))
                auto += 1

    with open(a.out, 'w') as f:
        f.write('x,y,behavior\n')
        f.writelines(f'{x},{y},{b}\n' for x, y, b in rows)
    if a.open_out:
        with open(a.open_out, 'w') as f:
            f.writelines(f'{x},{y}\n' for x, y in opened)
    if a.preview:
        ov = Image.fromarray(m.astype(np.uint8))
        d = ImageDraw.Draw(ov)
        side = {'MB_IMPASSABLE_WEST': (0, 0, 0, 15), 'MB_IMPASSABLE_EAST': (15, 0, 15, 15),
                'MB_IMPASSABLE_NORTH': (0, 0, 15, 0), 'MB_IMPASSABLE_SOUTH': (0, 15, 15, 15)}
        for x, y, b in rows:
            if b in side:
                x0, y0, x1, y1 = side[b]
                d.line((x * 16 + x0, y * 16 + y0, x * 16 + x1, y * 16 + y1), fill=(255, 0, 255), width=2)
        ov.resize((W * 2, H * 2), Image.NEAREST).save(a.preview)
    print(f'behaviors: {kept} kept from the committed tilesets, {auto} fence edges')


if __name__ == '__main__':
    main()
