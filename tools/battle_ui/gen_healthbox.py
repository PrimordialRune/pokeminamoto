#!/usr/bin/env python3
"""Builds the singles healthboxes from Omar's Battle UI v0.1 mockup (MyRes/UI/UItest2.ase).

The box frames are copied pixel for pixel from the mockup's "Layer 1" (tools/battle_ui/src/boxes_v0.1.png),
with the parts the engine draws at runtime cleared: name, level, status tag, the HP/TP fills, the
exp fill and the party pips. The only reshaping is in the HP/TP slots: the bar sprites are 64 px
long where the mockup's bars are 61, so each label is 3 px narrower.

Canvas = 128x64 (two 64x64 sprites). Mockup to canvas offsets: player (-128, -71), foe (-2, -4).
Where the engine writes (see src/battle_interface.c):
  name              rows 5-15, player x16-71, foe x8-71
  status tag        tile row 2, x24-55 (4 tiles)
  level             tile row 2, player x72-95, foe x64-87
  HP numbers        tile row 2, player x56-95 (START)
  HP / TP fills     sprites, fill rows 26-29 / 33-36, player x27-90, foe x23-86
  exp fill          tile row 5, player x16-79
  party pips        sprite over the tray, player x102-109, foe x0-7

Outputs (graphics/battle_interface/):
  healthbox_singles_player.png, healthbox_singles_opponent.png   box art
  ball_status_bar.png   its palette is the healthbox palette (TAG_HEALTHBOX_PAL)
  ball_display.png      its palette is the bar/pip palette (TAG_HEALTHBAR_PAL)
  hpbar.png, hpbar_anim.png, tpbar.png, misc_frameend.png, ball_caught_indicator.png
  expbar.png, status_tags.png
Usage: gen_healthbox.py <repo root>
"""
import os, sys
from PIL import Image

ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'
OUT = os.path.join(ROOT, 'graphics/battle_interface')
SRC = os.path.join(ROOT, 'tools/battle_ui/src/boxes_v0.1.png')

NAVY = (39, 31, 59)
WHITE = (255, 255, 255)
LIGHT_BLUE = (178, 220, 239)
GREY = (97, 97, 97)
CYAN = (49, 162, 242)
DARK_RED = (190, 38, 51)
CORAL = (225, 95, 105)
YELLOW = (255, 190, 9)
ORANGE = (255, 148, 66)
GREEN = (163, 206, 39)

# Healthbox palette. 1/3 are the text colours (AddTextPrinterAndCreateWindowOnHealthbox): navy with
# no shadow, like the mockup. 2 is the fill, 10/11 the gender symbols, 12-15 each battler's status colour.
BOX_PAL = [(0, 0, 0), NAVY, WHITE, WHITE, LIGHT_BLUE, GREY, NAVY, CYAN,
           YELLOW, DARK_RED, CORAL, CYAN, (0, 0, 248), (0, 248, 0), (248, 0, 0), (104, 144, 248)]
B_FILL, B_LIGHT, B_GREY, B_OUT, B_EXP = 2, 4, 5, 6, 7
BOX_INDEX = {NAVY: B_OUT, WHITE: B_FILL, LIGHT_BLUE: B_LIGHT, GREY: B_GREY, CYAN: B_EXP}

# Bar and pip palette (TAG_HEALTHBAR_PAL)
BAR_PAL = [(0, 0, 0), NAVY, WHITE, LIGHT_BLUE, GREY, DARK_RED, CORAL, ORANGE,
           YELLOW, GREEN, (248, 208, 48), (232, 72, 56), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0)]
R_NAVY, R_WHITE, R_EMPTY, R_GREY, R_DARK_RED, R_CORAL = 1, 2, 3, 4, 5, 6
FILLS = {'green': 9, 'yellow': 10, 'red': 11, 'orange': 7}


def save(name, px, w, h, pal):
    im = Image.new('P', (w, h))
    im.putpalette([v for c in pal for v in c] + [0] * (768 - 3 * len(pal)))
    im.putdata([px.get((x, y), 0) for y in range(h) for x in range(w)])
    im.save(os.path.join(OUT, name))


