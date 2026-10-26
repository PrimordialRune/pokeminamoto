#!/usr/bin/env python3
"""Minamoto's own title screen: paper, brown zigzag bands and pencil sketches.

    python3 tools/intro_devamp/title_minamoto.py                 # sources -> graphics
    python3 tools/intro_devamp/title_minamoto.py --placeholders  # (re)draw placeholder sources

Two styles share the engine in src/title_screen.c (TITLE_STYLE in that file):
- sketchbook: ruled notebook paper; each page is one wide pencil vista revealed
  left to right.
- flipbook: graph paper model sheets; each page is three rough creatures, drawn
  one at a time top to bottom in the slots x 0-79, 80-159, 160-239.

Sources, all 240x160, in tools/intro_devamp/src/title/ (replace them with real art):
- paper_<style>.png   opaque, at most 15 colours (BG3, palette 14). The bands are
                       part of it.
- press_start_<style>.png  transparent except the PRESS START text, at most 5
                       colours (BG2, palette 15). It blinks by turning those colours
                       into the paper colour under the text.
- <style>_<page>a.png, <style>_<page>b.png  one sketch page and its "line boil"
                       twin (same drawing, wobbled), transparent background. All
                       pages of a style share one palette: 15 colours at most
                       (BG1, palette 13). Pages are numbered from 1.
Each frame (a or b) may use 497 unique 8x8 tiles (flips count as the same): a
loads into char block 1, b into char block 2. The paper and PRESS START share
char block 3 and 177 tiles. sketch_pages.py makes the sketchbook pages from scans.

Outputs go to graphics/title_screen/minamoto/: tile sheets paper_<style>.png (paper
and text) and sketch_<style>_<page>_<a|b>.png (gbagfx makes the .4bpp), <name>.bin
tilemaps (32x20) and .pal palettes. make runs this when a
source changes (needs Pillow).
"""
import argparse
import glob
import math
import os
import random
import struct

from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", ".."))
SRC = os.path.join(HERE, "src", "title")
DST = os.path.join(ROOT, "graphics", "title_screen", "minamoto")
STYLES = ("sketchbook", "flipbook")
W, H = 240, 160
SKETCH_PAL, PAPER_PAL, TEXT_PAL = 13, 14, 15
# The tile sheets are padded to 16 tiles a row and gbagfx keeps the padding, so
# the limits leave room for 15 blank tiles.
MAX_SKETCH_TILES = 512 - 15   # one char block per boil frame
MAX_PAPER_TILES = 192 - 15    # char block 3 ends where the tilemaps start (screen block 27)


def gba(c):
    return tuple((v >> 3) << 3 for v in c[:3])


# ---------------------------------------------------------------- conversion

def flips(t):
    rows = [t[i * 8:i * 8 + 8] for i in range(8)]
    h = tuple(v for r in rows for v in r[::-1])
    v = tuple(v for r in rows[::-1] for v in r)
    hv = tuple(v for r in rows[::-1] for v in r[::-1])
    return ((t, 0), (h, 1), (v, 2), (hv, 3))


