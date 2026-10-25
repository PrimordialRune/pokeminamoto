#!/usr/bin/env python3
"""One-time conversion of the hand-written species tables into records.

Run from the repository root on the tree before speciesgen took over
(commit 61ff45d75 or the tables restored from it):

    python3 tools/speciesgen/extract.py src/data/species [facts.json]

It reads every per-species C table (constants, base stats, names, learnsets,
evolutions, Pokedex data, cries, graphics and animation tables) and writes
one JSON record per species plus species.txt with the species order. Kept in
the tree as documentation of how the records were made; speciesgen.py is the
tool you run.
"""
import json
import os
import re
import sys
from collections import OrderedDict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from recordfmt import dump
from cparse import (strip_comments, array_body, designated, split_top, fields,
                    c_string, cpp_values, enum_values, matching, eval_c)


def read(path):
    with open(path, encoding='utf-8') as f:
        return strip_comments(f.read())


def unprefix(value, prefix):
    value = value.strip()
    assert value.startswith(prefix), (value, prefix)
    return value[len(prefix):]


# ---------------------------------------------------------------- species ids
species_h = read('include/constants/species.h')
names = re.findall(r'#define (SPECIES_\w+)\s', species_h)
names = [n for n in names if n not in ('SPECIES_GEN1_MAX',)]
ids = cpp_values(names, ['constants/species.h'])
ids.pop('NUM_SPECIES', None)
by_id = {}
for n, v in ids.items():
    if v in by_id:
        raise SystemExit('duplicate species id %d: %s %s' % (v, by_id[v], n))
    by_id[v] = n
order = [by_id[i] for i in range(len(by_id))]
assert order[0] == 'SPECIES_NONE'
consts = [unprefix(n, 'SPECIES_') for n in order]
NUM = cpp_values(['NUM_SPECIES'], ['constants/species.h'])['NUM_SPECIES']
assert consts[NUM] == 'EGG'
sid = {c: i for i, c in enumerate(consts)}

R = OrderedDict((c, OrderedDict(species=c)) for c in consts)


def sp_key(key):
    """'SPECIES_X' or 'SPECIES_X - 1' etc. -> (const, offset)."""
    m = re.match(r'SPECIES_(\w+)\s*(?:-\s*(\d+))?$', key.strip())
    return m.group(1), int(m.group(2) or 0)


# ---------------------------------------------------------------- symbols
# Every species has a front animation table sAnims_<Symbol>: use it as the
# record's symbol, the stem of all its C names.
anims_h = read('src/data/pokemon_graphics/front_pic_anims.h')
anim_uses = [(sp_key(k)[0], unprefix(v, 'sAnims_'))
             for k, v in designated(array_body(anims_h, 'gMonFrontAnimsPtrTable'))]
stem_count = {}
for c, stem in anim_uses:
    stem_count[stem] = stem_count.get(stem, 0) + 1
for c, stem in anim_uses:
    # a table several species share names none of them except its owner
    R[c]['symbol'] = stem if stem_count[stem] == 1 or stem == ''.join(w.capitalize() for w in c.split('_')) else \
        ''.join(w.capitalize() for w in c.split('_'))
for c in consts:
    assert 'symbol' in R[c], c

# ---------------------------------------------------------------- names
names_h = read('src/data/text/species_names.h')
for k, v in designated(array_body(names_h, 'gSpeciesNames')):
    R[sp_key(k)[0]]['name'] = c_string(v)

# ---------------------------------------------------------------- dex numbers
dex_h = read('include/constants/pokedex.h')
national = enum_values(dex_h, 'NATIONAL_DEX_NONE')
hoenn = enum_values(dex_h, 'HOENN_DEX_NONE')
pokemon_c = read('src/pokemon.c')
for name, table, macro, field in (
        ('sSpeciesToHoennPokedexNum', hoenn, 'SPECIES_TO_HOENN', 'hoennDex'),
        ('sSpeciesToNationalPokedexNum', national, 'SPECIES_TO_NATIONAL', 'nationalDex')):
    for part in split_top(array_body(pokemon_c, name)):
        m = re.match(r'%s\((\w+)\)$' % macro, part)
        R[m.group(1)][field] = table[('HOENN_DEX_' if field == 'hoennDex' else 'NATIONAL_DEX_') + m.group(1)]
