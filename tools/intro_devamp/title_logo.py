#!/usr/bin/env python3
"""Pixel version of Omar's ポケットモンスター源 logo for the title screen.

    python3 tools/intro_devamp/title_logo.py [--preview OUT.png]

Source: tools/intro_devamp/src/logo_minamoto_small.png (copy of
PKMN_Minamoto/MyRes/logo/logo_minamoto_small.png). Writes the LeafGreen title's
logo layer (BG0, 8bpp) in graphics/title_screen/minamoto/:
game_title_logo.png (tile sheet), game_title_logo.bin (32x20 tilemap) and
game_title_logo.pal (208 colours: BG palettes 0-12).

How it is drawn (default, --method png):
- src/logo.png is the logo itself, an indexed PNG to edit by hand (Aseprite:
  Indexed mode). Index 0 is transparent; up to 207 more colours, which become BG
  palettes 0-12. An RGB/RGBA PNG works too: each distinct opaque colour gets an
  index. The build cuts it into 8x8 tiles; the logo can use at most 256 unique
  tiles (BG0's 16 KB char block). Its top-left corner sits at ORIGIN on screen.
- --export-png writes src/logo.png from the text master below (this overwrites
  edits made in the PNG).

The text master (--method master), kept as the source logo.png was made from:
- src/logo_master.txt is the hand-finished pixel master, one character per pixel:
  . outside, B banner, W white panel, L katakana, K 源 pink. Edit it by hand.
- --init-master regenerates it from the HD art (this loses hand edits): each
  katakana piece is downscaled on its own, pieces never touch, every letter gets a
  1 px white line, the banner rim is at least 2 px, and 源 is the hand-drawn
  EMBLEM_ART on a white badge. The ポ dot ring and edge notches were then fixed by
  hand in the master.
- Rendering: letters run light to dark top to bottom like the HD logo, the banner
  a step darker so the katakana separate from it; dark violet edge, 2 px along the
  bottom; the bold maroon subtitle is drawn under it.
Older methods are kept for comparison: --method box (colour snap of an area
downscale) and --method parts (part classification with weighted votes).
"""
import argparse
import os
import struct

from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "tools", "intro_devamp", "src", "logo_minamoto_small.png")
DST = os.path.join(ROOT, "graphics", "title_screen", "minamoto")

# The 源 emblem's box in the source artwork (pink block, white rim and lines).
EMBLEM_BOX = (572, 46, 664, 138)
# 源 drawn by hand for the master: 2 px strokes where they fit, 1 px gaps. It is
# slanted like the HD emblem and set on a white badge (its 1 px rim plus every
# gap), so it reads as pink cut by white lines.
EMBLEM_ART = """
.......PPPPPPPPPPPPPPPPP
.......PPPPPPPPPPPPPPPPP
.PP....PP...............
.PP....PP.....PP........
..PP...PP.....PP........
.......PP..PPPPPPPPPPP..
.......PP..PPPPPPPPPPP..
.......PP..PP.......PP..
PP.....PP..PP.......PP..
PP.....PP..PPPPPPPPPPP..
.PP....PP..PP.......PP..
.......PP..PP.......PP..
.......PP..PPPPPPPPPPP..
.......PP..PPPPPPPPPPP..
.......PP......PP.......
...PP..PP......PP.......
...PP.PP...PP..PP..PP...
..PP..PP...PP..PP..PP...
..PP..PP..PP...PP...PP..
.PP..PP...PP...PP...PP..
.PP..PP........PP....PP.
PP..PP.........PP.......
PP..PP.......PPPP.......
....PP.......PP.........
""".split()
EMBLEM_SLANT = 0.15     # columns of shift per row, top leaning right

WIDTH = 224           # logo width in pixels
ORIGIN = (8, 14)      # top-left corner on screen (centred under the top band)
TEXT_TOP = 72         # first row of "POCKET MONSTERS" block (relative to the logo)

# Colours, on the GBA 5-bit grid. Index 0 is transparent.
COLOURS = {
    "outline": (40, 8, 80),      # the HD logo's edge is its own darkest violet, not brown
    "text":    (80, 24, 24),     # POCKET MONSTERS: Omar's maroon
    "white":   (248, 248, 240),
    "purple1": (136, 32, 192),
    "purple2": (104, 16, 160),
    "purple3": (72, 8, 128),
    "letter":  (168, 56, 216),    # lightest purple
    "letterdk": (88, 16, 120),    # katakana rim against the white field
    "pink":    (240, 8, 184),
    "pinkdk":  (64, 0, 72),
}
ORDER = ["outline", "text", "white", "letter", "letterdk", "purple1", "purple2", "purple3", "pink", "pinkdk"]
INDEX = {name: i + 1 for i, name in enumerate(ORDER)}

