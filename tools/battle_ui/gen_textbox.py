#!/usr/bin/env python3
"""Builds the battle textbox art (BG0) from Omar's Battle UI v0.1 mockup.

Writes graphics/battle_interface/textbox.png, textbox_0.pal and textbox_map.bin.
The three screens are copied pixel for pixel from the mockup (MyRes/UI/UItest2.ase, exported to
tools/battle_ui/src/: message = frame 5, action = frame 0, move = frame 6), with the parts the
engine draws cleared. Also writes move_grid.png, move_info.png and action_menu.png: the art under
the windows that are redrawn whole (see battle_controller_player.c).

Tile index rules (see src/battle_bg.c, src/egg_hatch.c, src/trade.c):
  0          blank
  1-2        yes/no cursor arrow (Emerald's), recoloured to BG palette 5 (text.pal) so it matches
             the Yes/No text: white 14, arrow 13, shadow 15
  0x12-0x1A  overwritten in battle by the player's window frame
  0x22-0x2A  overwritten in battle by the player's window frame
  < 0x40     everything on the message screen: egg hatch and trade load their
             text window at tile 0x40
  0x7F       navy in BG palette 5, where no effectiveness icon shows
  0x80-0x8F  effectiveness icons in BG palette 5, 2x2 tiles each: neutral, not very effective,
             super effective, no effect
  palette 5  move screen info box tiles, drawn in the move type's color (B_UI_PAL_TYPE)
  0x90-0x10B battle text windows (message, yes/no)
  0x10C-0x18F more art for the action and move screens (the Battle Arena's windows also use
             0x100-0x163, but the Arena is unreachable in this game)
  >= 0x190   battle text windows. Loading this sheet blanks 0x90-0x10B: the windows there are
             always redrawn before they show
Usage: gen_textbox.py <repo root>
"""
import struct, sys, os
from PIL import Image

ROOT = sys.argv[1] if len(sys.argv) > 1 else '.'
GFX = os.path.join(ROOT, 'graphics/battle_interface')

PAL = [
    (0, 0, 0),        # 0  transparent / backdrop
    (0x27, 0x1F, 0x3B),  # 1  navy (outline, LCD text)
    (0xE1, 0x5F, 0x69),  # 2  coral (device body)
    (0xEF, 0xD2, 0xD2),  # 3  pink (bevel)
    (0x40, 0x57, 0x80),  # 4  blue slate (lens)
    (0xFF, 0xFF, 0xFF),  # 5  white
    (0x92, 0xB7, 0x5A),  # 6  LCD dark green (text shadow)
    (0x3D, 0x4B, 0x66),  # 7  slate (move panel)
    (0xB2, 0xDC, 0xEF),  # 8  light blue
    (0xFF, 0x94, 0x42),  # 9  orange
    (0xFF, 0xBE, 0x09),  # 10 yellow
    (0x31, 0xA2, 0xF2),  # 11 cyan
    (0xB8, 0x43, 0x4F),  # 12 coral shadow
    (0x8C, 0x8C, 0xA6),  # 13 grey
    (0xC7, 0xDE, 0x97),  # 14 LCD light green
    (0xA0, 0xC1, 0x6C),  # 15 LCD green (message window fill)
]
T, N, C, P, B, W, G2, S, L, O, Y, CY, CD, GR, GL, G = range(16)

