#!/usr/bin/env python3
"""Recolour the FireRed/LeafGreen intro art in Minamoto's style ("devamp").

    python3 tools/intro_devamp/devamp.py            # writes graphics/intro/frlg/
    python3 tools/intro_devamp/devamp.py --preview OUT.png

Same two steps as the devamped Red back sprite (variant E, art-direction/red-devamp/):

1. Palette remap. Every FRLG colour is replaced by a colour from the Minamoto kit
   (GSC hues at lower saturation, cool darks, warm lights, tinted outlines). Material
   ramps are mapped by brightness rank onto anchor ramps taken from the style guide
   (grass, foliage, neutral, wood) or from the Gen 2 sprite colours (Gengar, Nidorino).
2. Checker band. On the light side of every boundary between two tones of the same
   ramp, a 2 px band takes the darker tone on every other pixel, like GBC shading.

Pixel indices never change palette, so the intro keeps FRLG's palette slots, tile
counts and tilemaps. Background tilesets are deduplicated, so the band is computed
on the assembled screen (tilemap + flips) and every tile takes the majority result
of the places it appears.

Sources are pret/pokefirered's files, kept unchanged in tools/intro_devamp/frlg_src/.
"""
import argparse
import colorsys
import os
import struct
import sys
from collections import Counter

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "tools", "intro_devamp", "frlg_src")
DST = os.path.join(ROOT, "graphics", "intro", "frlg")


def hx(s):
    s = s.lstrip("#")
    return tuple(int(s[i:i + 2], 16) for i in (0, 2, 4))


def lum(c):
    r, g, b = c
    return 0.299 * r + 0.587 * g + 0.114 * b