# Weight of each part when voting for an output pixel.
STYLE = {"letter_share": 0.3, "white_share": 0.45, "letter_rim": False, "emblem_scale": 1.0, "separate": False,
         "method": "png", "m_white": 190, "m_letter": 128, "white_l": 0.75,
         "banner_ramp": ("letter", "purple1")}

FONT = {  # 5x5, '#' = ink
    "P": ["####.", "#...#", "####.", "#....", "#...."],
    "O": [".###.", "#...#", "#...#", "#...#", ".###."],
    "C": [".####", "#....", "#....", "#....", ".####"],
    "K": ["#...#", "#..#.", "###..", "#..#.", "#...#"],
    "E": ["#####", "#....", "####.", "#....", "#####"],
    "T": ["#####", "..#..", "..#..", "..#..", "..#.."],
    "M": ["#...#", "##.##", "#.#.#", "#...#", "#...#"],
    "N": ["#...#", "##..#", "#.#.#", "#..##", "#...#"],
    "S": [".####", "#....", ".###.", "....#", "####."],
    "R": ["####.", "#...#", "####.", "#..#.", "#...#"],
    " ": [".....", ".....", ".....", ".....", "....."],
}


def emblem(sp, box, w):
    """The 源 emblem as rows of P (pink), W (white) and . (banner), w cells wide."""
    x0, y0, x1, y1 = box
    k = (x1 - x0) / w
    rows = []
    for oy in range(int((y1 - y0) / k)):
        row = ""
        for ox in range(w):
            n = {"P": 0, "W": 0, ".": 0}
            for yy in range(int(y0 + oy * k), int(y0 + (oy + 1) * k)):
                for xx in range(int(x0 + ox * k), int(x0 + (ox + 1) * k)):
                    c = classify(sp[xx, yy])[0]
                    n["P" if c == "pink" else "W" if c == "white" else "."] += 1
            t = sum(n.values())
            row += "P" if n["P"] / t >= 0.45 else "W" if n["W"] / t >= 0.25 else "."
        rows.append(row)
    # white that touches no pink belongs to the banner's own stripes, not the emblem
    at = lambda x, y: rows[y][x] if 0 <= y < len(rows) and 0 <= x < w else "."
    return [[c if c != "W" or any(at(x + dx, y + dy) == "P" for dx in (-1, 0, 1) for dy in (-1, 0, 1)) else "."
             for x, c in enumerate(r)] for y, r in enumerate(rows)]


def classify(px):
    r, g, b, a = px
    if a < 128:
        return "t", 0
    lum = 0.299 * r + 0.587 * g + 0.114 * b
    if min(r, g, b) > STYLE.get("white_min", 200):
        return "white", lum
    if r > 190 and b > 140 and g < 120:
        return "pink", lum
    if r < 90 and b < 70:
        return "outline", lum
    return "purple", lum


# Box method: average the artwork's colour over each output pixel (what a careful
# hand downscale starts from), then snap it to the kit, with a checker only where
# a colour sits halfway between two purples. Letters and banner share one top-to-bottom gradient, as in
# the HD logo; the white lines survive because white wins over any light average.
BOX_KIT = ["white", "letter", "purple1", "purple2", "purple3", "outline"]