class Sheet:
    """Deduplicated 4bpp tiles (with flips); tile 0 is blank."""

    def __init__(self):
        self.tiles = [tuple([0] * 64)]
        self.lookup = {self.tiles[0]: (0, 0)}

    def add(self, t):
        for f, fl in flips(t):
            if f in self.lookup:
                i, g = self.lookup[f]
                return i, g ^ fl
        self.lookup[t] = (len(self.tiles), 0)
        self.tiles.append(t)
        return len(self.tiles) - 1, 0

    def tilemap(self, idx, pal):
        out = []
        for ty in range(20):
            for tx in range(32):
                if tx >= 30:
                    out.append(pal << 12)
                    continue
                t = tuple(idx[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8))
                i, fl = self.add(t)
                out.append(i | fl << 10 | pal << 12)
        return out

    def save(self, path, pal):
        cols = 16
        rows = (len(self.tiles) + cols - 1) // cols
        im = Image.new("P", (cols * 8, rows * 8))
        im.putpalette([v for c in pal + [(0, 0, 0)] * (256 - len(pal)) for v in c])
        px = im.load()
        for i, t in enumerate(self.tiles):
            for k, v in enumerate(t):
                px[i % cols * 8 + k % 8, i // cols * 8 + k // 8] = v
        im.save(path)


def write_bin(path, words):
    with open(path, "wb") as f:
        f.write(struct.pack("<%dH" % len(words), *words))


def write_pal(path, cols):
    cols = (list(cols) + [(0, 0, 0)] * 16)[:16]
    with open(path, "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n16\n")
        for c in cols:
            f.write("%d %d %d\n" % c)


def indexed(im, pal, opaque=False):
    """Map an RGBA image onto pal (list, index 0 = transparent); returns rows of indices."""
    im = im.convert("RGBA")
    if im.size != (W, H):
        raise SystemExit("%s is %dx%d, must be %dx%d" % (getattr(im, "filename", "image"), *im.size, W, H))
    rows = []
    for y in range(H):
        row = []
        for x in range(W):
            r, g, b, a = im.getpixel((x, y))
            if a < 128 and not opaque:
                row.append(0)
                continue
            c = gba((r, g, b))
            row.append(pal.index(c, 1) if opaque else pal.index(c))
        rows.append(row + [0] * 16)
    return rows


def pixels(im, mode="RGBA"):
    b = im.convert(mode).tobytes()
    n = len(mode)
    return [tuple(b[i:i + n]) for i in range(0, len(b), n)]


def colours(ims, opaque=False):
    seen = []
    for im in ims:
        for r, g, b, a in pixels(im):
            if a < 128 and not opaque:
                continue
            c = gba((r, g, b))
            if c not in seen:
                seen.append(c)
    return seen


GHOST = 0.6   # how far the pencil fades toward the paper while the logo is up


def ghost(c, paper):
    """A pencil colour faded toward the paper colour."""
    return gba(tuple(round(v + (p - v) * GHOST) for v, p in zip(c, paper)))


def convert(style):
    os.makedirs(DST, exist_ok=True)
    out = lambda n: os.path.join(DST, n)
    # paper + bands
    paper = Image.open(os.path.join(SRC, "paper_%s.png" % style))
    # index 0 of a BG palette is transparent (the backdrop would show), so the
    # paper's colours start at 1
    # most used colour first: the C code fades erased pages into entry 1
    count = {}
    for c in pixels(paper, "RGB"):
        count[gba(c)] = count.get(gba(c), 0) + 1
    pc = [(0, 0, 0)] + sorted(count, key=lambda c: -count[c])
    if len(pc) > 16:
        raise SystemExit("paper_%s.png has %d colours; 15 at most" % (style, len(pc) - 1))
    sheet = Sheet()
    sheet.tiles = []
    sheet.lookup = {}
    words = sheet.tilemap(indexed(paper, pc, opaque=True), PAPER_PAL)
    npaper = len(sheet.tiles)
    write_bin(out("paper_%s.bin" % style), words)
    write_pal(out("paper_%s.pal" % style), pc)

    # PRESS START: colours 1-5, entry 6 = the paper under the text (blink colour)
    ps = Image.open(os.path.join(SRC, "press_start_%s.png" % style)).convert("RGBA")
    tc = colours([ps])
    if len(tc) > 5:
        raise SystemExit("press_start_%s.png has %d colours; 5 at most" % (style, len(tc)))
    under = {}
    for (r, g, b, a), p in zip(pixels(ps), pixels(paper, "RGB")):
        if a >= 128:
            under[gba(p)] = under.get(gba(p), 0) + 1
    blink = max(under, key=under.get) if under else (0, 0, 0)
    tpal = [(0, 0, 0)] + tc + [tc[-1]] * (5 - len(tc)) + [blink]
    # the text's tiles join the paper's sheet (BG2 and BG3 share char block 3)
    words = sheet.tilemap(indexed(ps, tpal[:len(tc) + 1]), TEXT_PAL)
    if len(sheet.tiles) > MAX_PAPER_TILES:
        raise SystemExit("paper_%s.png and press_start_%s.png need %d tiles; %d at most"
                         % (style, style, len(sheet.tiles), MAX_PAPER_TILES))
    sheet.save(out("paper_%s.png" % style), pc)
    print("%s paper + text: %d + %d tiles" % (style, npaper, len(sheet.tiles) - npaper))
    write_bin(out("press_start_%s.bin" % style), words)
    write_pal(out("press_start_%s.pal" % style), tpal)

    # sketch pages
    pages = []
    n = 1
    while os.path.exists(os.path.join(SRC, "%s_%da.png" % (style, n))):
        pages.append((Image.open(os.path.join(SRC, "%s_%da.png" % (style, n))),
                      Image.open(os.path.join(SRC, "%s_%db.png" % (style, n)))))
        n += 1
    if not pages:
        raise SystemExit("no %s_1a.png in %s" % (style, SRC))
    sc = colours([im for p in pages for im in p])
    if len(sc) > 15:
        raise SystemExit("%s pages use %d colours; 15 at most" % (style, len(sc)))
    spal = [(0, 0, 0)] + sc
    write_pal(out("sketch_%s.pal" % style), spal)
    # the same lines faded toward the paper, shown while the logo is up
    write_pal(out("sketch_%s_ghost.pal" % style), [(0, 0, 0)] + [ghost(c, pc[1]) for c in sc])
    for i, page in enumerate(pages, 1):
        counts = []
        for frame, im in zip("ab", page):
            sheet = Sheet()
            words = sheet.tilemap(indexed(im, spal), SKETCH_PAL)
            if len(sheet.tiles) > MAX_SKETCH_TILES:
                raise SystemExit("%s_%d%s.png needs %d tiles; %d at most" % (style, i, frame, len(sheet.tiles), MAX_SKETCH_TILES))
            sheet.save(out("sketch_%s_%d_%s.png" % (style, i, frame)), spal)
            write_bin(out("sketch_%s_%d_%s.bin" % (style, i, frame)), words)
            counts.append(len(sheet.tiles))
        print("%s page %d: %d + %d tiles" % (style, i, *counts))
    print("%s: paper %d colours, %d pages" % (style, len(pc), len(pages)))


# ---------------------------------------------------------------- placeholders

PAPER = (240, 232, 208); FIBRE = (224, 216, 192)
RULE = (200, 212, 224); MARGIN = (232, 168, 160); GRID = (216, 216, 208)
BR_DK = (64, 40, 24); BR = (112, 64, 40); BR_LT = (160, 104, 64); CREAM = (248, 240, 216)
PEN = (88, 80, 96); PEN_MID = (136, 128, 136); PEN_LT = (184, 176, 176)
TOP_BAND, BOTTOM_BAND = 10, 142


def bands(d):
    """Brown bands, zigzag teeth toward the paper and a lighter zigzag stripe inside."""
    d.rectangle([0, 0, W, TOP_BAND - 1], fill=BR_DK)
    d.rectangle([0, BOTTOM_BAND, W, H], fill=BR_DK)
    for x in range(0, W, 8):
        d.polygon([(x, TOP_BAND - 1), (x + 4, TOP_BAND + 3), (x + 8, TOP_BAND - 1)], fill=BR_DK)
        d.line([(x, 4), (x + 4, 8)], fill=BR); d.line([(x + 4, 8), (x + 8, 4)], fill=BR)
        d.line([(x, 3), (x + 4, 7)], fill=BR_LT); d.line([(x + 4, 7), (x + 8, 3)], fill=BR_LT)
        d.polygon([(x, BOTTOM_BAND), (x + 4, BOTTOM_BAND - 4), (x + 8, BOTTOM_BAND)], fill=BR_DK)
        d.line([(x, BOTTOM_BAND + 6), (x + 4, BOTTOM_BAND + 2)], fill=BR); d.line([(x + 4, BOTTOM_BAND + 2), (x + 8, BOTTOM_BAND + 6)], fill=BR)
        d.line([(x, BOTTOM_BAND + 5), (x + 4, BOTTOM_BAND + 1)], fill=BR_LT); d.line([(x + 4, BOTTOM_BAND + 1), (x + 8, BOTTOM_BAND + 5)], fill=BR_LT)


def fibres(d):
    rnd = random.Random(3)
    cell = [(rnd.randrange(32), rnd.randrange(32)) for _ in range(5)]   # repeats every 32 px: few tiles
    for y0 in range(0, H, 32):
        for x0 in range(0, W, 32):
            for x, y in cell:
                d.point((x0 + x, y0 + y), fill=FIBRE)


def paper_sketchbook():
    im = Image.new("RGB", (W, H), PAPER); d = ImageDraw.Draw(im)
    fibres(d)
    for y in range(4, H, 8):
        d.line([0, y, W, y], fill=RULE)
    d.line([12, 0, 12, H], fill=MARGIN)
    bands(d)
    return im


def paper_flipbook():
    im = Image.new("RGB", (W, H), PAPER); d = ImageDraw.Draw(im)
    fibres(d)
    for x in range(0, W, 16):
        d.line([x, 0, x, H], fill=GRID)
    for y in range(0, H, 16):
        d.line([0, y, W, y], fill=GRID)
    bands(d)
    return im


def press_start():
    """PRESS START from FRLG's glyph tiles, cream with a dark rim, centred in the bottom band."""
    src = Image.open(os.path.join(ROOT, "tools", "intro_devamp", "frlg_src", "title", "copyright_press_start.png"))
    tiles = [21, 22, 23, 24, 25, 26, 53, 54, 55, 56, 57, 58]
    glyph = Image.new("P", (96, 8)); glyph.putpalette(src.getpalette())
    for i, t in enumerate(tiles):
        glyph.paste(src.crop((t % 16 * 8, t // 16 * 8, t % 16 * 8 + 8, t // 16 * 8 + 8)), (i * 8, 0))
    gx = glyph.load()
    xs = [x for x in range(96) for y in range(8) if gx[x, y] & 15 not in (0, 6)]
    ys = [y for x in range(96) for y in range(8) if gx[x, y] & 15 not in (0, 6)]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    im = Image.new("RGBA", (W, H)); px = im.load()
    ox = (W - (x1 - x0 + 1)) // 2 - x0
    oy = BOTTOM_BAND + 9 - y0
    for y in range(8):
        for x in range(96):
            v = gx[x, y] & 15
            if v not in (0, 6):
                px[ox + x, oy + y] = CREAM + (255,) if v in (3, 4, 5) else BR_LT + (255,)
    return im


def jline(d, pts, rnd, c=PEN, j=0.55):
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        n = max(2, int(math.hypot(x1 - x0, y1 - y0) / 3))
        prev = (x0, y0)
        for i in range(1, n + 1):
            t = i / n
            p = (round(x0 + (x1 - x0) * t + rnd.uniform(-j, j)), round(y0 + (y1 - y0) * t + rnd.uniform(-j, j)))
            d.line([prev, p], fill=c)
            prev = p


def ellipse(cx, cy, rx, ry, a0=0, a1=360, n=24):
    return [(cx + math.cos(math.radians(a0 + (a1 - a0) * i / n)) * rx,
             cy + math.sin(math.radians(a0 + (a1 - a0) * i / n)) * ry) for i in range(n + 1)]


def hatch(d, rnd, x0, y0, x1, y1, step=3, c=PEN_LT, slope=4):
    for x in range(x0, x1, step):
        jline(d, [(x, y1), (x + slope, y0)], rnd, c, 0.3)


def house(d, rnd, x, y, w, h):
    jline(d, [(x, y), (x, y + h), (x + w, y + h), (x + w, y)], rnd)
    jline(d, [(x - 2, y + 1), (x + w / 2, y - h * 0.7), (x + w + 2, y + 1), (x - 2, y + 1)], rnd)
    hatch(d, rnd, int(x), int(y - h * 0.5), int(x + w), int(y), 3, PEN_LT, 2)
    jline(d, [(x + w * 0.4, y + h), (x + w * 0.4, y + h * 0.45), (x + w * 0.65, y + h * 0.45), (x + w * 0.65, y + h)], rnd, PEN_MID)


def tree(d, rnd, x, y, s=1.0):
    jline(d, ellipse(x, y, 5 * s, 6 * s, -90, 270, 12), rnd)
    jline(d, [(x, y + 6 * s), (x, y + 11 * s)], rnd, PEN_MID)
    hatch(d, rnd, int(x), int(y - 2), int(x + 4 * s), int(y + 5), 2, PEN_LT, 1)


def sketchbook_page(n, boil):
    im = Image.new("RGBA", (W, H)); d = ImageDraw.Draw(im)
    rnd = random.Random(100 * n + boil)
    if n == 1:   # Masara from the hill, a giant horned monster behind it
        k = [(176, 140), (178, 110), (183, 96), (186, 84), (183, 70), (189, 60), (196, 64), (200, 50), (204, 64),
             (213, 70), (210, 84), (222, 90), (228, 104), (220, 110), (218, 126), (226, 140)]
        jline(d, k, rnd, PEN_MID)
        jline(d, [(196, 72), (199, 71)], rnd, PEN)
        hatch(d, rnd, 180, 96, 214, 140, 4, PEN_LT, 3)
        jline(d, [(14, 116), (60, 106), (110, 112), (160, 100), (240, 110)], rnd)
        jline(d, [(14, 128), (80, 124), (150, 130), (240, 126)], rnd, PEN_MID)
        for hx in (36, 60, 98):
            house(d, rnd, hx, 114, 14, 10)
        for tx in (22, 124, 140, 156, 232):
            tree(d, rnd, tx, 104)
        jline(d, [(70, 140), (80, 128), (96, 126), (110, 140)], rnd, PEN_LT)
    else:        # Route 1: a path running into the distance, tall grass, a bird overhead
        jline(d, [(60, 141), (100, 120), (112, 106), (116, 98)], rnd)
        jline(d, [(170, 141), (138, 120), (124, 106), (120, 98)], rnd)
        jline(d, [(14, 100), (90, 96), (150, 97), (240, 94)], rnd, PEN_MID)
        for x in range(20, 80, 7):
            jline(d, [(x, 136), (x + 2, 126), (x + 4, 136)], rnd, PEN_MID)
        for x in range(176, 236, 7):
            jline(d, [(x, 134), (x + 2, 124), (x + 4, 134)], rnd, PEN_MID)
        for tx in (24, 44, 196, 216, 232):
            tree(d, rnd, tx, 90, 1.2)
        jline(d, [(150, 64), (160, 58), (168, 62), (176, 56), (186, 60)], rnd)
        jline(d, [(166, 62), (168, 66)], rnd, PEN_MID)
        jline(d, [(196, 76), (200, 73), (204, 76), (208, 73)], rnd, PEN_LT)
    return im


CREATURES = {
    # each: list of (kind, args); drawn centred on the slot, feet at y 140
    "seed":   [("circle", (0, -14, 13, 11), PEN_LT), ("poly", [(-14, -2), (-12, -16), (-2, -26), (10, -22), (15, -10), (12, -1), (-14, -2)], PEN),
               ("poly", [(-10, -1), (-11, 0)], PEN), ("poly", [(-2, -26), (-6, -36), (2, -40), (8, -32), (-2, -26)], PEN),
               ("eye", (6, -16)), ("legs", (-8, 6))],
    "lizard": [("circle", (0, -18, 9, 12), PEN_LT), ("poly", [(-6, 0), (-8, -14), (-6, -26), (2, -34), (10, -30), (10, -20), (6, -10), (6, 0)], PEN),
               ("poly", [(6, -6), (14, -4), (20, -10), (22, -18), (18, -24)], PEN), ("poly", [(18, -24), (16, -30), (20, -28), (22, -32), (23, -24)], PEN_MID),
               ("eye", (5, -28)), ("legs", (-4, 4))],
    "turtle": [("circle", (0, -12, 15, 10), PEN_LT), ("poly", [(-16, -6), (-12, -18), (0, -22), (12, -18), (16, -6), (-16, -6)], PEN),
               ("poly", [(14, -12), (20, -18), (26, -16), (24, -8), (16, -8)], PEN), ("grid", (-10, -16, 20, 8)),
               ("eye", (22, -14)), ("legs", (-10, 10))],
    "bird":   [("circle", (0, -18, 10, 8), PEN_LT), ("poly", [(-10, -16), (-4, -24), (6, -24), (12, -18), (8, -10), (-6, -10), (-10, -16)], PEN),
               ("poly", [(12, -18), (18, -17), (12, -15)], PEN), ("poly", [(-4, -20), (-14, -32), (-8, -20)], PEN_MID),
               ("eye", (7, -20)), ("legs", (-2, 3))],
    "bug":    [("circle", (0, -10, 14, 8), PEN_LT), ("poly", [(-16, -8), (-10, -14), (0, -16), (10, -14), (16, -8), (10, -2), (-10, -2), (-16, -8)], PEN),
               ("poly", [(14, -12), (18, -20)], PEN), ("poly", [(10, -14), (12, -22)], PEN),
               ("eye", (11, -9)), ("legs", (-10, 7))],
    "rodent": [("circle", (0, -12, 11, 11), PEN_LT), ("poly", [(-10, -2), (-12, -12), (-6, -22), (4, -24), (12, -16), (10, -4), (-10, -2)], PEN),
               ("poly", [(-6, -22), (-8, -32), (-2, -24)], PEN), ("poly", [(4, -24), (6, -34), (8, -22)], PEN),
               ("poly", [(-10, -6), (-18, -8), (-20, -16), (-16, -22)], PEN_MID), ("eye", (5, -16)), ("legs", (-6, 6))],
}
FLIP_PAGES = {1: ("seed", "lizard", "turtle"), 2: ("bird", "bug", "rodent")}


def creature(d, rnd, cx, base, name):
    for kind, args, *c in CREATURES[name]:
        if kind == "circle":
            x, y, rx, ry = args
            jline(d, ellipse(cx + x, base + y, rx, ry), rnd, c[0], 0.4)
            jline(d, [(cx + x - rx, base + y), (cx + x + rx, base + y)], rnd, c[0], 0.3)
        elif kind == "poly":
            jline(d, [(cx + x, base + y) for x, y in args], rnd, c[0])
        elif kind == "eye":
            x, y = args
            d.rectangle([cx + x, base + y, cx + x + 1, base + y + 1], fill=PEN)
        elif kind == "legs":
            x, step = args
            for i in range(2):
                jline(d, [(cx + x + i * step * 2, base - 2), (cx + x + i * step * 2, base + 1), (cx + x + i * step * 2 + 3, base + 1)], rnd, PEN)
        elif kind == "grid":
            x, y, w, h = args
            hatch(d, rnd, cx + x, base + y, cx + x + w, base + y + h, 4, PEN_MID, 2)


def flipbook_page(n, boil):
    im = Image.new("RGBA", (W, H)); d = ImageDraw.Draw(im)
    rnd = random.Random(200 * n + boil)
    for slot, name in enumerate(FLIP_PAGES[n]):
        creature(d, rnd, 40 + slot * 80, 137, name)
    return im


def placeholders(force):
    os.makedirs(SRC, exist_ok=True)
    made = {
        "paper_sketchbook.png": paper_sketchbook(), "paper_flipbook.png": paper_flipbook(),
        "press_start_sketchbook.png": press_start(), "press_start_flipbook.png": press_start(),
    }
    for n in (1, 2):
        for boil, s in ((0, "a"), (1, "b")):
            made["sketchbook_%d%s.png" % (n, s)] = sketchbook_page(n, boil)
            made["flipbook_%d%s.png" % (n, s)] = flipbook_page(n, boil)
    for name, im in made.items():
        p = os.path.join(SRC, name)
        if os.path.exists(p) and not force:
            print("kept", name)
            continue
        im.save(p)
        print("drew", name)


def main():
    global DST
    ap = argparse.ArgumentParser()
    ap.add_argument("--placeholders", action="store_true", help="draw missing placeholder sources")
    ap.add_argument("--force", action="store_true", help="with --placeholders: overwrite existing sources")
    ap.add_argument("--out", default=DST)
    args = ap.parse_args()
    DST = args.out
    if args.placeholders:
        placeholders(args.force)
        return
    for style in STYLES:
        convert(style)


if __name__ == "__main__":
    main()
