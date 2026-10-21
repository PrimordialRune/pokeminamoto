#!/usr/bin/env python3
"""Draws the Battle UI v0.1 healthboxes: navy-outlined white boxes with a speech tail,
HP/TP bars in navy slots, the exp bar in the box's bottom band and "(PSN)"-style status tags.

The engine writes text, status tags, the exp bar and the HP/TP bars into fixed tiles of these
sprites, so the art keeps Emerald's geometry: name/level rows y5-15 are fill, the exp bar tiles
are x32-95 y32-39 (player), the status tag is x16-39 y24-31 (player) and x8-31 y16-23 (foe).

Outputs (all in graphics/battle_interface/):
  healthbox_singles_player.png, healthbox_singles_opponent.png  box art
  ball_status_bar.png    its palette is the healthbox palette (TAG_HEALTHBOX_PAL)
  ball_display.png       its palette is the bar palette (TAG_HEALTHBAR_PAL)
  hpbar.png, hpbar_anim.png, tpbar.png, misc_frameend.png       bar tiles
  expbar.png, status.png, status2-4.png
Usage: gen_healthbox.py <repo root>
"""
import os, sys
from PIL import Image

ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'
OUT = os.path.join(ROOT, 'graphics/battle_interface')

NAVY = (39, 31, 59)
WHITE = (255, 255, 255)

# Healthbox palette. 1/3 are the text colours (AddTextPrinterAndCreateWindowOnHealthbox),
# 2 the box fill, 12-15 are set at runtime to each battler's status colour.
BOX_PAL = [
    (0, 0, 0), NAVY, WHITE, (200, 196, 214), (226, 224, 236), NAVY, NAVY, (61, 75, 102),
    (64, 87, 128), (255, 190, 9), (225, 95, 105), (49, 162, 242),
    (0, 0, 248), (0, 248, 0), (248, 0, 0), (104, 144, 248),
]
B_TEXT, B_FILL, B_SHADOW, B_LIGHT, B_EDGE, B_OUT, B_SLATE, B_BLUE, B_EXP = 1, 2, 3, 4, 5, 6, 7, 8, 11

# Bar palette, shared with the party ball icons (ball_display.png uses 2-6, 8, 9).
BAR_PAL = [
    (0, 0, 0), (224, 112, 40), WHITE, (208, 200, 184), (128, 128, 136), (61, 75, 102), NAVY, (255, 170, 80),
    (240, 144, 112), (216, 104, 88), (176, 224, 80), (120, 184, 48), (248, 216, 56), (208, 160, 16),
    (248, 96, 72), (176, 48, 56),
]
R_EMPTY, R_FRAME, R_LABEL = 5, 6, 2
FILLS = {'green': (10, 11), 'yellow': (12, 13), 'red': (14, 15), 'orange': (7, 1)}

GLYPHS = {
    'H': ['1.1', '111', '1.1', '1.1'],
    'P': ['11.', '1.1', '11.', '1..'],
    'T': ['111', '.1.', '.1.', '.1.'],
}


def save(name, px, w, h, pal):
    im = Image.new('P', (w, h))
    im.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    im.putdata([px.get((x, y), 0) for y in range(h) for x in range(w)])
    im.save(os.path.join(OUT, name))


def bar_tile(filled, light, dark):
    t = {}
    for x in range(8):
        t[(x, 1)] = t[(x, 6)] = R_FRAME
        for y in range(2, 6):
            t[(x, y)] = (light if y < 4 else dark) if x < filled else R_EMPTY
    return t


def label_tiles(text):
    t = {}
    for x in range(16):
        for y in range(1, 7):
            if x == 0 or (x == 1 and y in (1, 6)):
                continue
            t[(x, y)] = R_FRAME
    for i, ch in enumerate(text):
        for gy, row in enumerate(GLYPHS[ch]):
            for gx, c in enumerate(row):
                if c == '1':
                    t[(3 + i * 4 + gx, 2 + gy)] = R_LABEL
    return t


def paste(dst, tile, ox):
    for (x, y), c in tile.items():
        dst[(ox + x, y)] = c