def draw_logo_box():
    from retro import oklab
    src = Image.open(SRC).convert("RGBA")
    src_box = src.getbbox()
    src = src.crop(src_box)
    sw, sh = src.size
    s = WIDTH / sw
    h = round(sh * s)
    text_top = round(TEXT_TOP * WIDTH / 200)
    # the HD art has a brushed noise texture: a median filter keeps the edges and
    # leaves the smooth gradient, so the snap gives clean bands
    from PIL import ImageFilter
    sm = src.filter(ImageFilter.MedianFilter(7)).resize((WIDTH, h), Image.BOX)
    p = sm.load()
    kit = {n: oklab(COLOURS[n]) for n in BOX_KIT}
    # Purples follow the HD logo's top-to-bottom gradient as clean bands: four
    # tones with one checker row pair between each.
    rows = [y for y in range(min(h, text_top)) for x in range(WIDTH) if p[x, y][3] >= 140]
    top, bottom = min(rows), max(rows)
    tones = ["letter", "purple1", "purple2", "purple3"]

    def purple_at(x, y):
        v = (y - top) / max(1, bottom - top) * 4 - 0.5
        i = max(0, min(3, int(v)))
        if 0 <= v and i < 3 and v - i > 0.8 and (x + y) & 1:
            i += 1
        return tones[i]
    grid = [["t"] * WIDTH for _ in range(h)]
    out = [[0] * WIDTH for _ in range(h)]
    for y in range(min(h, text_top)):
        for x in range(WIDTH):
            r, g, b, a = p[x, y]
            if a < 140:
                continue
            c = (r * 255 // a, g * 255 // a, b * 255 // a) if a < 255 else (r, g, b)
            lab = oklab(tuple(min(255, v) for v in c))
            if lab[0] > STYLE["white_l"]:
                name = "white"
            else:
                d = sorted(kit, key=lambda n: sum((u - v) ** 2 for u, v in zip(kit[n], lab)))
                name = d[0]
                if name != "outline" or lab[0] > 0.3:
                    name = purple_at(x, y)
            grid[y][x] = "pink" if name == "pink" else "white" if name == "white" else "in"
            out[y][x] = INDEX[name]
    # 1 px dark edge around the silhouette, 2 px along the bottom like the HD logo
    at = lambda x, y: grid[y][x] if 0 <= x < WIDTH and 0 <= y < h else "t"
    for y in range(h):
        for x in range(WIDTH):
            if grid[y][x] != "t" and any(at(x + dx, y + dy) == "t" for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))):
                out[y][x] = INDEX["outline"]
    for y in range(h - 1, 0, -1):
        for x in range(WIDTH):
            if out[y][x] == 0 and out[y - 1][x] == INDEX["outline"] and y < text_top:
                out[y][x] = INDEX["outline"]
                grid[y][x] = "in"
    return finish(out, grid, src.load(), src_box, s, h)