# Pixel art copied from the mockup. Letters: N navy, c coral, W white, o orange, y yellow, . transparent
TALK_BADGE = """\
....NNNNNNNNNNNNNNNNNNNN
....NNNNNNNNNNNNNNNNNNNN
....NNNNNNNWWWWWWNNNNNNN
....NNNNNNNWWNNNWWNNNNNN
....NNNNNNNWWNNNWWNNNNNN
....NNNNNNNWWNNNWWNNNNNN
....NNNNNNNWWWWWWNNNNNNN
....NNNNNNNWWNNNWWNNNNNN
....NNNNNNNWWNNNWWNNNNNN
....NNNNNNNNNNNNNNNNNNNN
....NNNNNNNNNNNNNNNNNNNN
....NNNNNNNNNNNNNNNNNNNN
....NNNNNWWWWWWWWWWNNNNN
....NNNNWNNNNNNNNNNWNNNN
....NNNNWNNNNNNNNNNWNNNN
....NNNNWNWWNWWNWWNWNNNN
....NNNNWNWWNWWNWWNWNNNN
....NNNNWNNNNNNNNNNNWNNN
....NNNNWNNNNNNNNNNNNWNN
....NNNNNWWWWWWWWWWWWWNN
.....NNNNNNNNNNNNNNNNNNN
......NNNNNNNNNNNNNNNNNN
.......NNNNNNNNNNNNNNNNN
"""
LETTER = {'.': T, 'N': N, 'c': C, 'W': W, 'o': O, 'y': Y, 'G': G, 'L': L, 'O': O, 'R': C}

