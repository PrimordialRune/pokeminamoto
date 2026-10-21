#!/usr/bin/env python3
"""Import Omar's player sprites (MyRes/Trainers/Player) over Brendan's slots.

usage: import_player.py <MyRes/Trainers/Player dir>

  red_normal.png   -> overworld walking + running (16x32 frames, Emerald order)
  000_trainer.png  -> trainer front pic (padded to 64x64)
  000_battle.png   -> 5-frame battle back pic (320x64 strip -> 64x320 column)

The mockup backgrounds are flat (132,123,165); they become index 0.
"""
import sys
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
BG = (132, 123, 165)
# Stray anti-alias pixels next to the background colour, folded into it
NEAR_BG = {(132, 126, 135)}
# The front pic has 16 colours plus background; its darkest trouser shade joins the shirt's
FRONT_MERGE = {(70, 64, 40): (48, 48, 48)}


def write_jasc(path, colors):
    colors = list(colors) + [(0, 0, 0)] * (16 - len(colors))
    with open(path, 'w') as f:
        f.write('JASC-PAL\n0100\n16\n')
        for c in colors:
            f.write('%d %d %d\n' % c)


def indexed(rgb, colors):
    """rgb image -> P image whose palette is `colors` (index 0 = background)"""
    lut = {c: i for i, c in enumerate(colors)}
    out = Image.new('P', rgb.size)
    out.putdata([lut[p] for p in rgb.getdata()])
    flat = [v for c in colors for v in c] + [0] * (48 - 3 * len(colors))
    out.putpalette(flat)
    return out


def flatten(path, merge={}):
    """RGBA mockup -> RGB with the background (and near-background strays) as BG"""
    im = Image.open(path).convert('RGBA')
    px = [BG if a == 0 or (r, g, b) in NEAR_BG else merge.get((r, g, b), (r, g, b))
          for r, g, b, a in im.getdata()]
    rgb = Image.new('RGB', im.size)
    rgb.putdata(px)
    return rgb


def palette_of(rgb):
    colors = [BG] + sorted({c for c in rgb.getdata() if c != BG}, key=lambda c: sum(c))
    assert len(colors) <= 16, '%d colours' % len(colors)
    return colors


def main(src):
    src = Path(src)

    # Overworld: already indexed, 9 frames in the first 144 px
    walk = Image.open(src / 'red_normal.png')
    assert walk.mode == 'P'
    walk = walk.crop((0, 0, 144, 32))
    pal = walk.getpalette()[:48]
    colors = [tuple(pal[i:i + 3]) for i in range(0, 48, 3)]
    colors[0] = (115, 197, 164)  # transparent; keep Emerald's key colour
    walk.putpalette([v for c in colors for v in c])
    ow = ROOT / 'graphics/object_events/pics/people/brendan'
    walk.save(ow / 'walking.png')
    walk.save(ow / 'running.png')  # no running frames yet: run with the walk cycle
    write_jasc(ROOT / 'graphics/object_events/palettes/brendan.pal', colors)
    # Water reflection: lighter and bluer, fitted to Emerald's Brendan pair
    refl = [colors[0]] + [(min(255, int(r * 0.6 + 100)), min(255, int(g * 0.6 + 104)), min(255, int(b * 0.6 + 108)))
                          for r, g, b in colors[1:]]
    write_jasc(ROOT / 'graphics/object_events/palettes/brendan_reflection.pal', refl)

    # Front pic: 56x64 centred in 64x64
    front = flatten(src / '000_trainer.png', FRONT_MERGE)
    canvas = Image.new('RGB', (64, 64), BG)
    canvas.paste(front, ((64 - front.width) // 2, 64 - front.height))
    colors = palette_of(canvas)
    indexed(canvas, colors).save(ROOT / 'graphics/trainers/front_pics/brendan.png')
    write_jasc(ROOT / 'graphics/trainers/palettes/brendan.pal', colors)

    # Back pic: 5 frames side by side -> stacked; palette lives in the PNG
    strip = flatten(src / '000_battle.png')
    n = strip.width // 64
    col = Image.new('RGB', (64, 64 * n), BG)
    for i in range(n):
        col.paste(strip.crop((i * 64, 0, i * 64 + 64, 64)), (0, i * 64))
    indexed(col, palette_of(col)).save(ROOT / 'graphics/trainers/back_pics/brendan.png')


if __name__ == '__main__':
    main(sys.argv[1])