def draw_logo():
    src = Image.open(SRC).convert("RGBA")
    src_box = src.getbbox()
    src = src.crop(src_box)
    sw, sh = src.size
    s = WIDTH / sw
    h = round(sh * s)
    sp = src.load()
    cls = [[classify(sp[x, y]) for x in range(sw)] for y in range(sh)]
    # Purple brightness range, for the gradient.
    lums = sorted(cls[y][x][1] for y in range(0, sh, 2) for x in range(0, sw, 2)
                  if cls[y][x][0] == "purple")
    lo, hi = lums[len(lums) // 20], lums[-len(lums) // 20]
    # The katakana are purple strokes inside a white line, on a purple banner. Purple
    # that can't reach the banner's dark edge without crossing white is letter fill.
    reach = [[False] * sw for _ in range(sh)]
    stack = [(x, y) for y in range(sh) for x in range(sw) if cls[y][x][0] in ("outline", "t")]
    for x, y in stack:
        reach[y][x] = True
    while stack:
        x, y = stack.pop()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < sw and 0 <= ny < sh and not reach[ny][nx] and cls[ny][nx][0] == "purple":
                reach[ny][nx] = True
                stack.append((nx, ny))
    for y in range(sh):
        for x in range(sw):
            if cls[y][x][0] == "purple" and not reach[y][x]:
                cls[y][x] = ("letter", cls[y][x][1])

    # Area coverage of each part under every output pixel.
    text_src_top = round(TEXT_TOP * sw / 200)   # the subtitle is redrawn below
    parts = ("t", "outline", "white", "purple", "letter", "pink")
    cov = {p: [[0.0] * WIDTH for _ in range(h)] for p in parts}
    shade = [[0.5] * WIDTH for _ in range(h)]
    for y in range(h):
        for x in range(WIDTH):
            x0, x1 = int(x / s), max(int(x / s) + 1, int((x + 1) / s))
            y0, y1 = int(y / s), max(int(y / s) + 1, int((y + 1) / s))
            n, plum, pn = 0, 0.0, 0
            for yy in range(y0, min(y1, sh)):
                for xx in range(x0, min(x1, sw)):
                    c, l = cls[yy][xx]
                    if yy >= text_src_top:
                        c = "t"
                    cov[c][y][x] += 1
                    n += 1
                    if c == "purple":
                        plum += l
                        pn += 1
            for p in parts:
                cov[p][y][x] /= max(n, 1)
            if pn:
                shade[y][x] = (plum / pn - lo) / (hi - lo)

    # Pick a part per pixel. Letters win at a lower share than the rest, so the
    # katakana keep their stroke width when the white field around them is thin.
    grid = [["t"] * WIDTH for _ in range(h)]
    for y in range(h):
        for x in range(WIDTH):
            c = {p: cov[p][y][x] for p in parts}
            if c["t"] > 0.5:
                g = "t"
            elif c["pink"] > 0.3:
                g = "pink"
            elif c["letter"] >= STYLE["letter_share"]:
                g = "letter"
            elif c["white"] >= STYLE["white_share"]:
                g = "white"
            elif c["outline"] >= 0.4:
                g = "outline"
            else:
                g = "purple"
            grid[y][x] = g
    N8 = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy]
    N4 = ((1, 0), (-1, 0), (0, 1), (0, -1))
    at = lambda x, y: grid[y][x] if 0 <= x < WIDTH and 0 <= y < h else "t"
    # Lone letter pixels are noise; a letter never touches the banner directly:
    # the white field between them is at least 1 px everywhere.
    for y in range(h):
        for x in range(WIDTH):
            if grid[y][x] == "letter" and sum(at(x + dx, y + dy) == "letter" for dx, dy in N4) <= 1:
                grid[y][x] = "white"
    for y in range(h):
        for x in range(WIDTH):
            if grid[y][x] == "letter" and STYLE["separate"]:
                for dx, dy in N8:
                    if at(x + dx, y + dy) in ("purple", "outline") and 0 <= y + dy < h and 0 <= x + dx < WIDTH:
                        if not any(at(x + dx + ex, y + dy + ey) == "t" for ex, ey in N4):
                            grid[y + dy][x + dx] = "white"

    # Top-to-bottom gradient, as in the HD logo: light tone, a checker step, dark tone.
    top = min((y for y in range(h) for x in range(WIDTH) if grid[y][x] != "t"), default=0)
    bottom = round(text_src_top * s) - 6

    def ramp(tones, x, y):
        v = (y - top) / max(1, bottom - top)
        step = 0 if v < 0.38 else 1 if v < 0.58 else 2
        return INDEX[[tones[0], tones[(x + y) & 1], tones[1]][step]]

    out = [[0] * WIDTH for _ in range(h)]
    for y in range(h):
        for x in range(WIDTH):
            c = grid[y][x]
            if c == "t":
                continue
            # 1 px tinted outline around the whole silhouette
            edge = any(at(x + dx, y + dy) == "t" for dx, dy in N4)
            if edge or c == "outline":
                out[y][x] = INDEX["outline"]
            elif c == "white":
                out[y][x] = INDEX["white"]
            elif c == "pink":
                out[y][x] = INDEX["pink"]
            elif c == "letter":
                rim = STYLE["letter_rim"] and any(at(x + dx, y + dy) == "white" for dx, dy in N4)
                out[y][x] = INDEX["letterdk"] if rim else ramp(("letter", "purple1"), x, y)
            else:
                out[y][x] = ramp(STYLE["banner_ramp"], x, y)
    # The HD logo sits on a heavy dark edge along its bottom: thicken the outline
    # downward by one pixel.
    for y in range(h - 1, 0, -1):
        for x in range(WIDTH):
            if out[y][x] == 0 and out[y - 1][x] == INDEX["outline"] and grid[y - 1][x] != "t":
                out[y][x] = INDEX["outline"]
    return finish(out, grid, sp, src_box, s, h)


MASTER = os.path.join(ROOT, "tools", "intro_devamp", "src", "logo_master.txt")
LOGO_PNG = os.path.join(ROOT, "tools", "intro_devamp", "src", "logo.png")
MAX_TILES = 256
PNG_PAL = None   # palette read from logo.png, used by build()


