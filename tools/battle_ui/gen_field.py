#!/usr/bin/env python3
"""Builds the GSC-white battle field (BG3) from the Background layer of Omar's Battle UI v0.1 mockup.

Input:  tools/battle_ui/src/field_v0.1.png (240x160, exported from MyRes/UI/UItest2.ase)
Output: graphics/battle_environment/field/tiles.png, map.bin and one palette per environment
        that uses it (the third palette is kept from that environment, since its intro grass uses it).
Usage: gen_field.py <repo root>
"""
import os, struct, sys
from PIL import Image

ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'
SRC = os.path.join(ROOT, 'tools/battle_ui/src/field_v0.1.png')
OUT = os.path.join(ROOT, 'graphics/battle_environment/field')
ENVS = {'grass': 'tall_grass', 'long_grass': 'long_grass', 'plain': 'plain'}
PALSLOT = 2  # the environment's first BG palette

def read_pal(path):
    lines = open(path).read().split('\n')
    return [tuple(int(v) for v in l.split()) for l in lines[3:] if l.strip()]

def write_pal(path, colors):
    with open(path, 'w', newline='\r\n') as f:
        f.write('JASC-PAL\n0100\n%d\n' % len(colors))
        for c in colors:
            f.write('%d %d %d\n' % c)

def main():
    img = Image.open(SRC).convert('RGB')
    # index 0 is transparent on BG3, so every color the art uses starts at 1
    colors = []
    for _, c in sorted(img.getcolors(), key=lambda nc: -sum(nc[1])):
        colors.append(c)
    if len(colors) > 15:
        raise SystemExit('field art has %d colors, max 15' % len(colors))
    index = {c: i + 1 for i, c in enumerate(colors)}
    W, H = 30, 20

    def tile(tx, ty):
        return tuple(tuple(index[img.getpixel((tx * 8 + x, ty * 8 + y))] for x in range(8)) for y in range(8))

    white = tuple(tuple([index[(255, 255, 255)] if (255, 255, 255) in index else 1] * 8) for _ in range(8))
    tiles = [white]
    lookup = {white: 0}

    def idx(t):
        for flip, f in ((0, lambda a: a), (1, lambda a: tuple(tuple(reversed(r)) for r in a))):
            k = f(t)
            if k in lookup:
                return lookup[k] | (flip << 10)
        lookup[t] = len(tiles)
        tiles.append(t)
        return lookup[t]

    pal_bits = PALSLOT << 12
    # 64x32 map: left screen is the field with the foe's platform, right screen
    # (seen while the intro slides) repeats the platform-free left half
    tilemap = [pal_bits] * (64 * 32)
    for ty in range(32):
        for tx in range(64):
            if ty >= H:
                continue
            if tx < W:
                sx = tx
            elif tx < 32:
                sx = tx - 16
            else:
                sx = (tx - 32) % 16
            v = idx(tile(sx, ty)) | pal_bits
            # screen size 1 is two 32x32 blocks side by side
            block, col = divmod(tx, 32)
            tilemap[block * 1024 + ty * 32 + col] = v
    if len(tiles) > 512:
        raise SystemExit('too many tiles: %d' % len(tiles))

    os.makedirs(OUT, exist_ok=True)
    out = Image.new('P', (128, ((len(tiles) + 15) // 16) * 8), 0)
    flat = [0, 0, 0]
    for c in colors:
        flat += list(c)
    out.putpalette(flat + [0] * (768 - len(flat)))
    for i, t in enumerate(tiles):
        for y in range(8):
            for x in range(8):
                out.putpixel(((i % 16) * 8 + x, (i // 16) * 8 + y), t[y][x])
    out.save(os.path.join(OUT, 'tiles.png'))
    with open(os.path.join(OUT, 'map.bin'), 'wb') as f:
        f.write(struct.pack('<%dH' % len(tilemap), *tilemap))

    field_pal = [(0, 0, 0)] + colors + [(0, 0, 0)] * (15 - len(colors))
    for name, env in ENVS.items():
        orig = read_pal(os.path.join(ROOT, 'graphics/battle_environment', env, 'palette.pal'))
        write_pal(os.path.join(OUT, name + '.pal'), field_pal + field_pal + orig[32:48])
    print('field: %d colors, %d tiles' % (len(colors), len(tiles)))

if __name__ == '__main__':
    main()