# HOENN_TO_NATIONAL entries must agree with the two maps above
for part in split_top(array_body(pokemon_c, 'sHoennToNationalOrder')):
    m = re.match(r'HOENN_TO_NATIONAL\((\w+)\)$', part)
    c = m.group(1)
    assert R[c]['hoennDex'] == hoenn['HOENN_DEX_' + c] and R[c]['nationalDex'] == national['NATIONAL_DEX_' + c], c
# national dex enum names must be species names
for k, v in national.items():
    if k.startswith('NATIONAL_DEX_') and k != 'NATIONAL_DEX_NONE' and not k.endswith('_COUNT'):
        assert R[k[13:]].get('nationalDex') == v, k

# ---------------------------------------------------------------- species info
info_h = read('src/data/pokemon/species_info.h')
m = re.search(r'#define OLD_UNOWN_SPECIES_INFO(.*?)\n(?=\S)', info_h, re.S)
old_unown_info = m.group(1).replace('\\\n', '\n')
STAT_KEYS = [('baseHP', 'hp'), ('baseAttack', 'attack'), ('baseDefense', 'defense'),
             ('baseSpeed', 'speed'), ('baseSpAttack', 'spAttack'), ('baseSpDefense', 'spDefense')]
EV_KEYS = [('evYield_HP', 'hp'), ('evYield_Attack', 'attack'), ('evYield_Defense', 'defense'),
           ('evYield_Speed', 'speed'), ('evYield_SpAttack', 'spAttack'), ('evYield_SpDefense', 'spDefense')]
FRIENDSHIP = {'STANDARD_FRIENDSHIP': 70}


def num(v):
    return eval_c(v)


def plist(v, prefix):
    v = v.strip()
    assert v[0] == '{' and v[-1] == '}'
    return [unprefix(x, prefix) for x in split_top(v[1:-1])]


for k, v in designated(array_body(info_h, 'gSpeciesInfo')):
    c = sp_key(k)[0]
    if v == 'OLD_UNOWN_SPECIES_INFO':
        v = old_unown_info.strip()
    if v.replace(' ', '') == '{0}':
        R[c]['info'] = None
        continue
    f = fields(v)
    r = R[c]
    r['baseStats'] = OrderedDict((out, num(f.pop(k2))) for k2, out in STAT_KEYS)
    r['types'] = plist(f.pop('types'), 'TYPE_')
    r['catchRate'] = num(f.pop('catchRate'))
    r['expYield'] = num(f.pop('expYield'))
    ev = OrderedDict((out, num(f.pop(k2))) for k2, out in EV_KEYS)
    r['evYield'] = OrderedDict((k2, x) for k2, x in ev.items() if x)
    items = [unprefix(f.pop('itemCommon'), 'ITEM_'), unprefix(f.pop('itemRare'), 'ITEM_')]
    if items != ['NONE', 'NONE']:
        r['heldItems'] = items
    g = f.pop('genderRatio')
    gm = re.match(r'PERCENT_FEMALE\(([\d.]+)\)$', g)
    if gm:
        x = float(gm.group(1))
        r['genderRatio'] = int(x) if x == int(x) else x
    else:
        r['genderRatio'] = {'MON_MALE': 'MALE', 'MON_FEMALE': 'FEMALE', 'MON_GENDERLESS': 'GENDERLESS'}[g]
    r['eggCycles'] = num(f.pop('eggCycles'))
    fr = f.pop('friendship')
    r['friendship'] = FRIENDSHIP.get(fr) or num(fr)
    r['growthRate'] = unprefix(f.pop('growthRate'), 'GROWTH_')
    r['eggGroups'] = plist(f.pop('eggGroups'), 'EGG_GROUP_')
    r['abilities'] = plist(f.pop('abilities'), 'ABILITY_')
    r['safariZoneFleeRate'] = num(f.pop('safariZoneFleeRate'))
    r['bodyColor'] = unprefix(f.pop('bodyColor'), 'BODY_COLOR_')
    nf = f.pop('noFlip')
    if nf != 'FALSE':
        assert nf == 'TRUE'
        r['noFlip'] = True
    assert not f, (c, f)


