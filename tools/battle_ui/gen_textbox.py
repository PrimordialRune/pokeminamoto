#!/usr/bin/env python3
"""Builds the battle textbox art (BG0) from Omar's Battle UI v0.1 mockup.

Writes graphics/battle_interface/textbox.png, textbox_0.pal and textbox_map.bin.
The art is drawn here on the 8x8 grid the engine's text windows need; the SEL
icon and Talk badge are copied from the mockup (MyRes/UI/UItest2.ase).

Tile index rules (see src/battle_bg.c, src/egg_hatch.c, src/trade.c):
  0          blank
  1-2        yes/no cursor arrow, drawn with palette 1 (kept from Emerald)
  0x12-0x1A  overwritten in battle by the player's window frame
  0x22-0x2A  overwritten in battle by the player's window frame
  < 0x40     everything on the message screen: egg hatch and trade load their
             text window at tile 0x40
  0x7F       solid navy, the "no effectiveness icon" tile
  0x80-0x8F  effectiveness icons, 2x2 tiles each: neutral, weak, strong, none
  >= 0x90    battle text windows
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
SEL_ICON = """\
..........NNNN..........
........NNyyyyNN........
......NNyooooooyNN......
.....NyoooyWWyoooyN.....
.....NooooooooooooN.....
....NyooooWWWWooooyN....
....NoooooWWWWoooooN....
....NoooooWWWWoooooN....
....NWooooWWWWooooWN....
....NNWoooyWWyoooWNN....
.....NNWooooooooWNN.....
.....NNNWWooooWWNNN.....
......NNNNWWWWNNNN......
.......NNNNNNNNNN.......
.........NNNNNN.........
........................
.NNNNNNNNNNNNNNNNNNNNN..
NNNWWWWNWWWWNWNNNNNNNNN.
NNNWWNNNWWWNNWNNNNNNNNN.
NNNNNWWNWNNNNWNNNNNNNNN.
NNNWWWWNWWWWNWWWWNWNNNN.
.NNNNNNNNNNNNNNNNNNNNN..
"""
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

# ---- the device panel, one 240x48 strip (BG rows 14-19) per battle screen ----

def device_top(cv, x0, x1, lens=True):
    """Coral body with the lens tab on top (rows 14+)."""
    cv.rect(x0, 0, x1, 47, C)
    cv.hline(x0, x1, 0, N)
    cv.hline(x0, x1, 1, P)
    cv.hline(x0, x1, 47, CD)
    if lens:
        # lens: navy ring, slate glass, white glint
        cv.rect(x0 + 3, 1, x0 + 7, 6, N); cv.rect(x0 + 2, 2, x0 + 8, 5, N)
        cv.rect(x0 + 4, 2, x0 + 6, 5, B); cv.rect(x0 + 3, 3, x0 + 7, 4, B)
        cv.put(x0 + 4, 3, W)
        for dx in (12, 17, 22):  # three status dots
            cv.rect(x0 + dx, 3, x0 + dx + 2, 4, N); cv.hline(x0 + dx + 1, x0 + dx + 1, 2, N); cv.hline(x0 + dx + 1, x0 + dx + 1, 5, N)

def lcd(cv, x0, y0, x1, y1):
    """Olive LCD screen with a navy rim and a darker lower edge."""
    cv.box(x0, y0, x1, y1, N, G)
    cv.hline(x0 + 1, x1 - 1, y1 - 1, G2)

def pipe(cv, x, y0, y1):
    """Pink vertical tube decoration from the mockup's right edge."""
    cv.vline(x, y0, y1, N); cv.vline(x + 3, y0, y1, N)
    cv.rect(x + 1, y0, x + 2, y1, P)
    cv.hline(x, x + 3, y0, N); cv.hline(x, x + 3, y1, N)
    cv.hline(x + 1, x + 2, y0 + 4, N); cv.hline(x + 1, x + 2, y1 - 4, N)

def screen_message():
    cv = Canvas(256, 48)
    device_top(cv, 0, 239)
    lcd(cv, 3, 7, 227, 44)          # text window B_WIN_MSG: cols 1-27, rows 15-18
    pipe(cv, 231, 10, 42)
    return cv

def screen_action():
    cv = Canvas(256, 48)
    device_top(cv, 0, 127)
    lcd(cv, 3, 7, 120, 44)          # B_WIN_ACTION_PROMPT: cols 1-14, rows 15-18
    pipe(cv, 122, 10, 42)
    # right box: starts lower, holds the buttons and the SEL. icon
    cv.rect(128, 3, 239, 47, C)
    cv.hline(128, 239, 3, N); cv.hline(129, 239, 4, P)
    cv.vline(128, 3, 47, N); cv.hline(128, 239, 47, CD)
    cv.box(135, 7, 208, 40, N)      # B_WIN_ACTION_MENU: cols 17-25, rows 15-18
    cv.art(212, 14, SEL_ICON)
    return cv