def copy_box(src, ox, oy, y0, y1):
    """Mockup rows y0..y1 (inclusive) of the box at offset (ox, oy) into a canvas dict.
    Each row keeps everything from its leftmost to its rightmost opaque pixel; holes become fill."""
    px = {}
    for my in range(y0, y1 + 1):
        xs = [mx for mx in range(ox, min(ox + 128, src.width)) if src.getpixel((mx, my))[3]]
        if not xs:
            continue
        for mx in range(min(xs), max(xs) + 1):
            c = src.getpixel((mx, my))
            px[(mx - ox, my - oy)] = BOX_INDEX.get(c[:3], B_FILL) if c[3] else B_FILL
    return px


def clear(px, x0, x1, y0, y1, color=B_FILL):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            px[(x, y)] = color


def reshape_slot(px, top, label_x0, label_x1, shift, fill_x0):
    """One HP/TP slot (6 rows from top): the label keeps its right part (shift px of solid navy
    come off its left), then a white column, then a 64 px well the bar sprite draws over."""
    for y in range(top, top + 6):
        row = dict(px)
        for x in range(label_x0, label_x1 + 1):
            px[(x, y)] = row[(x + shift, y)]
        px[(label_x1 + 1, y)] = row[(label_x1 + 1 + shift, y)]
        for x in range(fill_x0, fill_x0 + 64):
            px[(x, y)] = B_OUT if y == top + 5 else B_FILL


def gen_boxes():
    src = Image.open(SRC).convert('RGBA')

    # Player: mockup x128-239, rows 73-116
    px = copy_box(src, 128, 71, 73, 116)
    clear(px, 16, 97, 6, 6)
    clear(px, 15, 98, 7, 24)
    reshape_slot(px, 26, 11, 25, 3, 27)
    reshape_slot(px, 33, 11, 25, 3, 27)
    clear(px, 16, 80, 40, 41, B_LIGHT)
    clear(px, 103, 108, 4, 33, B_OUT)
    save('healthbox_singles_player.png', px, 128, 64, BOX_PAL)

    # Foe: mockup x2-108, rows 6-44
    px = copy_box(src, 2, 4, 6, 44)
    clear(px, 12, 92, 6, 24)
    reshape_slot(px, 26, 9, 21, 3, 23)
    reshape_slot(px, 33, 9, 21, 3, 23)
    clear(px, 1, 6, 4, 33, B_OUT)
    save('healthbox_singles_opponent.png', px, 128, 64, BOX_PAL)

    # Party summary bar at battle start: plain navy in the same palette
    im = Image.open(os.path.join(OUT, 'ball_status_bar.png'))
    save('ball_status_bar.png', {(x, y): B_OUT if im.getpixel((x, y)) else 0 for x in range(128) for y in range(8)},
         128, 8, BOX_PAL)


def bar_tile(filled, color):
    return {(x, y): (color if x < filled else R_EMPTY) for x in range(8) for y in range(1, 5)}


def paste(dst, tile, ox):
    for (x, y), c in tile.items():
        dst[(ox + x, y)] = c


# 4x4 pips, as in the mockup's trays: healthy = Poké Ball, statused = red, fainted = grey ring,
# empty slot = grey dot. 0 lets the tray show through.
PIPS = {
    'healthy': [[0, R_EMPTY, R_WHITE, 0], [R_EMPTY, R_WHITE, R_WHITE, R_WHITE],
                [R_DARK_RED, R_CORAL, R_CORAL, R_CORAL], [0, R_DARK_RED, R_DARK_RED, 0]],
    'status':  [[0, R_DARK_RED, R_DARK_RED, 0], [R_DARK_RED] * 4, [R_CORAL] * 4, [0, R_CORAL, R_CORAL, 0]],
    'fainted': [[0, R_GREY, R_GREY, 0], [R_GREY, 0, 0, R_GREY], [R_GREY, 0, 0, R_GREY], [0, R_GREY, R_GREY, 0]],
    'empty':   [[0, R_GREY, R_GREY, 0], [R_GREY] * 4, [R_GREY] * 4, [0, R_GREY, R_GREY, 0]],
}


def ball_icon(kind):
    """8x8 party ball for the battle-start summary bar: a pip on a navy disc."""
    t = {}
    for y in range(1, 7):
        for x in range(1, 7):
            if (x, y) not in ((1, 1), (6, 1), (1, 6), (6, 6)):
                t[(x, y)] = R_NAVY
    for y, row in enumerate(PIPS[kind]):
        for x, c in enumerate(row):
            if c:
                t[(2 + x, 2 + y)] = c
    return t