# ------------------------------------------------- shared-data bookkeeping
# A table entry that points at data another species owns becomes "@OWNER".
# Data belongs to the species whose symbol gives its C name; when no species
# does, to the first one that uses it, which then records the name.
def assign(field, uses, derive, value_of, sub=None):
    """uses: [(const, c_symbol)] in table order. Sets R[const][field] (or
    R[const][sub][field])."""
    owner_of = {}
    for c, sym in uses:
        if sym in owner_of:
            continue
        cands = [x for x, s in uses if s == sym and derive(R[x]['symbol']) == sym]
        owner_of[sym] = cands[0] if cands else c
    for c, sym in uses:
        tgt = R[c] if sub is None else R[c].setdefault(sub, OrderedDict())
        own = owner_of[sym]
        if own == c:
            val = value_of(sym)
            if derive(R[c]['symbol']) != sym:
                val = OrderedDict([('symbol', sym), ('data', val)]) if not isinstance(val, dict) else OrderedDict([('symbol', sym)] + list(val.items()))
            tgt[field] = val
        else:
            tgt[field] = '@' + own


# ---------------------------------------------------------------- level-up moves
lv_h = read('src/data/pokemon/level_up_learnsets.h')
learnsets = {}
for m in re.finditer(r'static const u16 (\w+)\[\]\s*=\s*\{', lv_h):
    body = lv_h[m.end():matching(lv_h, m.end() - 1)]
    moves = []
    for part in split_top(body):
        if part == 'LEVEL_UP_END':
            break
        mm = re.match(r'LEVEL_UP_MOVE\(\s*(\d+)\s*,\s*MOVE_(\w+)\s*\)$', part)
        moves.append([int(mm.group(1)), mm.group(2)])
    learnsets[m.group(1)] = moves
lvp_h = read('src/data/pokemon/level_up_learnset_pointers.h')
uses = [(sp_key(k)[0], v) for k, v in designated(array_body(lvp_h, 'gLevelUpLearnsets'))]
assign('levelUpMoves', uses, lambda s: 's%sLevelUpLearnset' % s, lambda s: learnsets[s])

# ---------------------------------------------------------------- TM/HM
tm_h = read('src/data/pokemon/tmhm_learnsets.h')
for k, v in designated(array_body(tm_h, 'gTMHMLearnsets')):
    c = sp_key(k)[0]
    v = v.strip()
    if v == '{}':
        R[c]['tmhm'] = []
        continue
    f = fields(v)
    inner = fields(f['learnset'])
    assert all(x == 'TRUE' for x in inner.values()), c
    R[c]['tmhm'] = list(inner.keys())

# ---------------------------------------------------------------- tutor
tu_h = read('src/data/pokemon/tutor_learnsets.h')
for k, v in designated(array_body(tu_h, 'sTutorLearnsets')):
    c = sp_key(k)[0]
    R[c]['tutor'] = re.findall(r'TUTOR\(MOVE_(\w+)\)', v)
    assert R[c]['tutor'] or v.replace(' ', '') == '(0)', (c, v)

# ---------------------------------------------------------------- egg moves
egg_h = read('src/data/pokemon/egg_moves.h')
egg_order = []
for part in split_top(array_body(egg_h, 'gEggMoves')):
    if part == 'EGG_MOVES_TERMINATOR':
        continue
    m = re.match(r'egg_moves\((.*)\)$', part, re.S)
    parts = split_top(m.group(1))
    R[parts[0]]['eggMoves'] = [unprefix(p, 'MOVE_') for p in parts[1:]]
    egg_order.append(parts[0])

# ---------------------------------------------------------------- evolutions
evo_h = read('src/data/pokemon/evolution.h')
for k, v in designated(array_body(evo_h, 'gEvolutionTable')):
    c = sp_key(k)[0]
    evos = []
    for e in split_top(v.strip()[1:-1]):
        a = split_top(e.strip()[1:-1])
        evos.append(OrderedDict([('method', unprefix(a[0], 'EVO_')),
                                 ('param', a[1] if not re.match(r'^\d+$', a[1].strip()) else int(a[1])),
                                 ('into', unprefix(a[2], 'SPECIES_'))]))
    R[c]['evolutions'] = evos