def screen_move():
    cv = Canvas(256, 48)
    device_top(cv, 0, 143, lens=False)
    # move list: slate screen with light rim
    cv.box(3, 3, 140, 44, N, S)
    cv.box(4, 4, 139, 43, L)
    cv.put(4, 4, L); cv.put(139, 4, L); cv.put(4, 43, L); cv.put(139, 43, L)
    for dx in (8, 13, 18):          # decoration from the mockup: three red dots, stripes
        cv.rect(dx, 40, dx + 2, 41, C)
    for dy in (40, 42):
        cv.hline(118, 134, dy, C)
    # info box: navy screen with a green rim
    cv.rect(144, 3, 239, 47, C)
    cv.hline(144, 239, 3, N); cv.hline(145, 239, 4, P)
    cv.vline(144, 3, 47, N); cv.hline(144, 239, 47, CD)
    cv.box(147, 7, 236, 44, G, N)
    cv.put(147, 7, C); cv.put(236, 7, C); cv.put(147, 44, C); cv.put(236, 44, C)
    return cv

def talk_badge(cv):
    cv.art(216, 0, TALK_BADGE)

# ---- effectiveness icons, 16x16 on navy ----

def icon_neutral():
    cv = Canvas(16, 16, N)
    cv.rect(2, 4, 13, 6, L); cv.rect(2, 9, 13, 11, L)
    return cv

def icon_weak():
    cv = Canvas(16, 16, N)
    cv.rect(2, 2, 13, 4, G)
    for i in range(6):
        cv.hline(2 + i, 13 - i, 6 + i, G)
    return cv

def icon_strong():
    cv = Canvas(16, 16, N)
    cv.rect(2, 11, 13, 13, O)
    for i in range(6):
        cv.hline(2 + i, 13 - i, 9 - i, O)
    return cv

def icon_none():
    cv = Canvas(16, 16, N)
    for i in range(10):
        cv.rect(3 + i, 3 + i, 4 + i, 3 + i, C)
        cv.rect(11 - i, 3 + i, 12 - i, 3 + i, C)
    return cv

# ---- tiling ----

def tile_at(cv, tx, ty):
    return tuple(tuple(cv.px[ty * 8 + y][tx * 8 + x] for x in range(8)) for y in range(8))

def hflip(t): return tuple(tuple(reversed(r)) for r in t)
def vflip(t): return tuple(reversed(t))

RESERVED = {0, 1, 2} | set(range(0x12, 0x1B)) | set(range(0x22, 0x2B))

class TileSet:
    def __init__(self, old_png):
        self.tiles = {}
        self.lookup = {}
        blank = tuple(tuple([0] * 8) for _ in range(8))
        self.add_fixed(0, blank)
        old = Image.open(old_png)
        for i in (1, 2):  # yes/no arrow, palette 1
            self.tiles[i] = tuple(tuple(old.getpixel(((i % 16) * 8 + x, (i // 16) * 8 + y)) for x in range(8)) for y in range(8))
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

def main():
    ts = TileSet(os.path.join(GFX, 'textbox.png'))
    tilemap = [0] * (32 * 64)

    def place(cv, top_row, rows, limit):
        for ty in range(rows):
            for tx in range(30):
                t = tile_at(cv, tx, ty)
                tilemap[(top_row + ty) * 32 + tx] = ts.index(t, limit)

    msg, act, mov = screen_message(), screen_action(), screen_move()
    badge = Canvas(256, 24)
    talk_badge(badge)
    # message screen first: everything on it must sit below tile 0x40
    place(msg, 14, 6, 0x40)
    place(badge, 0, 3, 0x40)
    ts.next = max(ts.next, 0x40)
    place(badge, 20, 3, 0x7F)
    place(badge, 40, 3, 0x7F)
    place(act, 34, 6, 0x7F)
    place(mov, 54, 6, 0x7F)

    navy = tuple(tuple([N] * 8) for _ in range(8))
    ts.tiles[0x7F] = navy
    for i, icon in enumerate((icon_neutral(), icon_weak(), icon_strong(), icon_none())):
        for j, (tx, ty) in enumerate(((0, 0), (1, 0), (0, 1), (1, 1))):
            ts.tiles[0x80 + i * 4 + j] = tile_at(icon, tx, ty)

    used = max(ts.tiles) + 1
    rows = (used + 15) // 16
    img = Image.new('P', (128, rows * 8), 0)
    flat = []
    for c in PAL:
        flat += list(c)
    img.putpalette(flat + [0] * (768 - len(flat)))
    for idx, t in ts.tiles.items():
        for y in range(8):
            for x in range(8):
                img.putpixel(((idx % 16) * 8 + x, (idx // 16) * 8 + y), t[y][x])
    img.save(os.path.join(GFX, 'textbox.png'))

    with open(os.path.join(GFX, 'textbox_0.pal'), 'w', newline='\r\n') as f:
        f.write('JASC-PAL\n0100\n16\n')
        for r, g, b in PAL:
            f.write('%d %d %d\n' % (r, g, b))
    with open(os.path.join(GFX, 'textbox_map.bin'), 'wb') as f:
        f.write(struct.pack('<%dH' % len(tilemap), *tilemap))
    print('tiles used: %d (highest 0x%x)' % (len(ts.tiles), used - 1))

if __name__ == '__main__':
    main()
