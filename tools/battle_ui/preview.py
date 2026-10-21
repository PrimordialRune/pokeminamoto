#!/usr/bin/env python3
"""Renders the three BG0 battle screens (message, action, move) over a field image for review."""
import struct, sys
from PIL import Image
root, field, out = sys.argv[1], sys.argv[2], sys.argv[3]
G = root + '/graphics/battle_interface/'
pal = [tuple(int(v) for v in l.split()) for l in open(G + 'textbox_0.pal').read().split('\n')[3:] if l.strip()]
t = Image.open(G + 'textbox.png'); tp = t.load()
d = open(G + 'textbox_map.bin', 'rb').read(); e = struct.unpack('<%dH' % (len(d) // 2), d)
bg = Image.open(field).convert('RGB').resize((240, 160))
sheet = Image.new('RGB', (240 * 3 + 8, 160), (255, 0, 255))
for s in range(3):
    im = bg.copy()
    for r in range(20):
        for c in range(30):
            v = e[(s * 20 + r) * 32 + c]; ti = v & 0x3ff; hf = v >> 10 & 1; vf = v >> 11 & 1
            for y in range(8):
                for x in range(8):
                    px = tp[(ti % 16) * 8 + (7 - x if hf else x), (ti // 16) * 8 + (7 - y if vf else y)]
                    if px:
                        im.putpixel((c * 8 + x, r * 8 + y), pal[px])
    sheet.paste(im, (s * 244, 0))
sheet.resize((sheet.width * 2, 320), Image.NEAREST).save(out)