class Canvas:
    def __init__(self, w, h, fill=T):
        self.w, self.h = w, h
        self.px = [[fill] * w for _ in range(h)]
    def put(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.px[y][x] = c
    def rect(self, x0, y0, x1, y1, c):  # inclusive
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.put(x, y, c)
    def hline(self, x0, x1, y, c): self.rect(x0, y, x1, y, c)
    def vline(self, x, y0, y1, c): self.rect(x, y0, x, y1, c)
    def box(self, x0, y0, x1, y1, outline, fill=None):
        if fill is not None:
            self.rect(x0, y0, x1, y1, fill)
        self.hline(x0 + 1, x1 - 1, y0, outline); self.hline(x0 + 1, x1 - 1, y1, outline)
        self.vline(x0, y0 + 1, y1 - 1, outline); self.vline(x1, y0 + 1, y1 - 1, outline)
    def art(self, x0, y0, text):
        for dy, row in enumerate(text.splitlines()):
            for dx, ch in enumerate(row):
                if ch != '.':
                    self.put(x0 + dx, y0 + dy, LETTER[ch])

MOCK_COLORS = {
    (0x27, 0x1F, 0x3B): N, (0xE1, 0x5F, 0x69): C, (0xFF, 0xFF, 0xFF): W, (0x40, 0x57, 0x80): B,
    (0xB2, 0xDC, 0xEF): L, (0x3D, 0x4B, 0x66): S, (0xA0, 0xC1, 0x6C): G, (0x92, 0xB7, 0x5A): G2,
    (0xEF, 0xD2, 0xD2): P, (0x31, 0xA2, 0xF2): CY, (0xFF, 0x94, 0x42): O, (0xFF, 0xBE, 0x09): Y,
}

# ---- message and action screens: copied from the mockup (frames 5 and 0) ----
# Both cover BG rows 13-19 of their screen (y 104-159); the panel starts at y 111.
# The LCD is drawn flat: the mockup's scanlines can't survive the text printer, which clears
# and scrolls the message window with one fill color.

PANEL_TOP = 104
MSG_WIN = (8, 128, 215, 151)        # B_WIN_MSG: cols 1-26, rows 16-18
PROMPT_WIN = (8, 128, 111, 151)     # B_WIN_ACTION_PROMPT: cols 1-13, rows 36-38
ACTION_WIN = (128, 120, 207, 159)   # B_WIN_ACTION_MENU: cols 16-25, rows 35-39, art copied in
BUTTONS = (130, 126, 207, 154)      # the four buttons, drawn by the engine

def copy_mockup(cv, src, x0, y0, x1, y1, top):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            p = src.getpixel((x, y))
            if p[3]:
                c = MOCK_COLORS[p[:3]]
                cv.put(x, y - top, G if c == G2 else c)

def lower_notch(cv, top):
    """The LCD's top-left notch sits 1 px higher than in the mockup, so the LCD
    starts on the text window's first row (y 128)."""
    for y in range(123, 128):
        for x in range(0, 27):
            cv.px[y - top][x] = cv.px[y + 1 - top][x]
    for x in range(0, 27):
        cv.px[128 - top][x] = cv.px[129 - top][x]

def screen_message():
    src = Image.open(os.path.join(ROOT, 'tools/battle_ui/src/message_v0.1.png')).convert('RGBA')
    cv = Canvas(256, 56)
    copy_mockup(cv, src, 0, 111, 239, 159, PANEL_TOP)
    lower_notch(cv, PANEL_TOP)
    return cv

def screen_action():
    src = Image.open(os.path.join(ROOT, 'tools/battle_ui/src/action_v0.1.png')).convert('RGBA')
    cv = Canvas(256, 56)
    copy_mockup(cv, src, 0, 111, 124, 118, PANEL_TOP)   # above row 119 the right side is the healthbox
    copy_mockup(cv, src, 0, 119, 239, 159, PANEL_TOP)
    lower_notch(cv, PANEL_TOP)
    x0, y0, x1, y1 = BUTTONS
    cv.rect(x0, y0 - PANEL_TOP, x1, y1 - PANEL_TOP, N)
    return cv

# ---- the move screen: copied from the mockup (tools/battle_ui/src/move_v0.1.png, UItest2.ase frame 6) ----
# It covers BG rows 51-59 (screen y 88-159): the move list is taller than the other screens.

MOVE_TOP = 88
TY = 16  # marker: the move type's color, BG palette 5 slot B_UI_PAL_TYPE

# Screen rects (x0, y0, x1, y1, inclusive) the engine draws into. Windows are in src/battle_bg.c.
GRID = (3, 111, 115, 148)          # move names, scanlined; window B_WIN_MOVE_NAME_1 is x0-119, y104-151
GRID_WIN = (0, 104, 119, 151)
TYPE_TEXT = (137, 126, 174, 133)   # in the type tab; window B_WIN_MOVE_TYPE x136-175, y120-135
TYPE_WIN = (136, 120, 175, 135)
COST_TEXT = (146, 137, 167, 148)   # the TP cost; window B_WIN_PP_REMAINING x144-167, y136-151
COST_WIN = (144, 136, 167, 151)
TP_TEXT = (195, 133, 215, 151)     # current/max TP; window B_WIN_TP_LEFT x192-215, y128-151
TP_WIN = (192, 128, 215, 151)
ICON = (216, 133, 231, 151)        # effectiveness icon, tiles at x216-231, y136-151
ICON_TILES = (216, 136, 231, 151)

def screen_move():
    src = Image.open(os.path.join(ROOT, 'tools/battle_ui/src/move_v0.1.png')).convert('RGBA')
    cv = Canvas(256, 72)
    def copy(x0, y0, x1, y1, type_tint):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                p = src.getpixel((x, y))
                if p[3] == 0:
                    continue
                c = MOCK_COLORS[p[:3]]
                if type_tint and c == G:
                    c = TY
                cv.put(x, y - MOVE_TOP, c)
    copy(0, 91, 127, 159, False)     # move list device; above it is the player's mon
    copy(128, 118, 239, 159, True)   # info box; above it is the player's healthbox
    # scanlines under the move names
    x0, y0, x1, y1 = GRID
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            cv.put(x, y - MOVE_TOP, B if (y & 1 and x0 < x < x1) else S)
    cv.put(x0, y0 - MOVE_TOP, L)
    for x0, y0, x1, y1 in (TYPE_TEXT, COST_TEXT, TP_TEXT, ICON):
        cv.rect(x0, y0 - MOVE_TOP, x1, y1 - MOVE_TOP, N)
    return cv

def crop(cv, rect, top):
    x0, y0, x1, y1 = rect
    out = Canvas(x1 - x0 + 1, y1 - y0 + 1)
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            out.put(x - x0, y - y0, cv.px[y - top][x])
    return out

def effect_icons():
    """Effectiveness icons in the type color, 16x16 over the info box background:
    = neutral (frame 3), down arrow not very effective (frame 6), up arrow super effective, x no effect."""
    src = Image.open(os.path.join(ROOT, 'tools/battle_ui/src/move_v0.1.png')).convert('RGBA')
    down = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            p = src.getpixel((ICON_TILES[0] + x, ICON_TILES[1] + y))
            if p[3] and MOCK_COLORS[p[:3]] == G:
                down.put(x, y, TY)
    neutral = Canvas(16, 16)
    neutral.rect(1, 4, 12, 6, TY); neutral.rect(1, 8, 12, 10, TY)
    up = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            up.put(x, y, down.px[15 - y][x])
    none = Canvas(16, 16)
    for i in range(10):
        none.rect(2 + i, 2 + i, 3 + i, 2 + i, TY)
        none.rect(11 - i, 2 + i, 12 - i, 2 + i, TY)
    return neutral, down, up, none

def talk_badge(cv):
    cv.art(216, 0, TALK_BADGE)

# ---- tiling ----

def tile_at(cv, tx, ty):
    return tuple(tuple(cv.px[ty * 8 + y][tx * 8 + x] for x in range(8)) for y in range(8))

def hflip(t): return tuple(tuple(reversed(r)) for r in t)
def vflip(t): return tuple(reversed(t))

RESERVED = {0, 1, 2} | set(range(0x12, 0x1B)) | set(range(0x22, 0x2B)) | set(range(0x7F, 0x10C))

class TileSet:
    def __init__(self, old_png):
        self.tiles = {}
        self.lookup = {}
        blank = tuple(tuple([0] * 8) for _ in range(8))
        self.add_fixed(0, blank)
        old = Image.open(old_png)
        arrow = {1: 14, 9: 13, 7: 15}  # Emerald's palette 0 colors -> text.pal (already-mapped pixels stay)
        for i in (1, 2):  # yes/no arrow, palette 5
            self.tiles[i] = tuple(tuple(arrow.get(old.getpixel(((i % 16) * 8 + x, (i // 16) * 8 + y)), old.getpixel(((i % 16) * 8 + x, (i // 16) * 8 + y))) for x in range(8)) for y in range(8))
        self.next = 3
    def add_fixed(self, idx, t):
        self.tiles[idx] = t
        self.lookup.setdefault(t, (idx, 0))
    def index(self, t, limit):
        for flip, f in ((0, lambda a: a), (1, hflip), (2, vflip), (3, lambda a: vflip(hflip(a)))):
            key = f(t)
            if key in self.lookup:
                return self.lookup[key][0] | (flip << 10)
        while self.next in RESERVED or self.next in self.tiles:
            self.next += 1
        if self.next >= limit:
            raise SystemExit('out of tiles (limit 0x%x)' % limit)
        idx = self.next
        self.tiles[idx] = t
        self.lookup[t] = (idx, 0)
        return idx

TO_PAL5 = {T: 0, C: 1, N: 5, 9: 9, W: 14}  # textbox colors to BG palette 5 (text.pal); 9 = B_UI_PAL_TYPE

def to_pal5(t):
    return tuple(tuple(TO_PAL5[9 if c == TY else c] for c in row) for row in t)

def write_png(path, tiles, width_tiles, pal):
    rows = (len(tiles) + width_tiles - 1) // width_tiles
    img = Image.new('P', (width_tiles * 8, rows * 8), 0)
    flat = []
    for c in pal:
        flat += list(c)
    img.putpalette(flat + [0] * (768 - len(flat)))
    for idx, t in enumerate(tiles):
        for y in range(8):
            for x in range(8):
                img.putpixel(((idx % width_tiles) * 8 + x, (idx // width_tiles) * 8 + y), t[y][x])
    img.save(path)

def window_tiles(cv, rect, top):
    w = crop(cv, rect, top)
    return [tile_at(w, tx, ty) for ty in range(w.h // 8) for tx in range(w.w // 8)]

def read_jasc(path):
    lines = open(path).read().split()
    return [tuple(int(v) for v in lines[3 + i * 3:6 + i * 3]) for i in range(16)]

ART_END = 0x190

def main():
    ts = TileSet(os.path.join(GFX, 'textbox.png'))
    tilemap = [0] * (32 * 64)

    def place(cv, top_row, rows, limit, skip=(), scroll=0):
        for ty in range(rows):
            for tx in range(30):
                if any(x0 <= tx * 8 <= x1 and y0 <= (top_row + ty) * 8 - scroll <= y1 for x0, y0, x1, y1 in skip):
                    continue  # under a window that is always drawn before the screen shows
                t = tile_at(cv, tx, ty)
                if any(TY in row for row in t):
                    tilemap[(top_row + ty) * 32 + tx] = ts.index(to_pal5(t), limit) | (5 << 12)
                else:
                    tilemap[(top_row + ty) * 32 + tx] = ts.index(t, limit)

    msg, act, mov = screen_message(), screen_action(), screen_move()
    badge = Canvas(256, 24)
    talk_badge(badge)
    # message screen first: everything on it must sit below tile 0x40
    place(msg, 13, 7, 0x40)
    place(badge, 0, 3, 0x40)
    # the other screens may use any free tile, including gaps below 0x40
    place(badge, 20, 3, ART_END)
    place(badge, 40, 3, ART_END)
    place(act, 33, 7, ART_END, (ACTION_WIN,), 160)
    place(mov, 51, 9, ART_END, (GRID_WIN, TYPE_WIN, COST_WIN, TP_WIN), 320)

    # effectiveness icons (palette 5): 0x7F navy for no icon, then 4 tiles per icon from 0x80
    bg = crop(mov, ICON_TILES, MOVE_TOP)
    assert all(c == N for row in bg.px for c in row)
    ts.tiles[0x7F] = to_pal5(tile_at(bg, 0, 0))
    for i, icon in enumerate(effect_icons()):
        comp = Canvas(16, 16)
        for y in range(16):
            for x in range(16):
                comp.put(x, y, icon.px[y][x] or bg.px[y][x])
        for j, (tx, ty) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
            ts.tiles[0x80 + i * 4 + j] = to_pal5(tile_at(comp, tx, ty))

    used = max(ts.tiles) + 1
    rows = (used + 15) // 16
    write_png(os.path.join(GFX, 'textbox.png'), [ts.tiles.get(i, ts.tiles[0]) for i in range(rows * 16)], 16, PAL)

    # backgrounds the move screen windows copy in before printing (see battle_controller_player.c)
    write_png(os.path.join(GFX, 'move_grid.png'), window_tiles(mov, GRID_WIN, MOVE_TOP), 15, PAL)
    write_png(os.path.join(GFX, 'action_menu.png'), window_tiles(act, ACTION_WIN, PANEL_TOP), 10, PAL)
    info = []
    for rect in (TYPE_WIN, COST_WIN, TP_WIN):
        info += [to_pal5(t) for t in window_tiles(mov, rect, MOVE_TOP)]
    write_png(os.path.join(GFX, 'move_info.png'), info, 1, read_jasc(os.path.join(GFX, 'text.pal')))

    with open(os.path.join(GFX, 'textbox_0.pal'), 'w', newline='\r\n') as f:
        f.write('JASC-PAL\n0100\n16\n')
        for r, g, b in PAL:
            f.write('%d %d %d\n' % (r, g, b))
    with open(os.path.join(GFX, 'textbox_map.bin'), 'wb') as f:
        f.write(struct.pack('<%dH' % len(tilemap), *tilemap))
    print('tiles used: %d (highest 0x%x)' % (len(ts.tiles), used - 1))

if __name__ == '__main__':
    main()