def gen_bars():
    # hpbar.png: blank, "H", "P", green bar 0-8 px
    px = {}
    paste(px, label_tiles('HP'), 8)
    for n in range(9):
        paste(px, bar_tile(n, *FILLS['green']), 24 + n * 8)
    save('hpbar.png', px, 96, 8, BAR_PAL)
    # hpbar_anim.png: yellow bar 0-8 px, red bar 0-8 px
    px = {}
    for n in range(9):
        paste(px, bar_tile(n, *FILLS['yellow']), n * 8)
        paste(px, bar_tile(n, *FILLS['red']), 72 + n * 8)
    save('hpbar_anim.png', px, 144, 8, BAR_PAL)
    # tpbar.png: "T", "P", orange bar 0-8 px; the second row is the player's copy, whose
    # see-through pixels are white so it covers the HP numbers under it
    px = {}
    paste(px, label_tiles('TP'), 0)
    for n in range(9):
        paste(px, bar_tile(n, *FILLS['orange']), 16 + n * 8)
    for x in range(88):
        for y in range(8):
            px[(x, y + 8)] = px.get((x, y), 0) or R_LABEL
    save('tpbar.png', px, 88, 16, BAR_PAL)
    # misc_frameend.png: the slot's rounded left end, shown in place of the label next to a status tag
    px = {}
    for y in range(1, 7):
        px[(7, y)] = R_FRAME
        if y not in (1, 6):
            px[(6, y)] = R_FRAME
    save('misc_frameend.png', px, 8, 8, BAR_PAL)
    # the party ball icons keep their art; only the palette changes
    im = Image.open(os.path.join(OUT, 'ball_display.png'))
    save('ball_display.png', {(x, y): im.getpixel((x, y)) for x in range(32) for y in range(8)}, 32, 8, BAR_PAL)


def box(px, x0, y0, x1, y1):
    """Navy 2px outline with clipped corners, white inside."""
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            corner = (x in (x0, x1)) and (y in (y0, y1))
            if corner:
                continue
            edge = x <= x0 + 1 or x >= x1 - 1 or y <= y0 + 1 or y >= y1 - 1
            px[(x, y)] = B_OUT if edge else B_FILL
    for x, y in ((x0 + 2, y0 + 2), (x1 - 2, y0 + 2), (x0 + 2, y1 - 2), (x1 - 2, y1 - 2)):
        px[(x, y)] = B_OUT


def tail(px, xb, ytop, ybot, direction):
    """Speech tail growing out of the box's side edge (outline columns xb and xb - direction),
    from ytop down to the box bottom ybot, pointing out. Its inside joins the box's inside."""
    for i in range(ybot - ytop + 1):
        y = ytop + i
        reach = i * 3 // 4
        for d in range(-2, reach + 1):
            x = xb + direction * d
            if y >= ybot - 1 or d >= reach - 1:
                px[(x, y)] = B_OUT
            elif i > 1:
                px[(x, y)] = B_FILL


def gen_boxes():
    # Foe: box x1-98, y2-31; the HP bar sits at y16-23, the TP bar at y23-30
    px = {}
    box(px, 1, 2, 98, 31)
    tail(px, 98, 20, 31, 1)
    save('healthbox_singles_opponent.png', px, 128, 32, BOX_PAL)

    # Player: box x9-100, y2-34; HP bar y17-24, TP bar y24-31; the exp bar tiles (x32-95, y32-39)
    # put white, navy, navy above the bar so the box's bottom edge runs through them
    px = {}
    box(px, 9, 2, 100, 34)
    tail(px, 9, 21, 34, -1)
    for x in range(10, 100):
        for y in range(35, 38):
            px[(x, y)] = B_OUT
    px[(10, 37)] = px[(99, 37)] = 0
    save('healthbox_singles_player.png', px, 128, 64, BOX_PAL)

    # Party summary bar at battle start: same palette
    im = Image.open(os.path.join(OUT, 'ball_status_bar.png'))
    save('ball_status_bar.png', {(x, y): im.getpixel((x, y)) for x in range(128) for y in range(8)}, 128, 8, BOX_PAL)


def gen_expbar():
    # rows: fill, navy, navy, bar, bar, navy (box bottom band)
    px = {}
    for n in range(9):
        for x in range(8):
            X = n * 8 + x
            px[(X, 0)] = B_FILL
            px[(X, 1)] = px[(X, 2)] = px[(X, 5)] = B_OUT
            px[(X, 3)] = px[(X, 4)] = B_EXP if x < n else B_SLATE
    save('expbar.png', px, 72, 8, BOX_PAL)


def gen_status():
    # Emerald draws white letters on a status-coloured tag; draw status-coloured letters
    # in brackets on the box fill instead.
    for name in ('status.png', 'status2.png', 'status3.png', 'status4.png'):
        im = Image.open(os.path.join(OUT, name))
        w, h = im.size
        dyn = {'status.png': 12, 'status2.png': 13, 'status3.png': 14, 'status4.png': 15}[name]
        px = {}
        for ty in range(h // 8):
            for y in range(8):
                for x in range(w):
                    c = im.getpixel((x, ty * 8 + y)) & 15
                    if x <= 19 and 2 <= x <= 17 and 1 <= y <= 6 and c == B_FILL:
                        c = dyn
                    elif c in (dyn, B_SHADOW) or x <= 19:
                        c = B_FILL
                    px[(x, ty * 8 + y)] = c
            for y in range(2, 6):
                px[(0, ty * 8 + y)] = px[(19, ty * 8 + y)] = dyn
            for y in (1, 6):
                px[(1, ty * 8 + y)] = px[(18, ty * 8 + y)] = dyn
        save(name, px, w, h, BOX_PAL)


if __name__ == '__main__':
    gen_bars()
    gen_boxes()
    gen_expbar()
    gen_status()