def gba(c):
    """Round to the GBA's 5-bit grid, the way Minamoto art is authored."""
    return tuple(min(248, (v + 4) // 8 * 8) for v in c)


# ---------------------------------------------------------------------------
# 1. Palette: FRLG colour -> Minamoto colour, grouped in ramps.
#
# A ramp lists the FRLG colours that make up one material and the Minamoto anchor
# colours (light to dark) it maps onto. FRLG colours are sorted by brightness and
# spread over the anchors, so the number of steps is kept. With "pairs" the mapping
# is given colour by colour instead.
# ---------------------------------------------------------------------------
RAMPS = {
    # Grass: GSC's yellow-lime at lower saturation (style guide rule 1, "Grass").
    "grass": dict(
        old="a4ff83 a4ee8b 83ee73 8bde83 a4de62 94d552 73d562 83d573 73c56a 62c552 "
            "52bd4a 52b441 6ab429 6aac39 39a439 399439 29a408 107b10",
        anchors="c8f098 a7ea75 88d860 6dc155 58a850 49945b 2b7b46"),
    # Tree canopy and the grass close-up's shadows: the foliage ramp, teal in the
    # shadows, yellow-green in the light.
    "foliage": dict(
        old="9ce69c 83cd94 6abd73 62bd7b 6ab483 41945a 418b62 31734a 317362 187341 "
            "108b62 107362 185a5a 204a5a 204152",
        anchors="a8d880 78bc68 49945b 348c51 2b7b46 106342 205030 303830"),
    # Light shafts, sky and the misty forest: the neutral ramp (violet-grey shadows,
    # warm cream lights; no pure greys).
    "neutral": dict(
        old="eef6f6 dee6e6 dedee6 bdbdd5 b4bdc5 949cac 8b9cac 6a7b8b",
        anchors="f0eee0 e7e6db b7b6b0 999890 8898b0 6c6890"),
    # The deep forest behind the trees: cool blue-violet darks (GSC's Olivine roof
    # blue, Masara's dark glass) rather than FRLG's teal navy.
    "deep": dict(old="526273 4a5a73 29395a 20415a", anchors="585c80 485478 343c60 283458"),
    # Tree trunks: the wood ramp.
    "wood": dict(old="a48329 736231 524a39", anchors="a08c58 887850 504828"),
    # Gengar: GSC's Gengar is a deep blue-violet (74,0,132). Lights lean warm, darks cool.
    "gengar": dict(pairs={
        "c583b4": "705098", "a46294": "503878", "83417b": "382460",
        "5a395a": "281848", "41294a": "1c1034"}),
    "gengar_eye": dict(pairs={"ffb47b": "f8a070", "ee624a": "e8704a", "cd3920": "a63a1a"}),
    "teeth": dict(pairs={"ffffff": "fdfcf8", "dedede": "d0cec8", "7b7b7b": "8898b0"}),
    # Nidorino: GSC's magenta (181,66,181) in the light, blue-violet (82,41,132) in the dark.
    "nido": dict(pairs={
        "deb4cd": "e0b4dc", "bd83bd": "b87cc0", "8b5a94": "84589c",
        "6a3962": "5c3878", "5a2952": "482c60"}),
    "nido_ear": dict(pairs={"39eebd": "80e0b8", "52cd9c": "50b890", "31a473": "3c9c7c", "00a473": "2c8870"}),
    "nido_mouth": dict(pairs={"ff6a52": "e8704a", "de4129": "a63a1a", "b41800": "7a2410"}),
    "horn": dict(pairs={"cdcdde": "d8d6e0", "838383": "8888a0"}),
    # Shared outline: the dark grey of the Red sprite kit, never black.
    "outline": dict(pairs={"101010": "292929"}),
    # Game Freak scene: navy night, cream text, GSC-yellow star, lavender sparkles.
    "gf_navy": dict(pairs={"18294a": "282848"}),
    "gf_text": dict(pairs={"b4b4b4": "b7b6b0", "525252": "585080"}),
    "gf_logo": dict(pairs={"bdac10": "d0b038", "8b7b10": "887828"}),
    "gf_presents": dict(pairs={"94e631": "a7ea75", "83a441": "6dc155"}),
    "star": dict(pairs={"ffff10": "f8f080", "c5c508": "e8c040", "838308": "a87828", "414100": "584018"}),
    "sparkle": dict(pairs={
        "dedeee": "e0def0", "c5c5de": "c8c4e8", "a4a4cd": "a8a3e0", "8b8bbd": "8880d0",
        "6a6aac": "6b63c8", "52529c": "5850a0"}),
    # Gengar's swipe and Nidorino's dust: GSC water blue-violet instead of FRLG cyan.
    "swipe": dict(pairs={"eef6bd": "f0eee0", "cdeeff": "d8d8f8", "9cdeff": "a8a3ff",
                         "73d5ff": "8880f0", "bde6ff": "c8c4f8", "deeeff": "e0def8"}),
    # The orange/mint "transparent" fillers stay as they are; they never show.
}

# Ramps whose tone boundaries get the checker band. Text, logo, sparkles and effects
# stay flat.
BANDED = {"grass", "foliage", "neutral", "deep", "wood", "gengar", "nido", "nido_ear", "gengar_eye"}

# White appears in many palettes with different jobs; give it to the teeth ramp.
COLOUR_MAP = {}   # old rgb -> (new rgb, ramp)


def build_map():
    for name, r in RAMPS.items():
        if "pairs" in r:
            for o, n in r["pairs"].items():
                COLOUR_MAP.setdefault(hx(o), (gba(hx(n)), name))
            continue
        olds = sorted({hx(c) for c in r["old"].split()}, key=lum, reverse=True)
        anchors = [hx(c) for c in r["anchors"].split()]
        for i, o in enumerate(olds):
            t = i / max(1, len(olds) - 1) * (len(anchors) - 1)
            k = min(int(t), len(anchors) - 2)
            f = t - k
            a, b = anchors[k], anchors[k + 1]
            n = tuple(round(a[j] + (b[j] - a[j]) * f) for j in range(3))
            COLOUR_MAP.setdefault(o, (gba(n), name))


def remap(rgb):
    hit = COLOUR_MAP.get(tuple(rgb))
    return (hit[0], hit[1]) if hit else (tuple(rgb), None)


# ---------------------------------------------------------------------------
# Palettes and images
# ---------------------------------------------------------------------------
def read_pal(path):
    words = open(path).read().split()
    n = int(words[2])
    vals = list(map(int, words[3:3 + 3 * n]))
    return [tuple(vals[i:i + 3]) for i in range(0, 3 * n, 3)]


def write_pal(path, cols):
    with open(path, "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n%d\n" % len(cols))
        for c in cols:
            f.write("%d %d %d\n" % c)


def png_pal(im):
    p = im.getpalette()
    return [tuple(p[i:i + 3]) for i in range(0, len(p), 3)]


class Pal:
    """One 16-colour palette after the remap: colour, ramp and brightness per index."""

    def __init__(self, old16):
        self.old = list(old16) + [(0, 0, 0)] * (16 - len(old16))
        self.new, self.ramp = [], []
        for c in self.old[:16]:
            n, r = remap(c)
            self.new.append(n)
            self.ramp.append(r if r in BANDED else None)
        self.lum = [lum(c) for c in self.new]


# Regions smaller than this (spots, lines, glints) neither get a band nor cause one,
# so small details stay crisp and only the big shading planes are dithered.
MIN_REGION = 16


def regions(idx, pals, w, h):
    """Size of the same-colour 4-connected region each pixel belongs to."""
    size = [[0] * w for _ in range(h)]
    seen = [[False] * w for _ in range(h)]
    for y0 in range(h):
        for x0 in range(w):
            if seen[y0][x0]:
                continue
            i, p = idx[y0][x0], pals[y0][x0]
            stack, comp = [(x0, y0)], []
            seen[y0][x0] = True
            while stack:
                x, y = stack.pop()
                comp.append((x, y))
                for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
                    if 0 <= nx < w and 0 <= ny < h and not seen[ny][nx] \
                            and idx[ny][nx] == i and pals[ny][nx] is p:
                        seen[ny][nx] = True
                        stack.append((nx, ny))
            for x, y in comp:
                size[y][x] = len(comp)
    return size


def band(idx, pals, w, h, width=2, transparent0=True):
    """Checker band on the light side of same-ramp tone boundaries.

    idx[y][x] is a palette-local index, pals[y][x] the Pal it uses (or None).
    Returns a new index grid.
    """
    out = [row[:] for row in idx]
    size = regions(idx, pals, w, h)
    near = [(1, 0), (-1, 0), (0, 1), (0, -1)]
    far = [(2, 0), (-2, 0), (0, 2), (0, -2), (1, 1), (1, -1), (-1, 1), (-1, -1)]
    for y in range(h):
        for x in range(w):
            if (x + y) & 1:
                continue
            i = idx[y][x]
            p = pals[y][x]
            if p is None or (transparent0 and i == 0) or p.ramp[i] is None:
                continue
            # Leave 1-2 px details (glints, pupils) alone: the pixel needs two
            # neighbours of its own colour.
            same = sum(1 for dx, dy in near
                       if 0 <= x + dx < w and 0 <= y + dy < h
                       and idx[y + dy][x + dx] == i and pals[y + dy][x + dx] is p)
            if same < 2 or size[y][x] < MIN_REGION:
                continue
            best = None
            for ring in ((near, far) if width == 2 else (near,)):
                for dx, dy in ring:
                    nx, ny = x + dx, y + dy
                    if not (0 <= nx < w and 0 <= ny < h) or pals[ny][nx] is not p:
                        continue
                    j = idx[ny][nx]
                    if (transparent0 and j == 0) or p.ramp[j] != p.ramp[i] or p.lum[j] >= p.lum[i] \
                            or size[ny][nx] < MIN_REGION:
                        continue
                    if best is None or p.lum[j] > p.lum[best]:
                        best = j     # the closest darker tone
                if best is not None:
                    break
            if best is not None:
                out[y][x] = best
    return out


def load_indexed(path):
    im = Image.open(path)
    if im.mode != "P":
        return im, None
    w, h = im.size
    data = list(im.get_flattened_data() if hasattr(im, "get_flattened_data") else im.getdata())
    return im, [data[y * w:(y + 1) * w] for y in range(h)]


def save_indexed(im, grid, palette16, path):
    w, h = im.size
    out = Image.new("P", (w, h))
    flat = []
    for c in palette16:
        flat.extend(c)
    rest = png_pal(im)[len(palette16):]
    for c in rest:
        flat.extend(c)
    out.putpalette(flat[:768])
    out.putdata([v for row in grid for v in row])
    out.save(path, optimize=False)


# ---------------------------------------------------------------------------
# Asset list
# ---------------------------------------------------------------------------
# Sprite sheets: whole image is drawn as-is (frames are separate areas).
# Last field: checker band width in pixels (0 = flat).
# runtime_pal: the .pal the game loads for it (None = the PNG's own palette).
SPRITES = [
    ("game_freak/star.png", None, 0),
    ("game_freak/sparkles_small.png", "game_freak/sparkles.pal", 0),
    ("game_freak/sparkles_big.png", "game_freak/sparkles.pal", 0),
    ("game_freak/logo.png", None, 0),
    ("game_freak/presents.png", None, 0),
    # The wide shot's small mons get a 1 px band; 2 px would cover them.
    ("scene_2/gengar.png", "gengar.pal", 1),
    ("scene_2/nidorino.png", "nidorino.pal", 1),
    ("scene_3/gengar_static.png", "gengar.pal", 2),
    ("scene_3/nidorino.png", "nidorino.pal", 2),
    ("scene_3/grass.png", None, 2),
    ("scene_3/swipe.png", None, 0),
    ("scene_3/recoil_dust.png", None, 0),
]

# Background layers: tileset PNG + tilemap; slots maps the tilemap's palette number
# to the palette (from a .pal, or the PNG's own) the game loads there.
#   (png, bin, {map palette: (pal file or None, index of the 16-colour block)}, banded)
BGS = [
    ("game_freak/bg.png", "game_freak/bg.bin", {0: ("game_freak/bg.pal", 0)}, False),
    ("scene_1/grass.png", "scene_1/grass.bin", {1: (None, 0)}, True),
    ("scene_1/bg.png", "scene_1/bg.bin", {2: (None, 0)}, True),
    ("scene_2/bg.png", "scene_2/bg.bin",
     {1: ("scene_2/bg.pal", 0), 2: ("scene_2/bg.pal", 1), 3: ("scene_2/bg.pal", 2)}, True),
    ("scene_2/plants.png", "scene_2/plants.bin", {1: ("scene_2/bg.pal", 0)}, True),
    ("scene_2/gengar_close.png", "scene_2/gengar_close.bin", {5: ("gengar.pal", 0)}, True),
    ("scene_2/nidorino_close.png", "scene_2/nidorino_close.bin", {6: ("scene_2/nidorino_close.pal", 0)}, True),
    ("scene_3/bg.png", "scene_3/bg.bin", {1: ("scene_3/bg.pal", 0), 2: ("scene_3/bg.pal", 1)}, True),
    ("scene_3/gengar_anim.png", "scene_3/gengar_anim.bin", {5: ("gengar.pal", 0)}, True),
]

PALS = ["game_freak/bg.pal", "game_freak/sparkles.pal", "gengar.pal", "nidorino.pal",
        "scene_2/bg.pal", "scene_2/nidorino_close.pal", "scene_3/bg.pal"]


def pal16(src_png_im, palfile, block):
    if palfile:
        cols = read_pal(os.path.join(SRC, palfile))
        return cols[block * 16:block * 16 + 16]
    return png_pal(src_png_im)[:16]


def do_sprite(rel, palfile, banded, dst):
    im, grid = load_indexed(os.path.join(SRC, rel))
    p = Pal(pal16(im, palfile, 0))
    if banded:
        w, h = im.size
        grid = band(grid, [[p] * w for _ in range(h)], w, h, banded)
    own = Pal(png_pal(im)[:16])   # the PNG's own palette, remapped, for the .gbapal built from it
    save_indexed(im, grid, own.new, os.path.join(dst, rel))


def do_bg(rel, binrel, slots, banded, dst):
    im, grid = load_indexed(os.path.join(SRC, rel))
    tw = im.width // 8
    ntiles = tw * (im.height // 8)
    raw = open(os.path.join(SRC, binrel), "rb").read()
    ents = struct.unpack("<%dH" % (len(raw) // 2), raw)
    pals = {k: Pal(pal16(im, f, b)) for k, (f, b) in slots.items()}
    if banded:
        mw = 32
        mh = (len(ents) + mw - 1) // mw
        W, H = mw * 8, mh * 8
        idx = [[0] * W for _ in range(H)]
        pp = [[None] * W for _ in range(H)]
        for n, e in enumerate(ents):
            t, hf, vf, pn = e & 0x3FF, e >> 10 & 1, e >> 11 & 1, e >> 12
            if t >= ntiles or pn not in pals:
                continue
            ox, oy = n % mw * 8, n // mw * 8
            tx, ty = t % tw * 8, t // tw * 8
            for y in range(8):
                for x in range(8):
                    sx = 7 - x if hf else x
                    sy = 7 - y if vf else y
                    idx[oy + y][ox + x] = grid[ty + sy][tx + sx]
                    pp[oy + y][ox + x] = pals[pn]
        res = band(idx, pp, W, H)
        votes = {}
        for n, e in enumerate(ents):
            t, hf, vf, pn = e & 0x3FF, e >> 10 & 1, e >> 11 & 1, e >> 12
            if t >= ntiles or pn not in pals:
                continue
            ox, oy = n % mw * 8, n // mw * 8
            v = votes.setdefault(t, [[Counter() for _ in range(8)] for _ in range(8)])
            for y in range(8):
                for x in range(8):
                    sx = 7 - x if hf else x
                    sy = 7 - y if vf else y
                    v[sy][sx][res[oy + y][ox + x]] += 1
        for t, v in votes.items():
            tx, ty = t % tw * 8, t // tw * 8
            for y in range(8):
                for x in range(8):
                    grid[ty + y][tx + x] = v[y][x].most_common(1)[0][0]
    own = Pal(png_pal(im)[:16])
    save_indexed(im, grid, own.new, os.path.join(dst, rel))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=DST)
    args = ap.parse_args()
    build_map()
    for rel in PALS:
        cols = read_pal(os.path.join(SRC, rel))
        write_pal(os.path.join(args.out, rel), [remap(c)[0] for c in cols])
    for rel, palfile, banded in SPRITES:
        do_sprite(rel, palfile, banded, args.out)
    for rel, binrel, slots, banded in BGS:
        do_bg(rel, binrel, slots, banded, args.out)
    # game_freak.png is greyscale: it uses the logo palette's text greys (13-15).
    unknown = set()
    for rel in PALS:
        for c in read_pal(os.path.join(SRC, rel)):
            if tuple(c) not in COLOUR_MAP and c not in ((0, 0, 0), (255, 0, 255)):
                unknown.add("%02x%02x%02x" % c)
    if unknown:
        print("colours left as FRLG:", " ".join(sorted(unknown)), file=sys.stderr)


if __name__ == "__main__":
    main()