def draw_logo_png():
    """Read src/logo.png as index rows; fills PNG_PAL."""
    global PNG_PAL, WIDTH
    im = Image.open(LOGO_PNG)
    WIDTH = im.width
    if im.mode == "P":   # index 0 is always the transparent one
        pl = im.getpalette()
        PNG_PAL = [tuple(pl[i:i + 3]) for i in range(0, min(len(pl), 208 * 3), 3)]
        data = list(im.tobytes())
        if max(data) >= 208:
            raise SystemExit("logo.png uses palette index %d; the logo has 208 slots (0-207)" % max(data))
    else:
        im = im.convert("RGBA")
        PNG_PAL = [(0, 0, 0)]
        index = {}
        data = []
        for r, g, b, a in im.getdata():
            if a < 128:
                data.append(0)
                continue
            if (r, g, b) not in index:
                index[(r, g, b)] = len(PNG_PAL)
                PNG_PAL.append((r, g, b))
            data.append(index[(r, g, b)])
        if len(PNG_PAL) > 208:
            raise SystemExit("logo.png has %d colours; the logo has room for 207" % (len(PNG_PAL) - 1))
    w = im.width
    if ORIGIN[0] + w > 256 or ORIGIN[1] + im.height > 160:
        raise SystemExit("logo.png is %dx%d; at %s it must fit in 252x156" % (w, im.height, ORIGIN))
    return [data[y * w:(y + 1) * w] for y in range(im.height)]


def export_png():
    """Render the text master into src/logo.png (indexed, index 0 transparent)."""
    logo = draw_logo_master()
    im = Image.new("P", (WIDTH, len(logo)))
    pal = [(0, 0, 0)] + [COLOURS[n] for n in ORDER]
    im.putpalette([v for c in pal for v in c])
    im.putdata([v for row in logo for v in row])
    im.save(LOGO_PNG, transparency=0)
    print("wrote", LOGO_PNG)
# Master grid letters: . outside, B banner, W white panel, L katakana, K 源 pink,
# O dark edge (forced), anything else is a typo.


def source_masks():
    """Letter / panel / silhouette / pink masks of the HD art at source resolution."""
    src = Image.open(SRC).convert("RGBA")
    box = src.getbbox()
    src = src.crop(box)
    sw, sh = src.size
    sp = src.load()
    cls = [[classify(sp[x, y])[0] for x in range(sw)] for y in range(sh)]
    reach = [[False] * sw for _ in range(sh)]
    stack = [(x, y) for y in range(sh) for x in range(sw) if cls[y][x] in ("outline", "t")]
    for x, y in stack:
        reach[y][x] = True
    while stack:
        x, y = stack.pop()
        for nx, ny in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if 0 <= nx < sw and 0 <= ny < sh and not reach[ny][nx] and cls[ny][nx] == "purple":
                reach[ny][nx] = True
                stack.append((nx, ny))
    masks = {k: Image.new("L", (sw, sh)) for k in "LPSK"}
    px = {k: m.load() for k, m in masks.items()}
    for y in range(sh):
        for x in range(sw):
            c = cls[y][x]
            letter = c == "purple" and not reach[y][x]
            if letter:
                px["L"][x, y] = 255
            if letter or c == "white":
                px["P"][x, y] = 255
            if c != "t":
                px["S"][x, y] = 255
            if c == "pink":
                px["K"][x, y] = 255
    return masks, box, sp


