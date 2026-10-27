#!/usr/bin/env python3
"""Import FireRed/LeafGreen maps, layouts and tilesets from a pokefirered checkout.

    python3 tools/frlg_import/import_maps.py /path/to/pokefirered

What it brings over (mainland Kanto only; Sevii Islands, Trainer Tower, Navel
Rock, Birth Island, Mt. Ember, link rooms and prototypes are skipped):

  * every map's layout, border, warps, connections, NPC placement and signs.
    Scripts, trainers, flags, items and coord triggers are not imported: NPCs
    and signs get no script, so they are placeholders.
  * the tilesets those maps use, converted to this engine's split.

Tileset conversion
------------------
FRLG splits tiles/metatiles 640 primary + 384 secondary with 7 + 6 BG
palettes; this engine (Emerald) splits 512 + 512 with 6 + 8. The converter
moves the top of FRLG's primary (tiles and metatiles 512-639, palette 6) into
every secondary that is used with it:

    primary   = FRLG primary tiles/metatiles 0-511, palettes 0-5
    secondary = FRLG primary tiles/metatiles 512-639 + FRLG secondary,
                palette 6 from the FRLG primary, 7-12 from the FRLG secondary

So every tile lands at the same VRAM index it had in FRLG, metatile ids keep
their values and map.bin files are copied byte for byte. The cost is about
6 KB of ROM per secondary (the duplicated primary tail).

Metatile attributes go from FRLG's u32 (behavior, terrain, encounter type,
layer type) to Emerald's u16 (behavior, layer type). Behaviors map by name;
FRLG-only ones use the table below. Floor tiles that only had FRLG's "land
encounter" flag (cave floors) become MB_INDOOR_ENCOUNTER.

Object event graphics are renamed OBJ_EVENT_GFX_FRLG_*; import_sprites.py
adds them. Rerunning this script overwrites what it generated before.
"""

import json
import os
import re
import shutil
import sys

from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(__file__), '..', '..'))

# Map groups and maps that are not mainland Kanto.
SKIP_GROUPS = {'gMapGroup_Link', 'gMapGroup_SpecialArea'}
SKIP_GROUP_RE = re.compile(r'Island(Route)?$')  # gMapGroup_IndoorOneIsland... (not Cinnabar)
SKIP_MAP_RE = re.compile(r'^((One|Two|Three|Four|Five|Six|Seven)Island|Prototype|MtEmber|TrainerTower|NavelRock|BirthIsland)')
RENAME_GROUPS = {
    'gMapGroup_TownsAndRoutes': 'gMapGroup_KantoTownsAndRoutes',
    'gMapGroup_Dungeons': 'gMapGroup_KantoDungeons',
}

FRLG_PRIMARY_TILES = 640
FRLG_PRIMARY_PALS = 7
EM_PRIMARY_TILES = 512
EM_PRIMARY_PALS = 6

