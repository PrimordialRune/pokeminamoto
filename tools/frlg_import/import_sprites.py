#!/usr/bin/env python3
"""Import FireRed/LeafGreen overworld (object event) sprites from a pokefirered checkout.

    python3 tools/frlg_import/import_sprites.py /path/to/pokefirered

Every FRLG graphics id OBJ_EVENT_GFX_<NAME> becomes OBJ_EVENT_GFX_FRLG_<NAME>,
numbered after Emerald's ids (graphics ids are u16 for this). Graphics go to
graphics/object_events/frlg/, the data tables to src/data/object_events/frlg/
with every symbol renamed *_Frlg* so nothing clashes with Emerald's tables.
Palette tags become OBJ_EVENT_PAL_TAG_FRLG_* (0x1140 up). Rerunning this
script overwrites what it generated before.
"""

import os
import re
import shutil
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(ROOT, 'src/data/object_events/frlg')
PAL_TAG_BASE = 0x1140
DATA_FILES = ['object_event_graphics.h', 'object_event_pic_tables.h', 'object_event_anims.h',
              'base_oam.h', 'object_event_subsprites.h', 'object_event_graphics_info.h']
BEGIN = '// FRLG import begin (tools/frlg_import/import_sprites.py)\n'
END = '// FRLG import end\n'
HEADER = '// Generated from pokefirered by tools/frlg_import/import_sprites.py.\n\n'


def frlg_symbol(name):
    return name.replace('_', '_Frlg', 1) if '_' in name else name + '_Frlg'


IN_PLACE = ('object_event_graphics.h', 'object_event_pic_tables.h', 'object_event_graphics_info.h')


def set_block(path, block, before=None, tag=''):
    """Puts block between FRLG markers in path: at the end, or before the last
    occurrence of `before`. A rerun replaces the old block."""
    begin = BEGIN.replace(')', ', %s)' % tag) if tag else BEGIN
    end = END.replace('end', 'end %s' % tag) if tag else END
    text = open(path).read()
    if begin in text:
        text = text[:text.index(begin)] + text[text.index(end) + len(end):]
    if before is None:
        text = text.rstrip('\n') + '\n\n' + begin + block + end
    else:
        i = text.rindex(before)
        text = text[:i] + begin + block + end + text[i:]
    open(path, 'w').write(text)