for r in R.values():
    for e in r.get('evolutions', []):
        if isinstance(e['param'], str):
            e['param'] = unprefix(e['param'], 'ITEM_') if e['param'].startswith('ITEM_') else e['param']

# ---------------------------------------------------------------- pokedex
ent_h = read('src/data/pokemon/pokedex_entries.h')
txt_h = read('src/data/pokemon/pokedex_text.h')
texts = {}
text_order = []
for m in re.finditer(r'const u8 (\w+)\[\]\s*=\s*_\(', txt_h):
    end = matching(txt_h, m.end() - 1)
    lines = re.findall(r'"((?:[^"\\]|\\.)*)"', txt_h[m.end():end])
    # one string per line, each but the last ending in \n: store the lines
    assert all(l.endswith('\\n') for l in lines[:-1]) and not lines[-1].endswith('\\n'), lines
    texts[m.group(1)] = [l[:-2] if l.endswith('\\n') else l for l in lines]
    text_order.append(m.group(1))
by_national = {}
for c, r in R.items():
    if 'nationalDex' in r:
        by_national.setdefault(r['nationalDex'], c)
by_national[0] = 'NONE'
dex_uses = []
dex_vals = {}
for k, v in designated(array_body(ent_h, 'gPokedexEntries')):
    c = by_national[national[k]]
    f = fields(v)
    e = OrderedDict()
    e['category'] = c_string(f.pop('categoryName'))
    e['height'] = num(f.pop('height'))
    e['weight'] = num(f.pop('weight'))
    desc = f.pop('description')
    for kk in ('pokemonScale', 'pokemonOffset', 'trainerScale', 'trainerOffset'):
        e[kk] = num(f.pop(kk))
    assert not f, f
    R[c]['pokedex'] = e
    dex_uses.append((c, desc))
assign('text', dex_uses, lambda s: 'g%sPokedexText' % s, lambda s: texts[s], sub='pokedex')
for c, r in R.items():
    if 'pokedex' in r:
        r['pokedex'].move_to_end('text', last=False)
        for kk in ('weight', 'height', 'category'):
            r['pokedex'].move_to_end(kk, last=False)

# Pokedex sort orders: speciesgen derives them, check it can
ord_h = read('src/data/pokemon/pokedex_orders.h')
dex_orders = {}
for name in ('gPokedexOrder_Alphabetical', 'gPokedexOrder_Weight', 'gPokedexOrder_Height'):
    dex_orders[name] = [national[x] for x in split_top(array_body(ord_h, name))]

# ---------------------------------------------------------------- cries
cry_h = read('src/data/pokemon/cry_ids.h')
for k, v in designated(array_body(cry_h, 'gSpeciesIdToCryId')):
    c = sp_key(k)[0]
    R[c]['cryId'] = num(v)

# ---------------------------------------------------------------- animation ids
for name, field in (('sMonFrontAnimIdsTable', 'frontAnim'), ('sMonAnimationDelayTable', 'frontAnimDelay')):
    for k, v in designated(array_body(pokemon_c, name)):
        c, off = sp_key(k)
        assert off == 1
        R[c].setdefault('graphics', OrderedDict())[field] = unprefix(v, 'ANIM_') if field == 'frontAnim' else num(v)
anim_c = read('src/pokemon_animation.c')
for k, v in designated(array_body(anim_c, 'sSpeciesToBackAnimSet')):
    c = sp_key(k)[0]
    R[c].setdefault('graphics', OrderedDict())['backAnim'] = unprefix(v, 'BACK_ANIM_')

