#!/usr/bin/env python3
"""Guess collision for a map.bin cut from a mockup.

A 16x16 cell is walkable when at least --walk-fraction of its pixels are grass
or path colors and it holds no building colors (outline, roof shadow, glass).
Everything else (trees, water, buildings, fences) is blocked. Rectangles can be
forced either way with --walk / --block X0,Y0,X1,Y1 (metatile coordinates,
inclusive). Writes collision into bits 10-11 and elevation 3 into bits 12-15,
and a collision.png preview next to OUT.bin.

Usage:
  collision.py MAP.png IN.bin OUT.bin [--walk 17,30,19,33] [--block 24,15,30,19]
"""

import argparse
import os
import struct

import numpy as np
from PIL import Image


def rgb(h):
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


WALK = [rgb(c) for c in ('6DC155', 'A7EA75', '49945B', 'B7B6B0', 'E7E6DB', '8898B0')]
BUILDING = [rgb(c) for c in ('301108', '585080', '74C1DC')]


def rect(s):
    return tuple(int(v) for v in s.split(','))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('inbin')
    ap.add_argument('outbin')
    ap.add_argument('--walk-fraction', type=float, default=0.8)
    ap.add_argument('--walk', type=rect, action='append', default=[])
    ap.add_argument('--block', type=rect, action='append', default=[])
    a = ap.parse_args()

    m = np.array(Image.open(a.map).convert('RGB')).astype(int)
    H, W, _ = m.shape
    mw, mh = W // 16, H // 16
    walk = np.zeros((H, W), bool)
    for c in WALK:
        walk |= (m == c).all(-1)
    d = open(a.inbin, 'rb').read()
    cells = list(struct.unpack('<%dH' % (len(d) // 2), d))

    inside = lambda x, y, r: r[0] <= x <= r[2] and r[1] <= y <= r[3]
    blocked = np.zeros((mh, mw), bool)
    for y in range(mh):
        for x in range(mw):
            cell = m[y * 16:y * 16 + 16, x * 16:x * 16 + 16]
            bld = any((cell == c).all(-1).any() for c in BUILDING)
            ok = walk[y * 16:y * 16 + 16, x * 16:x * 16 + 16].mean() >= a.walk_fraction and not bld
            if any(inside(x, y, r) for r in a.walk):
                ok = True
            if any(inside(x, y, r) for r in a.block):
                ok = False
            blocked[y, x] = not ok
            cells[y * mw + x] = (cells[y * mw + x] & 0x3FF) | (int(not ok) << 10) | (3 << 12)
    open(a.outbin, 'wb').write(struct.pack('<%dH' % len(cells), *cells))

    ov = np.array(Image.fromarray(m.astype(np.uint8)).convert('RGBA'))
    red = np.kron(blocked, np.ones((16, 16), bool))
    ov[red, :3] = (ov[red, :3] * 0.6 + np.array([255, 0, 0]) * 0.4).astype(np.uint8)
    Image.fromarray(ov).save(os.path.join(os.path.dirname(a.outbin) or '.', 'collision.png'))
    print('\n'.join(''.join('#' if b else '.' for b in row) for row in blocked))


if __name__ == '__main__':
    main()