def main(frlg):
    src_dir = os.path.join(frlg, 'src/data/object_events')
    files = {f: open(os.path.join(src_dir, f)).read() for f in DATA_FILES}

    # Graphics ids, in FRLG order
    consts = open(os.path.join(frlg, 'include/constants/event_objects.h')).read()
    num = int(re.search(r'#define NUM_OBJ_EVENT_GFX\s+(\d+)', consts).group(1))
    ids = [(m.group(1), int(m.group(2))) for m in re.finditer(r'#define OBJ_EVENT_GFX_(\w+)\s+(\d+)\s*$', consts, re.M)]
    ids = sorted([i for i in ids if i[1] < num], key=lambda i: i[1])
    pointers = dict(re.findall(r'\[OBJ_EVENT_GFX_(\w+)\]\s*=\s*&(\w+)',
                               open(os.path.join(src_dir, 'object_event_graphics_info_pointers.h')).read()))

    # Palette tags and the palette table
    mov = open(os.path.join(frlg, 'src/event_object_movement.c')).read()
    tags = re.findall(r'#define OBJ_EVENT_PAL_TAG_(\w+)\s+0x[0-9A-Fa-f]+', mov)
    tags = [t for t in tags if t not in ('NONE', 'BRIDGE_REFLECTION')]  # these two are shared with ours
    pal_table = re.search(r'sObjectEventSpritePalettes\[\] = \{(.*?)\n\};', mov, re.S).group(1)
    pal_entries = re.findall(r'\{(\w+),\s*OBJ_EVENT_PAL_TAG_(\w+)\}', pal_table)

    # Every symbol the data files define gets renamed
    defined = set()
    for text in files.values():
        defined |= set(re.findall(r'^(?:static )?const [^=;(]*?\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*=', text, re.M))
    sym_re = re.compile(r'\b(%s)\b' % '|'.join(sorted(defined, key=len, reverse=True)))
    tag_re = re.compile(r'\bOBJ_EVENT_PAL_TAG_(%s)\b' % '|'.join(sorted(tags, key=len, reverse=True)))

    def convert(text):
        # object_event_graphics.h also has FRLG's field effect graphics: not ours to take
        text = '\n'.join(l for l in text.split('\n') if not re.search(r'INCBIN_\w+\("graphics/(?!object_events/)', l))
        text = sym_re.sub(lambda m: frlg_symbol(m.group(1)), text)
        text = tag_re.sub(r'OBJ_EVENT_PAL_TAG_FRLG_\1', text)
        return text.replace('"graphics/object_events/', '"graphics/object_events/frlg/')

    if os.path.isdir(OUT):
        shutil.rmtree(OUT)
    os.makedirs(OUT)
    # Pics, pic tables and graphics infos go into our own files (marked blocks) so
    # Porymap, which reads only those, shows the FRLG sprites. The rest stays in OUT.
    for f, text in files.items():
        if f in IN_PLACE:
            set_block(os.path.join(ROOT, 'src/data/object_events', f), convert(text).strip('\n') + '\n')
        else:
            open(os.path.join(OUT, f), 'w').write(HEADER + convert(text))

    # FRLG-only sprite animation indexes (spin, raise hand, VS Seeker...)
    frlg_mov = open(os.path.join(frlg, 'include/constants/event_object_movement.h')).read()
    our_mov = open(os.path.join(ROOT, 'include/constants/event_object_movement.h')).read()
    anim_defs = ''.join('#define %s %s\n' % (n, v) for n, v in re.findall(r'^#define (ANIM_\w+)\s+(.+)$', frlg_mov, re.M)
                        if not re.search(r'#define %s\b' % n, our_mov))
    open(os.path.join(OUT, 'palette_tags.h'), 'w').write(HEADER + ''.join(
        '#define OBJ_EVENT_PAL_TAG_FRLG_%-30s 0x%04X\n' % (t, PAL_TAG_BASE + i) for i, t in enumerate(tags))
        + '\n' + anim_defs)
    open(os.path.join(OUT, 'sprite_palettes.h'), 'w').write(HEADER + ''.join(
        '    {%s, OBJ_EVENT_PAL_TAG_FRLG_%s},\n' % (frlg_symbol(p), t) for p, t in pal_entries if t in tags))
    infos = sorted({pointers[n] for n, _ in ids})
    ptr_path = os.path.join(ROOT, 'src/data/object_events/object_event_graphics_info_pointers.h')
    set_block(ptr_path, ''.join('extern const struct ObjectEventGraphicsInfo %s;\n' % frlg_symbol(i) for i in infos),
              before='\nconst struct ObjectEventGraphicsInfo *const gObjectEventGraphicsInfoPointers', tag='decls')
    set_block(ptr_path, ''.join('    [OBJ_EVENT_GFX_FRLG_%s] = &%s,\n' % (n, frlg_symbol(pointers[n])) for n, _ in ids),
              before='};', tag='pointers')

    # Graphics files
    gfx_src = os.path.join(frlg, 'graphics/object_events')
    gfx_dst = os.path.join(ROOT, 'graphics/object_events/frlg')
    if os.path.isdir(gfx_dst):
        shutil.rmtree(gfx_dst)
    used = set(re.findall(r'"graphics/object_events/([^"]+)\.(?:4bpp|8bpp|gbapal)"', files['object_event_graphics.h']))
    for path in used:
        for ext in ('.png', '.pal'):
            s = os.path.join(gfx_src, path + ext)
            if os.path.exists(s):
                os.makedirs(os.path.dirname(os.path.join(gfx_dst, path)), exist_ok=True)
                shutil.copyfile(s, os.path.join(gfx_dst, path + ext))

    # Spritesheet rules (frame sizes) for the copied pics
    rules = open(os.path.join(frlg, 'spritesheet_rules.mk')).read()
    pics = {p[len('pics/'):] for p in used if p.startswith('pics/')}
    out_rules = ['# Generated by tools/frlg_import/import_sprites.py\n\n']
    for m in re.finditer(r'^\$\(OBJEVENTGFXDIR\)/(\S+)\.4bpp: %\.4bpp: %\.png\n\t\$\(GFX\) \$< \$@ (.*)$', rules, re.M):
        if m.group(1) in pics:
            out_rules.append('graphics/object_events/frlg/pics/%s.4bpp: %%.4bpp: %%.png\n\t$(GFX) $< $@ %s\n\n' % (m.group(1), m.group(2)))
    open(os.path.join(ROOT, 'spritesheet_rules_frlg.mk'), 'w').write(''.join(out_rules))

    # Constants
    path = os.path.join(ROOT, 'include/constants/event_objects.h')
    text = open(path).read()
    if BEGIN in text:  # undo the previous run
        old = re.search(r'#define OBJ_EVENT_GFX_FRLG_BASE (\d+)', text).group(1)
        text = text[:text.index(BEGIN)] + text[text.index(END) + len(END):]
        text = re.sub(r'#define NUM_OBJ_EVENT_GFX\s+\(OBJ_EVENT_GFX_FRLG_BASE.*', '#define NUM_OBJ_EVENT_GFX %s' % old, text)
    em_num = re.search(r'#define NUM_OBJ_EVENT_GFX\s+(\d+)', text).group(1)
    block = ('// FireRed/LeafGreen sprites, numbered after Emerald\'s (graphics ids are u16).\n'
             '#define OBJ_EVENT_GFX_FRLG_BASE %s\n' % em_num)
    block += ''.join('#define OBJ_EVENT_GFX_FRLG_%-30s (OBJ_EVENT_GFX_FRLG_BASE + %d)\n' % (n, v) for n, v in ids)
    block += '#define NUM_OBJ_EVENT_GFX_FRLG %d\n\n' % len(ids)
    text = re.sub(r'#define NUM_OBJ_EVENT_GFX\s+(\d+)\n',
                  BEGIN + block + END + '#define NUM_OBJ_EVENT_GFX (OBJ_EVENT_GFX_FRLG_BASE + NUM_OBJ_EVENT_GFX_FRLG)\n', text)
    open(path, 'w').write(text)
    print('sprites %d, palettes %d, graphics files %d, sheet rules %d' % (len(ids), len(pal_entries), len(used), len(out_rules) - 1))


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(os.path.abspath(sys.argv[1]))
