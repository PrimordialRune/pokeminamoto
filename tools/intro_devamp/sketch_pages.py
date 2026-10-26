#!/usr/bin/env python3
"""Turn scanned pencil sketches into sketchbook title pages.

    python3 tools/intro_devamp/sketch_pages.py SCAN_DIR [--corner] [--preview out.png]

SCAN_DIR holds the scans named in PAGES (the 1990 Capsule Monsters sketches;
they are not in the repo, Omar's copies are in the project files under
intro-remake/sketches/). For each page this writes
tools/intro_devamp/src/title/sketchbook_<n>a.png and its line-boil twin
sketchbook_<n>b.png (the same scan sampled half a pixel off), 240x160,
transparent. By default each page is a full-width scene (PAGES) that the logo
covers; the title fades it to ghost lines while the logo is up. --corner makes
small vignettes in the bottom-right corner instead, clear of the logo (CORNER: a
tight crop on the main subject, two pencil tones); build those with
TITLE_SKETCH_CORNER set to TRUE in src/title_screen.c. title_minamoto.py then
turns the pages into tiles.

Method: grayscale, crop, thicken the lines with a min filter (scans are drawn
with thin pencil that would vanish when shrunk), box-downscale to the page size,
then three thresholds map darkness to three pencil greys; lone light specks go.
"""
import argparse
import os

from PIL import Image, ImageFilter

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "src", "title")
W, H = 240, 160
SCENE_TOP, SCENE_H = 14, 124        # between the top band's teeth and the bottom band's
PEN = (72, 64, 80); PEN_MID = (128, 120, 128); PEN_LT = (176, 168, 168)
PAPER = (240, 232, 208)

# page: scan, crop box in scan pixels, darkness thresholds (dark, mid, light), line grow
PAGES = [
    ("1_forest.png",             (40, 90, 715, 705),   (0.80, 0.66, 0.52), 3),
    ("2_gorse_vs_yadon.webp",    (60, 120, 600, 450),  (0.70, 0.50, 0.36), 3),
    ("3_karabajio_vs_mimi.webp", (140, 30, 600, 385),  (0.55, 0.32, 0.17), None),
    ("4_trade.webp",             (60, 30, 620, 425),   (0.55, 0.32, 0.17), None),
    ("5_lapras_ride.jpg",        (10, 20, 1270, 692),  (0.55, 0.32, 0.17), None),
]

# Corner vignettes: the free paper right of the logo's banner and above the
# bottom band, x 142-234, y 90-137. Must match SKETCH_LEFT in src/title_screen.c.
CORNER_W, CORNER_H, CORNER_RIGHT, CORNER_BOTTOM = 92, 48, 234, 137
CORNER = [
    ("1_forest.png",             (215, 185, 410, 425), (0.70, 0.45, 2.0), None),
    ("2_gorse_vs_yadon.webp",    (95, 180, 320, 410),  (0.70, 0.45, 2.0), None),
    ("3_karabajio_vs_mimi.webp", (200, 50, 570, 330),  (0.70, 0.45, 2.0), None),
    ("4_trade.webp",             (150, 50, 420, 370),  (0.70, 0.45, 2.0), None),
    ("5_lapras_ride.jpg",        (170, 50, 760, 560),  (0.70, 0.45, 2.0), None),
]


def scene(path, box, t, grow, shift=0.0, corner=False):
    im = Image.open(path).convert("L")
    x0, y0, x1, y1 = box
    if corner:
        h = CORNER_H
        w = round((x1 - x0) / (y1 - y0) * h)
        if w > CORNER_W:
            w = CORNER_W
            h = round((y1 - y0) / (x1 - x0) * w)
        ox, oy = CORNER_RIGHT - w, CORNER_BOTTOM + 1 - h
    else:
        h = SCENE_H
        w = min(round((x1 - x0) / (y1 - y0) * h), W)
        ox, oy = (W - w) // 2, SCENE_TOP
    sc = (y1 - y0) / h
    s = shift * sc
    im = im.crop((round(x0 + s), round(y0 + s), round(x1 + s), round(y1 + s)))
    k = grow or max(3, int(sc / 2) | 1)
    if k > 1:
        im = im.filter(ImageFilter.MinFilter(k))
    px = im.resize((w, h), Image.BOX).load()
    lvl = [[0] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            d = 1 - px[x, y] / 255
            lvl[y][x] = 3 if d > t[0] else 2 if d > t[1] else 1 if d > t[2] else 0
    out = Image.new("RGBA", (W, H))
    o = out.load()
    for y in range(h):
        for x in range(w):
            v = lvl[y][x]
            if v in (1, 2) and not any(lvl[y + dy][x + dx] for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                                       if (dx or dy) and 0 <= x + dx < w and 0 <= y + dy < h):
                continue
            if v:
                o[ox + x, oy + y] = (PEN_LT, PEN_MID, PEN)[v - 1] + (255,)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("scans")
    ap.add_argument("--corner", action="store_true", help="small vignettes in the bottom-right corner")
    ap.add_argument("--preview", help="also write a sheet of all pages on paper, 2x")
    args = ap.parse_args()
    pages = CORNER if args.corner else PAGES
    made = []
    for n, (name, box, t, grow) in enumerate(pages, 1):
        path = os.path.join(args.scans, name)
        for s, shift in (("a", 0.0), ("b", 0.5)):
            im = scene(path, box, t, grow, shift, args.corner)
            im.save(os.path.join(OUT, "sketchbook_%d%s.png" % (n, s)))
            made.append(im)
        print("page %d: %s" % (n, name))
    if args.preview:
        sheet = Image.new("RGB", (2 * (W + 8), len(pages) * (H + 8)), (40, 40, 40))
        for i, im in enumerate(made):
            bg = Image.new("RGBA", (W, H), PAPER + (255,))
            bg.alpha_composite(im)
            sheet.paste(bg.convert("RGB"), (i % 2 * (W + 8), i // 2 * (H + 8)))
        sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(args.preview)


if __name__ == "__main__":
    main()
