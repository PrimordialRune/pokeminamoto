#!/usr/bin/env python3
"""Push the devamped intro colours toward Game Boy Color palettes.

    python3 tools/intro_devamp/devamp.py
    python3 tools/intro_devamp/gbc.py [--strength 1.4]

Always run the two in this order: gbc.py works on devamp.py's output.

Keeps every colour devamp made (all the detail), and moves each one toward how
Gold/Silver/Crystal coloured the same thing:
- hue pulled toward its GSC family (lime grass, teal-green Venusaur, Gengar violet,
  Nidorino magenta, red-orange, GSC navy),
- saturation raised, the way GBC palettes are bold where FRLG is pastel,
- a contrast curve: lights toward GSC's paper white, darks toward its near-black.
Neutral greys only get the contrast curve. Index 0 (transparent) is left alone.
Only palettes change, so tiles, tilemaps and RAM stay the same.
"""
import argparse
import colorsys
import glob
import os

from PIL import Image

from devamp import gba, read_pal, write_pal

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DIRS = [os.path.join(ROOT, "graphics", "intro", "frlg")]

# (hue range in degrees, GSC target hue). Hues from the GSC sprites and the style guide.
FAMILIES = [
    ((60, 125), 88),     # grass and leaves: GSC lime
    ((125, 185), 150),   # Venusaur / tree teal-green: GSC's Venusaur green, a touch toward teal
    ((185, 255), 248),   # sky and navy: GSC's violet-blue
    ((255, 290), 274),   # Gengar: GSC (74,0,132)
    ((290, 345), 300),   # Nidorino: GSC (181,66,181)
    ((345, 360), 8),     # reds: GSC (255,41,16)
    ((0, 60), 12),
]


def gbcify(c, k):
    r, g, b = (v / 255 for v in c)
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    deg = h * 360
    # contrast curve around the middle, then lights a little whiter
    v2 = min(1.0, max(0.0, 0.5 + (v - 0.5) * (1 + 0.25 * k)))
    if s < 0.12:
        return gba(tuple(round(x * 255) for x in colorsys.hsv_to_rgb(h, s, v2)))
    for (lo, hi), target in FAMILIES:
        if lo <= deg < hi:
            d = (target - deg + 180) % 360 - 180
            deg = (deg + d * 0.5 * k) % 360
            break
    # bolder saturation in the mids; the brightest tones stay near paper
    s2 = min(1.0, s * (1 + 0.45 * k))
    if v2 > 0.9:
        s2 = s2 * (1 - 0.25 * k * (v2 - 0.9) / 0.1)
    return gba(tuple(round(x * 255) for x in colorsys.hsv_to_rgb(deg / 360, s2, v2)))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--strength", type=float, default=1.0)
    k = ap.parse_args().strength
    n = 0
    for d in DIRS:
        for p in glob.glob(os.path.join(d, "**", "*.pal"), recursive=True):
            cols = read_pal(p)
            write_pal(p, [c if i % 16 == 0 else gbcify(c, k) for i, c in enumerate(cols)])
            n += 1
        for p in glob.glob(os.path.join(d, "**", "*.png"), recursive=True):
            im = Image.open(p)
            if im.mode != "P":
                continue
            pl = im.getpalette()
            cols = [tuple(pl[i:i + 3]) for i in range(0, len(pl), 3)]
            im.putpalette([v for i, c in enumerate(cols) for v in (c if i % 16 == 0 else gbcify(c, k))])
            im.save(p)
            n += 1
    print("gbc colours applied to %d palettes (strength %.2f)" % (n, k))


if __name__ == "__main__":
    main()