def init_master():
    """First pass of the hand-finished master. Each katakana piece is downscaled on
    its own (area coverage >= 1/2), pieces that would touch are split by white,
    every letter gets exactly a 1 px white line, and only the HD art's wide white
    fields stay wider. The banner rim is at least 2 px. Hand edits go on top, in
    the text file."""
    masks, box, sp = source_masks()
    sw, sh = masks["S"].size
    s = WIDTH / sw
    h = round(round(TEXT_TOP * sw / 200) * s) - 2
    H = round(sh * s)
    small = {k: m.resize((WIDTH, H), Image.BOX).load() for k, m in masks.items()}
    # letter pieces at source resolution
    L = masks["L"].load()
    comp = {}
    n = 0
    for y in range(sh):
        for x in range(sw):
            if L[x, y] and (x, y) not in comp:
                n += 1
                stack = [(x, y)]
                comp[(x, y)] = n
                while stack:
                    cx, cy = stack.pop()
                    for nx, ny in ((cx + 1, cy), (cx - 1, cy), (cx, cy + 1), (cx, cy - 1)):
                        if 0 <= nx < sw and 0 <= ny < sh and L[nx, ny] and (nx, ny) not in comp:
                            comp[(nx, ny)] = n
                            stack.append((nx, ny))
    pieces = {}
    for (x, y), c in comp.items():
        pieces.setdefault(c, []).append((x, y))
    owner = [[0] * WIDTH for _ in range(h)]
    best = [[0.0] * WIDTH for _ in range(h)]
    for c, pts in pieces.items():
        if len(pts) < 30:
            continue
        m = Image.new("L", (sw, sh))
        mp = m.load()
        for x, y in pts:
            mp[x, y] = 255
        cov = m.resize((WIDTH, H), Image.BOX).load()
        for y in range(h):
            for x in range(WIDTH):
                v = cov[x, y] / 255
                if v >= STYLE["m_letter"] / 255 and v > best[y][x]:
                    best[y][x], owner[y][x] = v, c
    N8 = [(dx, dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1) if dx or dy]
    # pieces never touch: the weaker pixel of a touching pair turns white
    for y in range(h):
        for x in range(WIDTH):
            if owner[y][x]:
                for dx, dy in N8:
                    nx, ny = x + dx, y + dy
                    if 0 <= nx < WIDTH and 0 <= ny < h and owner[ny][nx] and owner[ny][nx] != owner[y][x]:
                        if best[y][x] <= best[ny][nx]:
                            owner[y][x] = -1
                            break
    g = [["." for _ in range(WIDTH)] for _ in range(h)]
    for y in range(h):
        for x in range(WIDTH):
            if small["S"][x, y] >= 128:
                g[y][x] = "B"
                if owner[y][x] > 0:
                    g[y][x] = "L"
                elif owner[y][x] < 0 or small["P"][x, y] - small["L"][x, y] >= STYLE["m_white"]:
                    g[y][x] = "W"
                if small["K"][x, y] >= 128:
                    g[y][x] = "K"
    at = lambda x, y: g[y][x] if 0 <= x < WIDTH and 0 <= y < h else "."
    for y in range(h):
        for x in range(WIDTH):
            if g[y][x] == "B" and any(at(x + dx, y + dy) == "L" for dx, dy in N8):
                g[y][x] = "W"
    for _ in range(2):
        snap = [r[:] for r in g]
        for y in range(h):
            for x in range(WIDTH):
                if snap[y][x] in "WL" and any(
                        not (0 <= x + dx < WIDTH and 0 <= y + dy < h) or snap[y + dy][x + dx] == "."
                        for dx, dy in N8):
                    g[y][x] = "B"
    # the 源 emblem: clear the scaled one, then set the hand-drawn glyph on its badge
    for y in range(h):
        for x in range(WIDTH):
            if g[y][x] == "K":
                g[y][x] = "B"
    ex0, ey0, ex1, ey1 = EMBLEM_BOX
    cx, cy = ((ex0 + ex1) / 2 - box[0]) * s, ((ey0 + ey1) / 2 - box[1]) * s
    gh = len(EMBLEM_ART)
    art = {}
    for y, row in enumerate(EMBLEM_ART):
        shift = round((gh - 1 - y) * EMBLEM_SLANT)
        for x, c in enumerate(row):
            if c == "P":
                art[(x + shift, y)] = True
    gw = max(x for x, _ in art) + 1
    ox, oy = round(cx - gw / 2), round(cy - gh / 2)
    badge = {(x + dx, y + dy) for (x, y) in art for dx in (-1, 0, 1) for dy in (-1, 0, 1)}
    # fill the badge's holes
    xs, ys = [p[0] for p in badge], [p[1] for p in badge]
    outside, stack = set(), [(min(xs) - 1, min(ys) - 1)]
    while stack:
        p = stack.pop()
        if p in outside or p in badge or not (min(xs) - 1 <= p[0] <= max(xs) + 1 and min(ys) - 1 <= p[1] <= max(ys) + 1):
            continue
        outside.add(p)
        stack += [(p[0] + 1, p[1]), (p[0] - 1, p[1]), (p[0], p[1] + 1), (p[0], p[1] - 1)]
    for y in range(min(ys), max(ys) + 1):
        for x in range(min(xs), max(xs) + 1):
            if (x, y) in outside:
                continue
            X, Y = ox + x, oy + y
            if 0 <= X < WIDTH and 0 <= Y < h and g[Y][X] != ".":
                g[Y][X] = "K" if (x, y) in art else "W"
    with open(MASTER, "w") as f:
        for y, row in enumerate(g):
            f.write("%02d|%s|\n" % (y, "".join(row)))
    print("master %dx%d written" % (WIDTH, h))


