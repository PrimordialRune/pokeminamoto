"""Self-contained repo reader for the GUI's /api/data.

Stopgap: the analysis team's minasim.gamedata replaces it. The server calls
`minasim.gamedata.api_data(repo)` when that function exists (returns the same dict as
`load(repo)` here); otherwise this module is used. Stdlib only.
"""
import csv
import json
import re
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]

PHYSICAL = {"NORMAL", "FIGHTING", "FLYING", "POISON", "GROUND", "ROCK", "BUG", "GHOST", "STEEL"}


def norm(s):
    return re.sub(r"[^a-z0-9]", "", s.lower().replace("♀", "f").replace("♂", "m"))


def _read(p):
    return Path(p).read_text(encoding="utf-8", errors="replace")


def source_files(repo=REPO):
    r = Path(repo)
    files = [r / "src/data/moves_info.h", r / "src/data/items.h", r / "src/data/text/item_descriptions.h",
             r / "src/data/trainers.party", r / "src/data/species/species.txt",
             r / "docs/design/data/species.csv", r / "docs/design/data/overworld.csv",
             r / "docs/design/data/trainers.csv", r / "src/data/item_icon_table.h"]
    files += sorted((r / "src/data/species").glob("*.json"))
    return [f for f in files if f.exists()]


def _csv(repo, name):
    p = Path(repo) / "docs/design/data" / name
    if not p.exists():
        return []
    with p.open(encoding="utf-8") as f:
        return list(csv.DictReader(f))


def _blocks(text, prefix):
    """Yield (CONST, body) for `[PREFIX_X] =\n    { ... },` entries."""
    for m in re.finditer(r"\[(%s_\w+)\]\s*=\s*\{(.*?)\n    \},?" % prefix, text, re.S):
        yield m.group(1), m.group(2)


def _cstr(body, field):
    m = re.search(r"\.%s\s*=\s*_\(\s*((?:\"(?:[^\"\\]|\\.)*\"\s*)+)\)" % field, body)
    if not m:
        return None
    return re.sub(r"\s+", " ", "".join(re.findall(r"\"((?:[^\"\\]|\\.)*)\"", m.group(1))).replace("\\n", " ").replace("\\p", " ").replace("\\l", " ")).strip()


def _num(body, field, default=None):
    m = re.search(r"\.%s\s*=\s*(-?\d+)" % field, body)
    return int(m.group(1)) if m else default


def species(repo=REPO):
    r = Path(repo)
    sd = r / "src/data/species"
    order = [l.strip() for l in _read(sd / "species.txt").splitlines() if l.strip() and not l.startswith("#")]
    mina = {}
    for row in _csv(r, "species.csv"):
        loc = row.get("localized_name") or ""
        if loc:
            mina[norm(loc)] = {"minamoto": row["pocket_monster"], "habitat": row.get("habitat"),
                               "signature_move": row.get("signature_move"), "release": row.get("release")}
    out = []
    for name in order:
        if name in ("none",):
            continue
        if name == "egg":
            break
        p = sd / (name + ".json")
        if not p.exists():
            continue
        j = json.loads(_read(p))
        const = j["species"]
        g = j.get("graphics", {})
        ref = g.get("stillFrontPic") or g.get("frontPic") or "graphics/pokemon/%s/x" % name
        folder = Path(ref["data"] if isinstance(ref, dict) else ref).parent.name
        mn = mina.get(norm(const), {})
        bs = j.get("baseStats", {})
        out.append({
            "id": const, "index": len(out) + 1, "dex": j.get("nationalDex", j.get("dex")),
            "name": j.get("name", const), "minamoto": mn.get("minamoto"),
            "habitat": mn.get("habitat"), "signature_move": mn.get("signature_move"),
            "types": j.get("types", []), "base_stats": bs, "bst": sum(bs.values()),
            "learnset": {"level": [{"level": l, "move": m} for l, m in j.get("levelUpMoves", [])],
                         "tmhm": j.get("tmhm", []), "tutor": j.get("tutor", [])},
            "evolutions": j.get("evolutions", []),
            "growth_rate": j.get("growthRate"), "catch_rate": j.get("catchRate"),
            "exp_yield": j.get("expYield"), "folder": folder,
            "icon_palette": g.get("iconPalette", 0),
        })
    return out


def moves(repo=REPO):
    mi = _read(Path(repo) / "src/data/moves_info.h")
    descs = {}
    for m in re.finditer(r"static const u8 (\w+)\[\]\s*=\s*_\(\s*((?:\"(?:[^\"\\]|\\.)*\"\s*)+)\)", mi):
        descs[m.group(1)] = re.sub(r"\s+", " ", " ".join(x.replace("\\n", " ") for x in re.findall(r"\"((?:[^\"\\]|\\.)*)\"", m.group(2)))).strip()
    out = []
    for const, body in _blocks(mi, "MOVE"):
        if const == "MOVE_NONE":
            continue
        t = re.search(r"\.type\s*=\s*TYPE_(\w+)", body)
        typ = t.group(1) if t else "NORMAL"
        power = _num(body, "power", 0)
        d = re.search(r"\.description\s*=\s*(\w+)", body)
        out.append({
            "id": const[5:], "name": _cstr(body, "name") or const[5:], "type": typ, "power": power,
            "accuracy": _num(body, "accuracy", 0), "tp_cost": _num(body, "tpCost", 0),
            "category": "status" if power == 0 else ("physical" if typ in PHYSICAL else "special"),
            "priority": _num(body, "priority", 0),
            "effect": (re.search(r"\.effect\s*=\s*(\w+)", body) or [None, None])[1],
            "description": descs.get(d.group(1), "") if d else "",
        })
    return out