# ---------------------------------------------------------------- front pic anim frames
frames = {}
frame_order = []
for m in re.finditer(r'static const union AnimCmd (sAnim_\w+)\[\]\s*=\s*\{', anims_h):
    body = anims_h[m.end():matching(anims_h, m.end() - 1)]
    cmds = []
    for part in split_top(body):
        mm = re.match(r'ANIMCMD_FRAME\(\s*(\d+)\s*,\s*(\d+)\s*\)$', part)
        if mm:
            cmds.append([int(mm.group(1)), int(mm.group(2))])
        else:
            assert part == 'ANIMCMD_END', part
    frames[m.group(1)] = cmds
    frame_order.append(m.group(1))
anim_tables = {}
for m in re.finditer(r'(SINGLE|DOUBLE)_ANIMATION\((\w+)\)', anims_h):
    n = 1 if m.group(1) == 'SINGLE' else 2
    anim_tables[m.group(2)] = ['sAnim_GeneralFrame0'] + ['sAnim_%s_%d' % (m.group(2), i) for i in range(1, n + 1)]
for m in re.finditer(r'static const union AnimCmd \*const sAnims_(\w+)\[\]\s*=\s*\{', anims_h):
    body = anims_h[m.end():matching(anims_h, m.end() - 1)]
    anim_tables[m.group(1)] = split_top(body)
anim_owner = {}
for c, stem in anim_uses:
    anim_owner.setdefault(stem, c)
    if R[c]['symbol'] == stem:
        anim_owner[stem] = c
for c, stem in anim_uses:
    if anim_owner[stem] != c:
        R[c].setdefault('graphics', OrderedDict())['frontAnimFrames'] = '@' + anim_owner[stem]
for c, r in R.items():
    if r['symbol'] not in anim_tables:
        continue
    t = anim_tables[r['symbol']]
    want = ['sAnim_%s_%d' % (r['symbol'], i) for i in range(0, len(t))]
    g = r.setdefault('graphics', OrderedDict())
    if t[0] == 'sAnim_GeneralFrame0':
        t[0] = None
        want[0] = None
    else:
        # its own frame 0 animation instead of the shared one (Castform)
        g['frontAnimFrame0'] = True
    assert t == want, (c, t)
    g['frontAnimFrames'] = [frames[x] for x in t if x]

# ---------------------------------------------------------------- graphics tables
data_c = read('src/data.c')
gfx_tables = [
    ('frontPic', 'src/data/pokemon_graphics/front_pic_table.h', 'gMonFrontPicTable', 'SPECIES_SPRITE', 'gMonFrontPic_'),
    ('stillFrontPic', 'src/data/pokemon_graphics/still_front_pic_table.h', 'gMonStillFrontPicTable', 'SPECIES_SPRITE', 'gMonStillFrontPic_'),
    ('backPic', 'src/data/pokemon_graphics/back_pic_table.h', 'gMonBackPicTable', 'SPECIES_SPRITE', 'gMonBackPic_'),
    ('palette', 'src/data/pokemon_graphics/palette_table.h', 'gMonPaletteTable', 'SPECIES_PAL', 'gMonPalette_'),
    ('shinyPalette', 'src/data/pokemon_graphics/shiny_palette_table.h', 'gMonShinyPaletteTable', 'SPECIES_SHINY_PAL', 'gMonShinyPalette_'),
]
# Every INCBIN of a mon graphic, wherever it is defined
incbins = OrderedDict()
for path in ('src/anim_mon_front_pics.c', 'src/data/graphics/pokemon.h', 'src/graphics.c'):
    for m in re.finditer(r'const (u8|u16|u32) (gMon\w+)\[\]\s*=\s*INCBIN_U(8|16|32)\(\s*"([^"]+)"\s*\)', read(path)):
        incbins[m.group(2)] = (m.group(4), path)


def gfx_value(sym):
    return incbins[sym][0]


def gfx_derive(prefix):
    return lambda s: prefix + s


for field, path, table, macro, prefix in gfx_tables:
    uses = []
    for part in split_top(array_body(read(path), table)):
        m = re.match(r'%s\(\s*(\w+)\s*,\s*(\w+)\s*\)$' % macro, part)
        uses.append((m.group(1), m.group(2)))
    assign(field, uses, gfx_derive(prefix), gfx_value, sub='graphics')