def draw_logo_master():
    rows = [line.split("|")[1] for line in open(MASTER).read().splitlines() if "|" in line]
    h0 = len(rows)
    text_top = round(TEXT_TOP * WIDTH / 200)
    h = max(h0, text_top)
    g = [list(r) for r in rows] + [["."] * WIDTH for _ in range(h - h0)]
    at = lambda x, y: g[y][x] if 0 <= x < WIDTH and 0 <= y < h else "."
    ys = [y for y in range(h0) if any(c != "." for c in g[y])]
    top, bottom = min(ys), max(ys)
    # Letters and banner both run light to dark, top to bottom, as in the HD logo,
    # but the banner is a step darker so the katakana separate from it even
    # where the white line is 1 px.

    def purple_at(x, y, letter):
        tones = ("letter", "purple1") if letter else ("purple2", "purple3")
        v = (y - top) / max(1, bottom - top)
        if v < 0.6:
            return INDEX[tones[0]]
        if v < 0.67:
            return INDEX[tones[(x + y) & 1]]
        return INDEX[tones[1]]
    out = [[0] * WIDTH for _ in range(h)]
    for y in range(h):
        for x in range(WIDTH):
            c = g[y][x]
            if c == ".":
                continue
            edge = any(at(x + dx, y + dy) == "." for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if edge or c == "O":
                out[y][x] = INDEX["outline"]
            elif c == "W":
                out[y][x] = INDEX["white"]
            elif c == "K":
                out[y][x] = INDEX["pink"]
            else:
                out[y][x] = purple_at(x, y, c == "L")
    for y in range(h - 1, 0, -1):   # 2 px dark edge along the bottom, as in the HD logo
        for x in range(WIDTH):
            if out[y][x] == 0 and out[y - 1][x] == INDEX["outline"] and g[y - 1][x] != "." and y < text_top:
                out[y][x] = INDEX["outline"]
    return subtitle(out)


def finish(out, grid, sp, src_box, s, h):
    """Paste the 源 emblem and draw the subtitle; grid marks "t" outside the logo."""
    # 源 emblem: sampled on its own grid, aligned to the emblem's box, so its thin
    # white lines land on whole pixels, and drawn 1.3x bigger than the rest so the
    # kanji still reads; then pasted over the scaled one, inside the silhouette.
    ex0, ey0, ex1, ey1 = EMBLEM_BOX
    bx, by = src_box[0], src_box[1]
    ew = round((ex1 - ex0) * s * STYLE["emblem_scale"])
    ox = round(((ex0 + ex1) / 2 - bx) * s - ew / 2)
    oy = round(((ey0 + ey1) / 2 - by) * s - ew / 2)
    cells = emblem(sp, (ex0 - bx, ey0 - by, ex1 - bx, ey1 - by), ew)
    for y in range(max(0, oy - 2), min(h, oy + len(cells) + 2)):   # clear the scaled kanji
        for x in range(max(0, ox - 2), min(WIDTH, ox + len(cells[0]) + 2)):
            if grid[y][x] in ("pink", "white") or out[y][x] in (INDEX["pink"], INDEX["pinkdk"]):
                out[y][x] = INDEX["purple3"]
    for cy, row in enumerate(cells):
        for cx, k in enumerate(row):
            x, y = ox + cx, oy + cy
            if k != "." and 0 <= x < WIDTH and 0 <= y < h and grid[y][x] != "t" \
                    and out[y][x] != INDEX["outline"]:
                out[y][x] = INDEX["pink" if k == "P" else "white"]

    return subtitle(out)


def subtitle(out):
    # "POCKET MONSTERS" between two rules, bold like the HD subtitle: every stem
    # is doubled to the right.
    text = "POCKET MONSTERS"
    adv = lambda ch: 7 if ch != " " else 4
    tw = sum(adv(ch) for ch in text) - 1
    # left of centre, so it clears Venusaur's leaves
    cx = round(0.34 * WIDTH)
    tx0 = cx - tw // 2
    ty0 = round(TEXT_TOP * WIDTH / 200) + 3
    while len(out) < ty0 + 9:
        out.append([0] * WIDTH)
    for x in range(tx0 - 3, tx0 + tw + 3):
        out[ty0 - 2][x] = INDEX["text"]
        out[ty0 + 6][x] = INDEX["text"]
        out[ty0 + 7][x] = INDEX["text"]
    x = tx0
    for ch in text:
        for gy, row in enumerate(FONT[ch]):
            for gx, p in enumerate(row):
                if p == "#":
                    out[ty0 + gy][x + gx] = INDEX["text"]
                    out[ty0 + gy][x + gx + 1] = INDEX["text"]
        x += adv(ch)
    return out


def build(out_dir):
    logo = {"box": draw_logo_box, "parts": draw_logo, "master": draw_logo_master,
            "png": draw_logo_png}[STYLE["method"]]()
    lh = len(logo)
    W, H = 256, 160
    screen = [[0] * W for _ in range(H)]
    ox, oy = ORIGIN
    for y in range(lh):
        for x in range(WIDTH):
            screen[oy + y][ox + x] = logo[y][x]
    # Cut into 8x8 tiles, deduplicated; tile 0 is blank.
    tiles = [tuple([0] * 64)]
    lookup = {tiles[0]: 0}
    tmap = []
    for ty in range(H // 8):
        for tx in range(W // 8):
            t = tuple(screen[ty * 8 + y][tx * 8 + x] for y in range(8) for x in range(8))
            if t not in lookup:
                lookup[t] = len(tiles)
                tiles.append(t)
            tmap.append(lookup[t])
    if len(tiles) > MAX_TILES:
        raise SystemExit("logo needs %d unique 8x8 tiles; BG0 has room for %d" % (len(tiles), MAX_TILES))
    cols = 16
    rows = (len(tiles) + cols - 1) // cols
    sheet = Image.new("P", (cols * 8, rows * 8))
    pal = list(PNG_PAL) if STYLE["method"] == "png" else [(0, 0, 0)] + [COLOURS[n] for n in ORDER]
    pal += [(0, 0, 0)] * (256 - len(pal))
    sheet.putpalette([v for c in pal for v in c])
    px = sheet.load()
    for i, t in enumerate(tiles):
        bx, by = i % cols * 8, i // cols * 8
        for k, v in enumerate(t):
            px[bx + k % 8, by + k // 8] = v
    sheet.save(os.path.join(out_dir, "game_title_logo.png"))
    with open(os.path.join(out_dir, "game_title_logo.bin"), "wb") as f:
        f.write(struct.pack("<%dH" % len(tmap), *tmap))
    with open(os.path.join(out_dir, "game_title_logo.pal"), "w", newline="\r\n") as f:
        f.write("JASC-PAL\n0100\n208\n")
        for c in pal[:208]:
            f.write("%d %d %d\n" % c)
    print(f"logo {WIDTH}x{lh}, {len(tiles)} tiles")
    return screen, pal


def main():
    global WIDTH
    ap = argparse.ArgumentParser()
    ap.add_argument("--preview")
    ap.add_argument("--width", type=int, default=WIDTH)
    ap.add_argument("--share", type=float, default=STYLE["letter_share"])
    ap.add_argument("--no-rim", action="store_true")
    ap.add_argument("--dark-banner", action="store_true", help="banner darker than the letters")
    ap.add_argument("--white", type=float, default=STYLE["white_share"])
    ap.add_argument("--method", choices=["png", "master", "box", "parts"], default=STYLE["method"])
    ap.add_argument("--export-png", action="store_true", help="write src/logo.png from the text master (overwrites it)")
    ap.add_argument("--init-master", action="store_true", help="regenerate src/logo_master.txt (loses hand edits)")
    ap.add_argument("--white-l", type=float, default=STYLE["white_l"])
    ap.add_argument("--emblem", type=float, default=STYLE["emblem_scale"], help="源 emblem size vs. the rest")
    ap.add_argument("--out", default=DST)
    args = ap.parse_args()
    WIDTH = args.width
    STYLE.update(letter_share=args.share, letter_rim=not args.no_rim, emblem_scale=args.emblem, white_share=args.white,
                 method=args.method, white_l=args.white_l)
    if args.dark_banner:
        STYLE["banner_ramp"] = ("purple2", "purple3")
    if args.init_master:
        init_master()
        return
    if args.export_png:
        export_png()
        return
    screen, pal = build(args.out)
    if args.preview:
        im = Image.new("RGB", (240, 160), (232, 128, 96))
        p = im.load()
        for y in range(160):
            for x in range(240):
                if screen[y][x]:
                    p[x, y] = pal[screen[y][x]]
        im.resize((960, 640), Image.NEAREST).save(args.preview)


if __name__ == "__main__":
    main()