def gen_bars():
    # hpbar.png: 3 unused tiles (Emerald's "H"/"P" label, now in the box art), green bar 0-8 px
    px = {}
    for n in range(9):
        paste(px, bar_tile(n, FILLS['green']), 24 + n * 8)
    save('hpbar.png', px, 96, 8, BAR_PAL)
    px = {}
    for n in range(9):
        paste(px, bar_tile(n, FILLS['yellow']), n * 8)
        paste(px, bar_tile(n, FILLS['red']), 72 + n * 8)
    save('hpbar_anim.png', px, 144, 8, BAR_PAL)
    px = {}
    for n in range(9):
        paste(px, bar_tile(n, FILLS['orange']), n * 8)
    save('tpbar.png', px, 72, 8, BAR_PAL)
    # doubles only: the slot's rounded left end
    px = {(7, y): R_NAVY for y in range(1, 5)}
    px.update({(6, y): R_NAVY for y in range(2, 4)})
    save('misc_frameend.png', px, 8, 8, BAR_PAL)
    # party balls: full, empty, fainted, statused
    px = {}
    for i, kind in enumerate(('healthy', 'empty', 'fainted', 'status')):
        paste(px, ball_icon(kind), i * 8)
    save('ball_display.png', px, 32, 8, BAR_PAL)
    save('ball_caught_indicator.png', ball_icon('healthy'), 8, 8, BAR_PAL)
    # pip shapes for the C table (printed, pasted into sPartyPipGfx by hand)
    return PIPS


def gen_expbar():
    # rows: bar, bar, white, navy (inner box bottom), white, navy (outer frame), see-through
    px = {}
    for n in range(9):
        for x in range(8):
            X = n * 8 + x
            px[(X, 0)] = px[(X, 1)] = B_EXP if x < n else B_LIGHT
            px[(X, 2)] = px[(X, 4)] = B_FILL
            px[(X, 3)] = px[(X, 5)] = B_OUT
    save('expbar.png', px, 72, 8, BOX_PAL)


# The mockup's bold 7x7 capitals, for the status tags
GLYPHS = {
    'P': ['######.', '##...##', '##...##', '######.', '##.....', '##.....', '##.....'],
    'S': ['.#####.', '##...##', '##.....', '.#####.', '.....##', '##...##', '.#####.'],
    'N': ['##...##', '###..##', '####.##', '#######', '##.####', '##..###', '##...##'],
    'A': ['.#####.', '##...##', '##...##', '##...##', '#######', '##...##', '##...##'],
    'R': ['######.', '##...##', '##...##', '##...##', '######.', '##...##', '##...##'],
    'L': ['##.....', '##.....', '##.....', '##.....', '##.....', '##.....', '#######'],
    'F': ['#######', '##.....', '##.....', '######.', '##.....', '##.....', '##.....'],
    'Z': ['#######', '....###', '...###.', '..###..', '.###...', '###....', '#######'],
    'B': ['######.', '##...##', '##...##', '######.', '##...##', '##...##', '######.'],
}
BRACKET = ['..#', '.##', '###', '###', '###', '.##', '..#']
STATUSES = ['PSN', 'PAR', 'SLP', 'FRZ', 'BRN']  # STATUS_TAG_* order in battle_interface.c


def gen_status_tags():
    # 32x8 per tag, one block of 5 per battler, each in its own status colour slot (12 + battler)
    px = {}
    for battler in range(4):
        color = 12 + battler
        for s, word in enumerate(STATUSES):
            oy = (battler * 5 + s) * 8
            clear(px, 0, 31, oy, oy + 7)
            for y in range(7):
                for x in range(3):
                    if BRACKET[y][x] == '#':
                        px[(x, oy + 1 + y)] = color
                    if BRACKET[y][2 - x] == '#':
                        px[(28 + x, oy + 1 + y)] = color
                for i, ch in enumerate(word):
                    for x, c in enumerate(GLYPHS[ch][y]):
                        if c == '#':
                            px[(4 + i * 8 + x, oy + 1 + y)] = color
    save('status_tags.png', px, 32, 160, BOX_PAL)


if __name__ == '__main__':
    gen_boxes()
    gen_bars()
    gen_expbar()
    gen_status_tags()