# FRLG behaviors with no same-named Emerald behavior.
BEHAVIOR_MAP = {
    'MB_UNUSED_01': 'MB_NORMAL',
    'MB_RUNNING_DISALLOWED': 'MB_NO_RUNNING',
    'MB_FAST_WATER': 'MB_DEEP_WATER',
    'MB_UNDERWATER_BLOCKED_ABOVE': 'MB_DEEP_WATER',
    'MB_UNUSED_WATER': 'MB_POND_WATER',
    'MB_CYCLING_ROAD_WATER': 'MB_POND_WATER',
    'MB_STRENGTH_BUTTON': 'MB_NORMAL',
    'MB_ROCK_STAIRS': 'MB_NORMAL',
    'MB_SAND_CAVE': 'MB_SAND',
    'MB_SPIN_RIGHT': 'MB_SLIDE_EAST',
    'MB_SPIN_LEFT': 'MB_SLIDE_WEST',
    'MB_SPIN_UP': 'MB_SLIDE_NORTH',
    'MB_SPIN_DOWN': 'MB_SLIDE_SOUTH',
    'MB_STOP_SPINNING': 'MB_NORMAL',
    'MB_CAVE_DOOR': 'MB_NON_ANIMATED_DOOR',
    'MB_FALL_WARP': 'MB_NORMAL',
    'MB_REGULAR_WARP': 'MB_AQUA_HIDEOUT_WARP',
    'MB_LAVARIDGE_1F_WARP': 'MB_LAVARIDGE_GYM_1F_WARP',
    'MB_WARP_DOOR': 'MB_ANIMATED_DOOR',
    'MB_UP_RIGHT_STAIR_WARP': 'MB_EAST_ARROW_WARP',
    'MB_UP_LEFT_STAIR_WARP': 'MB_WEST_ARROW_WARP',
    'MB_DOWN_RIGHT_STAIR_WARP': 'MB_EAST_ARROW_WARP',
    'MB_DOWN_LEFT_STAIR_WARP': 'MB_WEST_ARROW_WARP',
    'MB_UNION_ROOM_WARP': 'MB_NORMAL',
    'MB_POKEMART_SHELF': 'MB_SHOP_SHELF',
    'MB_TRASH_BIN': 'MB_TRASH_CAN',
    'MB_CYCLING_ROAD_PULL_DOWN': 'MB_NORMAL',
    'MB_CYCLING_ROAD_PULL_DOWN_GRASS': 'MB_NORMAL',
}  # anything else missing (signs, cabinets, posters...) -> MB_NORMAL

MOVEMENT_MAP = {'MOVEMENT_TYPE_WANDER_AROUND_SLOWER': 'MOVEMENT_TYPE_WANDER_AROUND'}
BATTLE_SCENES = {'MAP_BATTLE_SCENE_NORMAL', 'MAP_BATTLE_SCENE_GYM'}

TILESET_HEADER_BEGIN = '// FRLG import begin (tools/frlg_import/import_maps.py)\n'
TILESET_HEADER_END = '// FRLG import end\n'


def read_behaviors(path):
    vals = {}
    text = open(path).read()
    if re.search(r'^enum', text, re.M):
        i = 0
        for line in text[text.index('enum'):].splitlines():
            m = re.match(r'\s*(MB_\w+)\s*(?:=\s*(0x[0-9A-Fa-f]+|\d+))?\s*,', line)
            if m:
                if m.group(2):
                    i = int(m.group(2), 0)
                vals[m.group(1)] = i
                i += 1
    else:
        for m in re.finditer(r'#define\s+(MB_\w+)\s+(0x[0-9A-Fa-f]+|\d+)', text):
            vals[m.group(1)] = int(m.group(2), 0)
    return vals


def snake(name):
    return re.sub(r'(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])', '_', name).lower()


class FrlgTileset:
    def __init__(self, frlg, symbol):
        headers = open(os.path.join(frlg, 'src/data/tilesets/headers.h')).read()
        m = re.search(r'const struct Tileset ' + symbol + r' =\s*\{(.*?)\};', headers, re.S)
        body = m.group(1)
        field = lambda f: re.search(r'\.' + f + r'\s*=\s*(\w+)', body).group(1)
        self.symbol = symbol
        self.secondary = field('isSecondary') == 'TRUE'
        self.callback = field('callback')
        graphics = open(os.path.join(frlg, 'src/data/tilesets/graphics.h')).read() + open(os.path.join(frlg, 'src/graphics.c')).read()
        metatiles = open(os.path.join(frlg, 'src/data/tilesets/metatiles.h')).read()
        tiles = re.search(field('tiles') + r'\[\] = INCBIN_U32\("([^"]+)\.4bpp\.lz"\)', graphics).group(1)
        self.tiles_png = os.path.join(frlg, tiles + '.png')
        self.dir = os.path.dirname(self.tiles_png)
        self.metatiles = os.path.join(frlg, re.search(field('metatiles') + r'\[\] = INCBIN_U16\("([^"]+)"\)', metatiles).group(1))
        self.attributes = os.path.join(frlg, re.search(field('metatileAttributes') + r'\[\] = INCBIN_U32\("([^"]+)"\)', metatiles).group(1))
        self.name = symbol[len('gTileset_'):]
        self.new_symbol = 'gTileset_Frlg' + self.name
        self.folder = os.path.dirname(self.metatiles)  # the tileset's own folder (tiles can be shared)
        self.new_dir = os.path.join('data/tilesets', 'secondary' if self.secondary else 'primary',
                                    'frlg_' + os.path.basename(self.folder))