icon_c = read('src/pokemon_icon.c')
uses = [(sp_key(k)[0], v) for k, v in designated(array_body(icon_c, 'gMonIconTable'))]
assign('icon', uses, gfx_derive('gMonIcon_'), gfx_value, sub='graphics')
for k, v in designated(array_body(icon_c, 'gMonIconPaletteIndices')):
    R[sp_key(k)[0]]['graphics']['iconPalette'] = num(v)
foot_h = read('src/data/pokemon_graphics/footprint_table.h')
uses = [(sp_key(k)[0], v) for k, v in designated(array_body(foot_h, 'gMonFootprintTable'))]
assign('footprint', uses, gfx_derive('gMonFootprint_'), gfx_value, sub='graphics')

for field, path, table in (('frontCoords', 'src/data/pokemon_graphics/front_pic_coordinates.h', 'gMonFrontPicCoords'),
                           ('backCoords', 'src/data/pokemon_graphics/back_pic_coordinates.h', 'gMonBackPicCoords')):
    for k, v in designated(array_body(read(path), table)):
        f = fields(v)
        m = re.match(r'MON_COORDS_SIZE\(\s*(\d+)\s*,\s*(\d+)\s*\)$', f['size'])
        R[sp_key(k)[0]]['graphics'][field] = [int(m.group(1)), int(m.group(2)), num(f['y_offset'])]
for k, v in designated(array_body(read('src/data/pokemon_graphics/enemy_mon_elevation.h'), 'gEnemyMonElevation')):
    R[sp_key(k)[0]]['graphics']['elevation'] = num(v)

# ---------------------------------------------------------------- tidy + write
# Field order in each record
ORDER = ['species', 'name', 'symbol', 'nationalDex', 'hoennDex', 'baseStats', 'types', 'catchRate',
         'expYield', 'evYield', 'heldItems', 'genderRatio', 'eggCycles', 'friendship', 'growthRate',
         'eggGroups', 'abilities', 'safariZoneFleeRate', 'bodyColor', 'noFlip', 'info', 'cryId',
         'evolutions', 'levelUpMoves', 'tmhm', 'tutor', 'eggMoves', 'pokedex', 'graphics']
GFX_ORDER = ['frontPic', 'stillFrontPic', 'backPic', 'palette', 'shinyPalette', 'icon', 'iconPalette',
             'footprint', 'frontCoords', 'backCoords', 'elevation', 'frontAnimFrame0', 'frontAnimFrames', 'frontAnim',
             'frontAnimDelay', 'backAnim']

outdir = sys.argv[1]
os.makedirs(outdir, exist_ok=True)


def filename(c):
    return c.lower() + '.json'


for c, r in R.items():
    out = OrderedDict()
    for k in ORDER:
        if k in r:
            out[k] = r[k]
    if out['symbol'] == ''.join(w.capitalize() for w in c.split('_')):
        del out['symbol']      # the default
    extra = set(r) - set(ORDER)
    assert not extra, (c, extra)
    if 'graphics' in out:
        g = out['graphics']
        out['graphics'] = OrderedDict((k, g[k]) for k in GFX_ORDER if k in g)
        assert set(g) <= set(GFX_ORDER), set(g) - set(GFX_ORDER)
    with open(os.path.join(outdir, filename(c)), 'w', encoding='utf-8') as f:
        f.write(dump(out) + '\n')

with open(os.path.join(outdir, 'species.txt'), 'w') as f:
    f.write('# Species in id order: SPECIES_<NAME> = line number (comments and blank\n'
            '# lines do not count). Each name has a record <name>.json next to this file.\n'
            '# NUM_SPECIES is the id of EGG; the entries after EGG are alternate forms.\n')
    for c in consts:
        f.write(c.lower() + '\n')

# Optional second argument: facts about the old tables (orders, INCBIN paths)
# for checking the generated output against them
with open(sys.argv[2] if len(sys.argv) > 2 else os.devnull, 'w') as f:
    json.dump({'dex_orders': dex_orders, 'egg_order': egg_order, 'text_order': text_order,
               'frame_order': frame_order, 'incbins': list(incbins.items()), 'num_species': NUM}, f, indent=1)
print('wrote %d records' % len(R))
