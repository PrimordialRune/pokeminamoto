"""Indexed GBA PNG -> transparent RGBA PNG, with palette resolution like the game does.

PIL is optional: without it `available()` is False and the server answers 501.
Cache: build/balance/sprites/<kind>/<key>@<scale>.png, invalidated by source mtimes.
"""
import io
import json
import re
from pathlib import Path

try:
    from PIL import Image
except Exception:  # pragma: no cover
    Image = None

REPO = Path(__file__).resolve().parents[4]
CACHE = REPO / "build" / "balance" / "sprites"
_DEFAULT_TYPE_PAL = {"NORMAL": 13, "FIGHTING": 13, "FLYING": 14, "POISON": 14, "GROUND": 13, "ROCK": 13,
                     "BUG": 15, "GHOST": 14, "STEEL": 13, "MYSTERY": 15, "FIRE": 13, "WATER": 14, "GRASS": 15,
                     "ELECTRIC": 13, "PSYCHIC": 14, "ICE": 14, "DRAGON": 15, "DARK": 13}


class SpriteError(Exception):
    def __init__(self, msg, status=404):
        super().__init__(msg)
        self.status = status


def available():
    return Image is not None


# ---- palettes -------------------------------------------------------------------------

def read_pal(path):
    """JASC .pal or binary 15-bit .gbapal -> list of (r,g,b)."""
    path = Path(path)
    if path.suffix == ".pal":
        lines = path.read_text().split()
        n = int(lines[2])
        v = list(map(int, lines[3:3 + 3 * n]))
        return [tuple(v[i:i + 3]) for i in range(0, len(v), 3)]
    b = path.read_bytes()
    out = []
    for i in range(0, len(b) - 1, 2):
        c = b[i] | (b[i + 1] << 8)
        out.append(((c & 31) * 255 // 31, ((c >> 5) & 31) * 255 // 31, ((c >> 10) & 31) * 255 // 31))
    return out


def first_existing(*paths):
    for p in paths:
        if p and Path(p).exists():
            return Path(p)
    return None


def convert(png, palette_file=None, crop=None, scale=1):
    """Indexed PNG + optional palette file -> RGBA PIL image (index 0 transparent)."""
    if Image is None:
        raise SpriteError("Pillow not installed", 501)
    im = Image.open(png)
    if im.mode == "P":
        if palette_file:
            pal = read_pal(palette_file)
        else:
            flat = im.getpalette() or []
            pal = [tuple(flat[i:i + 3]) for i in range(0, len(flat), 3)]
        pal = (list(pal) + [(0, 0, 0)] * 256)[:256]
        w, h = im.size
        idx = im.tobytes()
        rgba = bytearray(w * h * 4)
        for i, p in enumerate(idx):
            if p == 0:
                continue
            r, g, b = pal[p]
            o = i * 4
            rgba[o] = r
            rgba[o + 1] = g
            rgba[o + 2] = b
            rgba[o + 3] = 255
        im = Image.frombytes("RGBA", (w, h), bytes(rgba))
    else:
        im = im.convert("RGBA")  # true colour: pass through
    if crop:
        im = im.crop(crop)
    if scale and scale > 1:
        im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    return im


# ---- lookups --------------------------------------------------------------------------

_sp_cache = {"key": None, "map": {}}


def _norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def _species_map(repo):
    d = Path(repo) / "src/data/species"
    key = d.stat().st_mtime if d.exists() else 0
    if _sp_cache["key"] == (str(repo), key):
        return _sp_cache["map"]
    m = {}
    for p in d.glob("*.json"):
        try:
            j = json.loads(p.read_text(encoding="utf-8"))
        except Exception:
            continue
        g = j.get("graphics", {})
        ref = g.get("stillFrontPic") or g.get("frontPic") or g.get("icon")
        if isinstance(ref, dict):
            ref = ref.get("data")
        folder = Path(ref).parent.name if ref else p.stem
        info = {"folder": folder, "icon_palette": g.get("iconPalette", 0), "const": j.get("species", p.stem.upper())}
        for k in (j.get("species", ""), p.stem, folder):
            if k:
                m[_norm(k)] = info
    _sp_cache.update(key=(str(repo), key), map=m)
    return m


def species_info(name, repo=REPO):
    n = _norm(re.sub(r"^SPECIES_", "", name, flags=re.I))
    info = _species_map(repo).get(n)
    if not info:
        raise SpriteError("unknown species " + name)
    return info


def _item_map(repo):
    r = Path(repo)
    gfx = (r / "src/data/graphics/items.h").read_text(errors="replace")
    icons = dict(re.findall(r"(gItemIcon_\w+)\[\]\s*=\s*INCBIN_U32\(\"graphics/items/icons/(\w+)\.4bpp", gfx))
    pals = dict(re.findall(r"(gItemIconPalette_\w+)\[\]\s*=\s*INCBIN_U32\(\"graphics/items/icon_palettes/(\w+)\.gbapal", gfx))
    tab = (r / "src/data/item_icon_table.h").read_text(errors="replace")
    out = {}
    for c, i, p in re.findall(r"\[ITEM_(\w+)\]\s*=\s*\{(gItemIcon_\w+),\s*(gItemIconPalette_\w+)\}", tab):
        if i in icons and p in pals:
            out[c] = (icons[i], pals[p])
    try:  # TM_TOXIC is declared with .itemId = ITEM_TM06: use TM06's icon
        txt = (r / "src/data/items.h").read_text(errors="replace")
        for a, b in re.findall(r"\[ITEM_(\w+)\]\s*=\s*\{[^}]*?\.itemId\s*=\s*ITEM_(\w+)", txt):
            if a != b and b in out and a not in out:
                out[a] = out[b]
    except OSError:
        pass
    return out


def _type_pal_map(repo):
    try:
        t = (Path(repo) / "src/pokemon_summary_screen.c").read_text(errors="replace")
        blk = t[t.index("sMoveTypeToOamPaletteNum"):]
        blk = blk[:blk.index("};")]
        m = {k: int(v) for k, v in re.findall(r"\[TYPE_(\w+)\]\s*=\s*(\d+)", blk)}
        return m or _DEFAULT_TYPE_PAL
    except Exception:
        return _DEFAULT_TYPE_PAL


def resolve(kind, name, repo=REPO, variant=None, shiny=False):
    """-> (png path, palette file or None, crop or None)."""
    r = Path(repo)
    if kind == "pokemon":
        info = species_info(name, r)
        d = r / "graphics/pokemon" / info["folder"]
        v = variant or "front"
        if v not in ("front", "back", "icon"):
            raise SpriteError("variant must be front, back or icon")
        png = d / (v + ".png")
        if v == "icon":
            pal = first_existing(r / ("graphics/pokemon/icon_palettes/icon_palette_%d.pal" % info["icon_palette"]),
                                 r / ("graphics/pokemon/icon_palettes/icon_palette_%d.gbapal" % info["icon_palette"]))
            crop = (0, 0, 32, 32)
        else:
            pal = first_existing(d / ("shiny.pal" if shiny else "normal.pal"),
                                 d / ("shiny.gbapal" if shiny else "normal.gbapal"))
            crop = None
        return png, pal, crop
    if kind == "item":
        key = re.sub(r"^ITEM_", "", name.upper())
        m = _item_map(r)
        if key not in m:
            alt = key.lower()
            png = r / "graphics/items/icons" / (alt + ".png")
            if png.exists():
                return png, first_existing(r / "graphics/items/icon_palettes" / (alt + ".pal")), None
            raise SpriteError("unknown item " + name)
        ic, pl = m[key]
        pal = first_existing(r / "graphics/items/icon_palettes" / (pl + ".pal"),
                             r / "graphics/items/icon_palettes" / (pl + ".gbapal"))
        return r / "graphics/items/icons" / (ic + ".png"), pal, None
    if kind == "trainer":
        slug = re.sub(r"[^a-z0-9]+", "_", name.lower()).strip("_")
        d = r / "graphics/trainers/front_pics"
        png = d / (slug + ".png")
        if not png.exists():
            raise SpriteError("unknown trainer pic " + name)
        return png, first_existing(d / (slug + ".pal"), d / (slug + ".gbapal")), None
    if kind == "type":
        t = re.sub(r"^TYPE_", "", name.upper())
        t = {"FIGHT": "FIGHTING"}.get(t, t)
        png = r / "graphics/types" / (("fight" if t == "FIGHTING" else t.lower()) + ".png")
        if not png.exists():
            raise SpriteError("unknown type " + name)
        n = _type_pal_map(r).get(t, 13) - 12
        return png, first_existing(r / ("graphics/types/move_types_%d.pal" % n)), None
    raise SpriteError("unknown sprite kind " + kind)


def render_png(kind, name, variant=None, scale=1, shiny=False, repo=REPO, use_cache=True):
    """-> PNG bytes. Raises SpriteError."""
    if Image is None:
        raise SpriteError("Pillow not installed", 501)
    try:
        scale = max(1, min(int(scale or 1), 8))
    except ValueError:
        scale = 1
    png, pal, crop = resolve(kind, name, repo, variant, shiny)
    if not png.exists():
        raise SpriteError("missing file " + png.name)
    stamp = max(p.stat().st_mtime_ns for p in [png] + ([pal] if pal else []))
    key = "%s_%s%s@%d.png" % (name.upper(), variant or "", "_shiny" if shiny else "", scale)
    cpath = Path(repo) / "build/balance/sprites" / kind / re.sub(r"[^\w.@-]", "_", key)
    if use_cache and cpath.exists() and cpath.stat().st_mtime_ns >= stamp:
        return cpath.read_bytes()
    buf = io.BytesIO()
    convert(png, pal, crop, scale).save(buf, "PNG", optimize=True)
    data = buf.getvalue()
    if use_cache:
        try:
            cpath.parent.mkdir(parents=True, exist_ok=True)
            cpath.write_bytes(data)
        except OSError:
            pass
    return data


def manifest(repo=REPO):
    """What exists: lets the GUI avoid 404s."""
    r = Path(repo)
    pk = {}
    for k, info in _species_map(r).items():
        if k != _norm(info["const"]) or info["const"] in ("NONE", "EGG"):
            continue
        d = r / "graphics/pokemon" / info["folder"]
        pk[info["const"]] = [v for v in ("front", "back", "icon") if (d / (v + ".png")).exists()]
    try:
        items = sorted(c for c, (ic, _p) in _item_map(r).items() if (r / "graphics/items/icons" / (ic + ".png")).exists())
    except OSError:
        items = []
    tr = sorted(p.stem for p in (r / "graphics/trainers/front_pics").glob("*.png"))
    ty = sorted(("FIGHTING" if p.stem == "fight" else p.stem.upper()) for p in (r / "graphics/types").glob("*.png") if not p.stem.startswith("contest"))
    return {"pil": Image is not None, "pokemon": pk, "item": items, "trainer": tr, "type": ty,
            "url": {"pokemon": "/sprites/pokemon/<SPECIES|folder>/<front|back|icon>.png[?scale=2..4&shiny=1]",
                    "item": "/sprites/item/<ITEM>.png", "trainer": "/sprites/trainer/<pic_slug>.png",
                    "type": "/sprites/type/<TYPE>.png"}}