class Importer:
    def __init__(self, frlg):
        self.frlg = frlg
        self.frlg_mb = {v: k for k, v in read_behaviors(os.path.join(frlg, 'include/constants/metatile_behaviors.h')).items()}
        self.em_mb = read_behaviors(os.path.join(ROOT, 'include/constants/metatile_behaviors.h'))
        self.songs = open(os.path.join(ROOT, 'include/constants/songs.h')).read()
        self.movements = open(os.path.join(ROOT, 'include/constants/event_object_movement.h')).read()
        self.unmapped_behaviors = set()

    # ---------------------------------------------------------------- tilesets

    def convert_attributes(self, raw):
        out = bytearray()
        for i in range(0, len(raw), 4):
            a = int.from_bytes(raw[i:i + 4], 'little')
            name = self.frlg_mb.get(a & 0x1FF, 'MB_NORMAL')
            name = BEHAVIOR_MAP.get(name, name)
            if name not in self.em_mb:
                self.unmapped_behaviors.add(name)
                name = 'MB_NORMAL'
            if name == 'MB_NORMAL' and (a >> 24) & 7 == 1:  # TILE_ENCOUNTER_LAND
                name = 'MB_INDOOR_ENCOUNTER'
            out += (self.em_mb[name] | (((a >> 29) & 3) << 12)).to_bytes(2, 'little')
        return bytes(out)

    def write_tileset(self, ts, primary=None):
        dest = os.path.join(ROOT, ts.new_dir)
        os.makedirs(os.path.join(dest, 'palettes'), exist_ok=True)
        img = Image.open(ts.tiles_png)
        assert img.mode in ('P', 'L'), ts.tiles_png
        metatiles = open(ts.metatiles, 'rb').read()
        attributes = self.convert_attributes(open(ts.attributes, 'rb').read())
        pals = [open(os.path.join(ts.dir, 'palettes', '%02d.pal' % i)).read() for i in range(16)]
        if not ts.secondary:
            rows = EM_PRIMARY_TILES // 16
            img = img.crop((0, 0, 128, rows * 8))
            metatiles = metatiles[:EM_PRIMARY_TILES * 16]
            attributes = attributes[:EM_PRIMARY_TILES * 2]
        else:
            p = primary
            pimg = Image.open(p.tiles_png)
            tail = pimg.crop((0, EM_PRIMARY_TILES // 16 * 8, 128, FRLG_PRIMARY_TILES // 16 * 8))
            # Pad the FRLG secondary to whole rows of 16 tiles.
            sec = img
            out = Image.new(pimg.mode, (128, tail.height + sec.height))
            if pimg.mode == 'P':
                out.putpalette(pimg.getpalette())
            out.paste(tail, (0, 0))
            out.paste(sec.convert(pimg.mode) if sec.mode != pimg.mode else sec, (0, tail.height))
            img = out
            pmeta = open(p.metatiles, 'rb').read()
            pattr = self.convert_attributes(open(p.attributes, 'rb').read())
            metatiles = pmeta[EM_PRIMARY_TILES * 16:FRLG_PRIMARY_TILES * 16] + metatiles
            attributes = pattr[EM_PRIMARY_TILES * 2:FRLG_PRIMARY_TILES * 2] + attributes
            ppals = [open(os.path.join(p.dir, 'palettes', '%02d.pal' % i)).read() for i in range(16)]
            for i in range(EM_PRIMARY_PALS, FRLG_PRIMARY_PALS):
                pals[i] = ppals[i]
        assert img.height // 8 * 16 <= 512, ts.symbol
        assert len(metatiles) // 16 <= 512, ts.symbol
        img.save(os.path.join(dest, 'tiles.png'), bits=4) if img.mode == 'P' else img.save(os.path.join(dest, 'tiles.png'))
        open(os.path.join(dest, 'metatiles.bin'), 'wb').write(metatiles)
        open(os.path.join(dest, 'metatile_attributes.bin'), 'wb').write(attributes)
        for i, pal in enumerate(pals):
            open(os.path.join(dest, 'palettes', '%02d.pal' % i), 'w', newline='\r\n').write(pal.replace('\r\n', '\n'))
        anim = os.path.join(ts.folder, 'anim')
        if os.path.isdir(anim):
            shutil.copytree(anim, os.path.join(dest, 'anim'), dirs_exist_ok=True)

    def tileset_sources(self, tilesets):
        graphics, metatiles, headers = [], [], []
        for ts in tilesets:
            n = 'Frlg' + ts.name
            d = ts.new_dir
            graphics.append('const u32 gTilesetTiles_%s[] = INCBIN_U32("%s/tiles.4bpp.lz");\n\n' % (n, d))
            graphics.append('const u16 gTilesetPalettes_%s[][16] =\n{\n' % n)
            graphics += ['\tINCBIN_U16("%s/palettes/%02d.gbapal"),\n' % (d, i) for i in range(16)]
            graphics.append('};\n\n')
            metatiles.append('const u16 gMetatiles_%s[] = INCBIN_U16("%s/metatiles.bin");\n' % (n, d))
            metatiles.append('const u16 gMetatileAttributes_%s[] = INCBIN_U16("%s/metatile_attributes.bin");\n\n' % (n, d))
            cb = 'InitTilesetAnim_' + n if ts.callback != 'NULL' else 'NULL'
            headers.append('const struct Tileset %s =\n{\n    .isCompressed = TRUE,\n    .isSecondary = %s,\n'
                           '    .tiles = gTilesetTiles_%s,\n    .palettes = gTilesetPalettes_%s,\n'
                           '    .metatiles = gMetatiles_%s,\n    .metatileAttributes = gMetatileAttributes_%s,\n'
                           '    .callback = %s,\n};\n\n' % (ts.new_symbol, 'TRUE' if ts.secondary else 'FALSE', n, n, n, n, cb))
        for path, chunk in (('src/data/tilesets/graphics.h', graphics), ('src/data/tilesets/metatiles.h', metatiles),
                            ('src/data/tilesets/headers.h', headers)):
            replace_block(os.path.join(ROOT, path), ''.join(chunk))

    # -------------------------------------------------------------------- maps

    def music(self, mus):
        rg = mus.replace('MUS_', 'MUS_RG_', 1)
        if re.search(r'\b%s\b' % rg, self.songs):
            return rg
        if re.search(r'\b%s\b' % mus, self.songs):
            return mus
        return 'MUS_DUMMY'

    def movement(self, mv):
        mv = MOVEMENT_MAP.get(mv, mv)
        return mv if re.search(r'\b%s\b' % mv, self.movements) else 'MOVEMENT_TYPE_FACE_DOWN'

    def run(self):
        frlg = self.frlg
        groups = json.load(open(os.path.join(frlg, 'data/maps/map_groups.json')))
        maps = {}  # name -> new group
        for g in groups['group_order']:
            if g in SKIP_GROUPS or SKIP_GROUP_RE.search(g):
                continue
            for m in groups[g]:
                if not SKIP_MAP_RE.match(m):
                    maps[m] = RENAME_GROUPS.get(g, g)
        jsons = {m: json.load(open(os.path.join(frlg, 'data/maps', m, 'map.json'))) for m in maps}
        map_ids = {j['id'] for j in jsons.values()}

        # Existing (non-FRLG) maps and layouts must not collide.
        our_groups = json.load(open(os.path.join(ROOT, 'data/maps/map_groups.json')))
        frlg_group_names = {RENAME_GROUPS.get(g, g) for g in groups['group_order']}
        our_groups['group_order'] = [g for g in our_groups['group_order'] if g not in frlg_group_names]
        for g in list(our_groups):
            if g != 'group_order' and g not in our_groups['group_order']:
                del our_groups[g]
        ours = {m for g in our_groups['group_order'] for m in our_groups[g]}
        clash = ours & set(maps)
        assert not clash, clash

        layouts = json.load(open(os.path.join(frlg, 'data/layouts/layouts.json')))
        frlg_layouts = {l['id']: l for l in layouts['layouts'] if l}
        used_layouts = sorted({j['layout'] for j in jsons.values()}, key=list(frlg_layouts).index)
        our_layouts = json.load(open(os.path.join(ROOT, 'data/layouts/layouts.json')))
        # A rerun replaces the layouts it added before (listed in the manifest).
        manifest_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'imported_layouts.txt')
        previous = set(open(manifest_path).read().split()) if os.path.exists(manifest_path) else set()
        our_layouts['layouts'] = [l for l in our_layouts['layouts'] if l['id'] not in previous]
        # FRLG layouts whose id, name or folder clash with one of ours get a Frlg prefix.
        taken = {l['id'] for l in our_layouts['layouts']} | {l['name'] for l in our_layouts['layouts']} | \
                {os.path.dirname(l['blockdata_filepath']) for l in our_layouts['layouts']}
        layout_rename = {}
        for lid in used_layouts:
            l = frlg_layouts[lid]
            folder = os.path.dirname(l['blockdata_filepath'])
            if {lid, l['name'], folder} & taken:
                layout_rename[lid] = ('LAYOUT_FRLG_' + lid[len('LAYOUT_'):], 'Frlg' + l['name'],
                                      os.path.join(os.path.dirname(folder), 'Frlg' + os.path.basename(folder)))
            else:
                layout_rename[lid] = (lid, l['name'], folder)

        # Tilesets
        prim_syms = sorted({frlg_layouts[l]['primary_tileset'] for l in used_layouts})
        sec_syms = sorted({frlg_layouts[l]['secondary_tileset'] for l in used_layouts})
        prims = {s: FrlgTileset(frlg, s) for s in prim_syms}
        secs = {}
        for l in used_layouts:
            p, s = frlg_layouts[l]['primary_tileset'], frlg_layouts[l]['secondary_tileset']
            if s in secs:
                assert secs[s][1] == p, 'secondary %s used with two primaries' % s
            else:
                secs[s] = (FrlgTileset(frlg, s), p)
        for ts in prims.values():
            self.write_tileset(ts)
        for ts, p in secs.values():
            self.write_tileset(ts, prims[p])
        self.tileset_sources(list(prims.values()) + [ts for ts, _ in secs.values()])
        write_tileset_anims(frlg, list(prims.values()) + [ts for ts, _ in secs.values()])
        ts_symbol = {s: t.new_symbol for s, t in prims.items()}
        ts_symbol.update({s: t.new_symbol for s, (t, _) in secs.items()})

        # Layouts
        for lid in used_layouts:
            l = frlg_layouts[lid]
            new_id, name, folder = layout_rename[lid]
            dest = os.path.join(ROOT, folder)
            os.makedirs(dest, exist_ok=True)
            shutil.copyfile(os.path.join(frlg, l['blockdata_filepath']), os.path.join(dest, 'map.bin'))
            border = open(os.path.join(frlg, l['border_filepath']), 'rb').read()
            bw = l.get('border_width', 2)
            if bw != 2:  # this engine only has 2x2 borders: keep the top-left 2x2
                border = border[0:4] + border[bw * 2:bw * 2 + 4]
            open(os.path.join(dest, 'border.bin'), 'wb').write(border[:8])
            our_layouts['layouts'].append({
                'id': new_id, 'name': name, 'width': l['width'], 'height': l['height'],
                'primary_tileset': ts_symbol[l['primary_tileset']],
                'secondary_tileset': ts_symbol[l['secondary_tileset']],
                'border_filepath': folder + '/border.bin', 'blockdata_filepath': folder + '/map.bin',
            })
        with open(os.path.join(ROOT, 'data/layouts/layouts.json'), 'w') as f:
            json.dump(our_layouts, f, indent=2)
            f.write('\n')
        open(manifest_path, 'w').write(''.join(layout_rename[l][0] + '\n' for l in used_layouts))

        # Maps
        for name, j in jsons.items():
            out = {
                'id': j['id'], 'name': name, 'layout': layout_rename[j['layout']][0], 'music': self.music(j['music']),
                'region_map_section': j['region_map_section'], 'requires_flash': j['requires_flash'],
                'weather': j['weather'], 'map_type': j['map_type'], 'allow_cycling': j['allow_cycling'],
                'allow_escaping': j['allow_escaping'], 'allow_running': j['allow_running'],
                'show_map_name': j['show_map_name'],
                'battle_scene': j['battle_scene'] if j['battle_scene'] in BATTLE_SCENES else 'MAP_BATTLE_SCENE_NORMAL',
                'connections': [c for c in (j.get('connections') or []) if c['map'] in map_ids] or None,
                'object_events': [], 'warp_events': [], 'coord_events': [], 'bg_events': [],
            }
            for o in j.get('object_events', []):
                gfx = o['graphics_id']
                if not gfx.startswith('OBJ_EVENT_GFX_VAR_'):  # var gfx stay dynamic
                    gfx = gfx.replace('OBJ_EVENT_GFX_', 'OBJ_EVENT_GFX_FRLG_', 1)
                if o['type'] == 'clone':
                    if o['target_map'] not in map_ids:
                        continue
                    out['object_events'].append({**({'local_id': o['local_id']} if 'local_id' in o else {}),
                                                 'type': 'clone', 'graphics_id': gfx, 'x': o['x'], 'y': o['y'],
                                                 'target_local_id': o['target_local_id'], 'target_map': o['target_map']})
                    continue
                local_id = {'local_id': o['local_id']} if 'local_id' in o else {}
                out['object_events'].append({
                    **local_id, 'type': 'object', 'graphics_id': gfx, 'x': o['x'], 'y': o['y'], 'elevation': o['elevation'],
                    'movement_type': self.movement(o['movement_type']),
                    'movement_range_x': o['movement_range_x'], 'movement_range_y': o['movement_range_y'],
                    'trainer_type': 'TRAINER_TYPE_NONE', 'trainer_sight_or_berry_tree_id': '0',
                    'script': '0', 'flag': '0',
                })
            for w in j.get('warp_events', []):
                dest = w['dest_map'] if w['dest_map'] in map_ids else 'MAP_DYNAMIC'
                out['warp_events'].append({'x': w['x'], 'y': w['y'], 'elevation': w['elevation'],
                                           'dest_map': dest, 'dest_warp_id': str(w['dest_warp_id'])})
            for b in j.get('bg_events', []):
                if b['type'] == 'sign':
                    out['bg_events'].append({'type': 'sign', 'x': b['x'], 'y': b['y'], 'elevation': b['elevation'],
                                             'player_facing_dir': b['player_facing_dir'], 'script': '0'})
            d = os.path.join(ROOT, 'data/maps', name)
            os.makedirs(d, exist_ok=True)
            with open(os.path.join(d, 'map.json'), 'w') as f:
                json.dump(out, f, indent=2)
                f.write('\n')
            with open(os.path.join(d, 'scripts.pory'), 'w') as f:
                f.write('// Placeholder imported from FireRed (tools/frlg_import). No events yet.\n')
                f.write('mapscripts %s_MapScripts {}\n' % name)

        # Map groups (FRLG groups go after ours so existing map ids keep their values)
        for g in groups['group_order']:
            ng = RENAME_GROUPS.get(g, g)
            members = [m for m in groups[g] if m in maps]
            if members:
                our_groups['group_order'].append(ng)
                our_groups[ng] = members
        with open(os.path.join(ROOT, 'data/maps/map_groups.json'), 'w') as f:
            json.dump(our_groups, f, indent=2)
            f.write('\n')

        # event_scripts.s includes
        path = os.path.join(ROOT, 'data/event_scripts.s')
        lines = [l for l in open(path).read().split('\n') if not l.startswith('\t.include "data/maps/') or
                 l.split('/')[2] not in maps]
        idx = max(i for i, l in enumerate(lines) if l.startswith('\t.include "data/maps/'))
        lines[idx + 1:idx + 1] = ['\t.include "data/maps/%s/scripts.inc"' % m for m in maps]
        open(path, 'w').write('\n'.join(lines))

        gfx = sorted({o['graphics_id'] for j in jsons.values() for o in j.get('object_events', [])})
        print('maps %d, layouts %d, primary tilesets %d, secondary tilesets %d' % (len(maps), len(used_layouts), len(prims), len(secs)))
        print('object gfx used: %d' % len(gfx))
        if self.unmapped_behaviors:
            print('behaviors mapped to MB_NORMAL:', ' '.join(sorted(self.unmapped_behaviors)))
        anims = [t for t in list(prims.values()) + [t for t, _ in secs.values()] if t.callback != 'NULL']
        print('tilesets with animations:', ' '.join(t.new_symbol for t in anims))


def write_tileset_anims(frlg, tilesets):
    """FRLG's tile animations, renamed Frlg*, into src/data/tileset_anims_frlg.h
    (included by src/tileset_anims.c). VRAM offsets are unchanged because the
    converted tilesets keep FRLG's VRAM layout."""
    src = open(os.path.join(frlg, 'src/tileset_anims.c')).read()
    keep = {t.name for t in tilesets if t.callback != 'NULL'}
    # Split into top-level chunks (blank-line separated) and keep the ones that
    # belong to an imported tileset's animation.
    chunks = re.split(r'\n(?=static |void |// )', src)
    out = []
    for c in chunks:
        names = set(re.findall(r'(?:sTilesetAnims|QueueAnimTiles|InitTilesetAnim|TilesetAnim)_([A-Z][A-Za-z]*?)(?=_|\(|\b)', c))
        if not names or not names <= keep:
            continue
        for n in keep:
            c = re.sub(r'\b(sTilesetAnims|QueueAnimTiles|InitTilesetAnim|TilesetAnim)_%s(?=_|\(|\b)' % n, r'\1_Frlg%s' % n, c)
        c = re.sub(r'data/tilesets/(primary|secondary)/(\w+)/', r'data/tilesets/\1/frlg_\2/', c)
        out.append(c.rstrip('\n') + '\n')
    text = ('// FireRed tile animations, generated by tools/frlg_import/import_maps.py.\n'
            '// Included by src/tileset_anims.c.\n\n')
    decls = sorted(set(re.findall(r'^static void (\w+)\(u16 timer\)', ''.join(out), re.M)))
    text += ''.join('static void %s(u16 timer);\n' % d for d in decls) + '\n'
    open(os.path.join(ROOT, 'src/data/tileset_anims_frlg.h'), 'w').write(text + '\n'.join(out))
    hdr = os.path.join(ROOT, 'include/tileset_anims.h')
    inits = ''.join('void InitTilesetAnim_Frlg%s(void);\n' % n for n in sorted(keep))
    replace_block(hdr, inits, before='#endif')


def replace_block(path, block, before=None):
    text = open(path).read()
    if TILESET_HEADER_BEGIN in text:
        text = text[:text.index(TILESET_HEADER_BEGIN)] + text[text.index(TILESET_HEADER_END) + len(TILESET_HEADER_END):]
    block = TILESET_HEADER_BEGIN + block + TILESET_HEADER_END
    if before:
        i = text.rindex(before)
        text = text[:i].rstrip('\n') + '\n\n' + block + '\n' + text[i:]
    else:
        text = text.rstrip('\n') + '\n\n' + block
    open(path, 'w').write(text)


if __name__ == '__main__':
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    Importer(os.path.abspath(sys.argv[1])).run()
