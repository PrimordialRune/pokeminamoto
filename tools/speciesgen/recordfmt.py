"""The canonical text layout of a species record (used by extract.py and speciesgen.py --format)."""
import json


def dump(obj, indent=0):
    """JSON with short lists (moves, frames, numbers) on one line."""
    pad = '    ' * indent
    if isinstance(obj, dict):
        if not obj:
            return '{}'
        if all(not isinstance(v, (dict, list)) for v in obj.values()) and len(obj) <= 6 and indent > 0:
            return '{' + ', '.join('%s: %s' % (json.dumps(k), json.dumps(v, ensure_ascii=False)) for k, v in obj.items()) + '}'
        items = ['%s    %s: %s' % (pad, json.dumps(k), dump(v, indent + 1)) for k, v in obj.items()]
        return '{\n' + ',\n'.join(items) + '\n' + pad + '}'
    if isinstance(obj, list):
        if not obj:
            return '[]'
        flat = '[' + ', '.join(dump(v, indent + 1) for v in obj) + ']'
        if len(flat) + len(pad) <= 100 and '\n' not in flat:
            return flat
        return '[\n' + ',\n'.join(pad + '    ' + dump(v, indent + 1) for v in obj) + '\n' + pad + ']'
    return json.dumps(obj, ensure_ascii=False)


# Key order of a record and of its "graphics" object; unknown keys go last
KEYS = ['species', 'name', 'symbol', 'nationalDex', 'dex', 'baseStats', 'types', 'catchRate',
        'expYield', 'evYield', 'heldItems', 'genderRatio', 'eggCycles', 'friendship', 'growthRate',
        'eggGroups', 'abilities', 'safariZoneFleeRate', 'bodyColor', 'noFlip', 'info', 'cry',
        'evolutions', 'levelUpMoves', 'tmhm', 'tutor', 'eggMoves', 'pokedex', 'graphics']
GFX_KEYS = ['frontPic', 'stillFrontPic', 'backPic', 'palette', 'shinyPalette', 'icon', 'iconPalette',
            'footprint', 'frontCoords', 'backCoords', 'elevation', 'frontAnimFrame0', 'frontAnimFrames',
            'frontAnim', 'frontAnimDelay', 'backAnim']


def ordered(d, keys):
    rank = {k: i for i, k in enumerate(keys)}
    return dict(sorted(d.items(), key=lambda kv: rank.get(kv[0], len(keys))))


def canonical(rec):
    """The record as speciesgen --format writes it."""
    rec = ordered({k: v for k, v in rec.items() if not k.startswith('_')}, KEYS)
    if isinstance(rec.get('graphics'), dict):
        rec['graphics'] = ordered(rec['graphics'], GFX_KEYS)
    return dump(rec) + '\n'