def items(repo=REPO):
    r = Path(repo)
    txt = _read(r / "src/data/items.h")
    descs = {}
    dp = r / "src/data/text/item_descriptions.h"
    if dp.exists():
        for m in re.finditer(r"static const u8 (\w+)\[\]\s*=\s*_\(\s*((?:\"(?:[^\"\\]|\\.)*\"\s*)+)\)", _read(dp)):
            descs[m.group(1)] = re.sub(r"\s+", " ", " ".join(x.replace("\\n", " ") for x in re.findall(r"\"((?:[^\"\\]|\\.)*)\"", m.group(2)))).strip()
    out = []
    for const, body in _blocks(txt, "ITEM"):
        if const == "ITEM_NONE":
            continue
        d = re.search(r"\.description\s*=\s*(\w+)", body)
        pk = re.search(r"\.pocket\s*=\s*POCKET_(\w+)", body)
        out.append({"id": const[5:], "name": _cstr(body, "name") or const[5:], "price": _num(body, "price", 0),
                    "pocket": pk.group(1) if pk else None,
                    "description": descs.get(d.group(1), "") if d else ""})
    return out


def trainers(repo=REPO, species_list=None):
    r = Path(repo)
    names = {norm(s["id"]): s["id"] for s in (species_list or species(r))}
    names.update({norm(s["name"]): s["id"] for s in (species_list or [])})
    loc = {}
    for row in _csv(r, "trainers.csv"):
        loc[norm(row.get("localized_name") or "")] = row["name"]
    pic_dir = r / "graphics/trainers/front_pics"
    out = []
    cur = None
    mon = None
    for line in _read(r / "src/data/trainers.party").splitlines():
        m = re.match(r"=== (TRAINER_\w+) ===", line)
        if m:
            cur = {"id": m.group(1), "name": "", "class": "", "pic": "", "pic_slug": "", "double": False,
                   "ai": [], "items": [], "party": []}
            out.append(cur)
            mon = None
            continue
        if cur is None:
            continue
        kv = re.match(r"(Name|Class|Pic|Double Battle|AI|Items):\s*(.*)", line)
        if kv and mon is None:
            k, v = kv.groups()
            if k == "Name": cur["name"] = v
            elif k == "Class": cur["class"] = v
            elif k == "Pic":
                cur["pic"] = v
                cur["pic_slug"] = re.sub(r"[^a-z0-9]+", "_", v.lower()).strip("_")
            elif k == "Double Battle": cur["double"] = v.strip() == "Yes"
            elif k == "AI": cur["ai"] = [x.strip() for x in v.split("/")]
            elif k == "Items": cur["items"] = [x.strip() for x in v.split("/")]
            continue
        s = line.strip()
        if not s:
            continue
        if s.startswith("- ") and mon is not None:
            mon["moves"].append(re.sub(r"[^A-Z0-9]+", "_", s[2:].upper()).strip("_"))
        elif re.match(r"Level:\s*(\d+)", s) and mon is not None:
            mon["level"] = int(re.match(r"Level:\s*(\d+)", s).group(1))
        elif ":" in s and mon is not None:
            k, v = s.split(":", 1)
            if k.strip() == "Moves":
                pass
        elif ":" not in s:
            head = re.sub(r"\s*\(.*?\)", "", s.split("@")[0]).strip()
            item = s.split("@")[1].strip().upper().replace(" ", "_") if "@" in s else None
            mon = {"species": names.get(norm(head), head.upper()), "level": 1, "moves": [], "item": item}
            cur["party"].append(mon)
    for t in out:
        t["pic_ok"] = (pic_dir / (t["pic_slug"] + ".png")).exists()
        t["kind"] = ("gym" if t["class"].lower() == "leader" else "rival" if t["class"].lower() == "rival"
                     else "elite" if t["class"].lower().startswith("elite") else "trainer")
    return [t for t in out if t["id"] != "TRAINER_NONE"]


def stages(repo=REPO):
    """Fallback: the design doc's town order, one stage per town/city (no map data)."""
    out = []
    for row in _csv(repo, "overworld.csv"):
        nm = row["name"]
        if nm.isupper():
            out.append({"id": "t%02d" % (len(out) + 1), "index": len(out), "name": nm.title(),
                        "localized": row.get("localized_name") or "", "badges": None})
    return out


def load(repo=REPO):
    sp = species(repo)
    return {"species": sp, "moves": moves(repo), "items": items(repo),
            "trainers": trainers(repo, sp), "stages": stages(repo)}
