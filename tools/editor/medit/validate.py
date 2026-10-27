"""Validation with the same rules as tools/check_data.py.

The check functions are check_data's own: they are called on a Ctx whose file reads see the edited text instead
of the repo file (`check_data.read` is swapped for the duration of a call), and whose species data (`ctx.gd`) is
rebuilt from the edited records. A message therefore reads exactly as `python3 tools/check_data.py` prints it for
the same file. `Result.new` marks messages that were not there before the edit (the repo already has known
problems, tools/check_data_known.json).
"""
import copy
import os
import sys
import threading

from . import paths

_LOCK = threading.Lock()
_CD = None
_BASE = {}


def check_data():
    global _CD
    if _CD is None:
        for d in (os.path.join(paths.REPO, "tools"), os.path.join(paths.REPO, "tools", "balance")):
            if d not in sys.path:
                sys.path.insert(0, d)
        import check_data as cd
        _CD = cd
    return _CD


def _species_info_text(recs):
    """A stand-in for the generated src/data/pokemon/species_info.h: only the catchRate fields the check reads."""
    out = []
    for r in recs.values():
        if "catchRate" in r:
            out.append("[SPECIES_%s] = {\n .catchRate = %d,\n},\n" % (r["species"], r["catchRate"]))
    return "".join(out)


def _gamedata(base_gd, records):
    gd = copy.copy(base_gd)
    gd._recs = records
    gd.species, gd.learnsets, gd.evolutions = {}, {}, {}
    gd._load_species()
    gd._load_learnsets()
    gd._load_evolutions()
    return gd


def run(files=None, records=None, checks=None):
    """-> list of (check, message). `files`: {repo path: text} overlay; `records`: {SPECIES: record} overlay."""
    cd = check_data()
    files = files or {}
    with _LOCK:
        orig_read = cd.read
        ctx = cd.Ctx()
        if records is not None:
            allrecs = dict(ctx.gd._records())
            allrecs.update(records)
            ctx.gd = _gamedata(ctx.gd, allrecs)
            files = dict(files)
            files.setdefault("src/data/pokemon/species_info.h", _species_info_text(allrecs))

        def read(rel):
            return files[rel] if rel in files else orig_read(rel)
        cd.read = read
        try:
            for fn in checks:
                getattr(cd, fn)(ctx)
        finally:
            cd.read = orig_read
        return list(ctx.errors)


SPECIES_CHECKS = ("species_base_data", "species_contiguous", "species_learnsets", "species_evolutions")


def _speciesgen_errors(records):
    sys.path.insert(0, os.path.join(paths.REPO, "tools", "speciesgen"))
    import speciesgen as S
    here = os.getcwd()
    try:
        os.chdir(paths.REPO)               # speciesgen reads include/global.h relative to the repo root
        recs = S.load()
        recs = [dict(records[r["species"]], _id=r["_id"]) if r["species"] in records else r for r in recs]
        db = S.DB(recs)
        for _p, gen in S.OUTPUTS:
            gen(db)
    except S.RecordError as e:
        return [("speciesgen", str(e))]
    except KeyError as e:       # an unknown move, item or type name: the generator has no table row for it
        return [("speciesgen", "the generator does not know %s (a name in the record is misspelt)" % e)]
    except Exception as e:      # a malformed record can trip the generator anywhere
        return [("speciesgen", "%s: %s" % (type(e).__name__, e))]
    finally:
        os.chdir(here)
    return []


def baseline(kind):
    if kind not in _BASE:
        cd = check_data()
        names = {"species": SPECIES_CHECKS, "trainers": ("trainer_parties",), "encounters": ("wild_encounters",)}[kind]
        _BASE[kind] = {(n, m) for n, m in run(checks=names)}
    return _BASE[kind]


def result(errors, kind):
    base = baseline(kind)
    return [{"check": n, "message": m, "new": (n, m) not in base} for n, m in errors]


def validate_species(name, rec):
    recs = {name: rec}
    errs = run(records=recs, checks=SPECIES_CHECKS)
    with _LOCK:
        errs += _speciesgen_errors(recs)
    return result(errs, "species")


def validate_trainers(text):
    return result(run(files={paths.TRAINERS: text}, checks=("trainer_parties",)), "trainers")


def validate_encounters(text):
    try:
        return result(run(files={paths.ENCOUNTERS: text}, checks=("wild_encounters",)), "encounters")
    except (KeyError, ValueError, TypeError) as e:     # a structurally broken file: check_data itself would crash
        return [{"check": "wild_encounters", "message": "%s: %s" % (type(e).__name__, e), "new": True}]


def validate_arena(values):
    """The knobs are u16 (include/arena_ecology.h). PLACEHOLDER limits: Omar owns what a sane value is."""
    errs = []
    pct = [k for k in values if k.endswith("Pct")]
    for k, v in values.items():
        if not 0 <= v <= 0xFFFF:
            errs.append(("arena", "%s = %d does not fit a u16" % (k, v)))
    for k in pct:
        if values[k] > 1000:
            errs.append(("arena", "%s = %d%% is above 1000%%" % (k, values[k])))
    for k in ("firstMoverNeedsVents", "leekAlwaysCrits", "leekNeedsVents", "windRestartsVents", "rockDefFlyers"):
        if k in values and values[k] not in (0, 1):
            errs.append(("arena", "%s must be 0 or 1" % k))
    if "ventBlockers" in values and values["ventBlockers"] & ~0xF:
        errs.append(("arena", "ventBlockers = %d has bits other than ARENA_BLOCK_* (1, 2, 4, 8)" % values["ventBlockers"]))
    if "ventBlockTurns" in values and values["ventBlockTurns"] > 0xFF:
        errs.append(("arena", "ventBlockTurns above 255 (0xFF = for the rest of the battle)"))
    return [{"check": n, "message": m, "new": True} for n, m in errs]
