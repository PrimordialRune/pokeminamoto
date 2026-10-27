"""Shared plumbing for the signals: the run context, the process pool, seeds and flag helpers."""
import inspect
import math
import multiprocessing as mp
import os
import time
import zlib

from .. import stats

_TRACE = os.environ.get("MINASIM_TRACE")   # dir: each worker writes its current battle kwargs (crash repro)
_G = None            # the Ctx shared with forked workers
_ENGINE = None


def seed_of(*key):
    """Stable 31-bit seed from a cell key: same build + same key = same battle, in any process."""
    return zlib.crc32("|".join(str(k) for k in key).encode()) & 0x7FFFFFFF


def load_engine():
    """The real engine if `minasim.engine` imports (and MINASIM_FAKE is unset), else the test stand-in."""
    if os.environ.get("MINASIM_FAKE") != "1":
        try:
            from ..engine import Engine, Mon  # noqa: F401
            return Engine, Mon, "native"
        except Exception:
            pass
    import sys
    here = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    if here not in sys.path:
        sys.path.insert(0, here)
    from tests.fake_engine import Engine, Mon  # noqa: F401
    return Engine, Mon, "fake"


def engine():
    global _ENGINE
    if _ENGINE is None:
        E, _, kind = load_engine()
        if kind == "native":
            try:
                _ENGINE = E(build=False)           # workers never run make; run_report builds once up front
            except Exception:
                _ENGINE = E()
        else:
            _ENGINE = E()
    return _ENGINE


def mon_cls():
    return load_engine()[1]


def battle(ctx_or_none, **kw):
    """engine.battle with only the keyword arguments this engine accepts (bag, hp ... may be missing)."""
    e = engine()
    if not hasattr(e, "_accepts"):
        try:
            e._accepts = set(inspect.signature(e.battle).parameters)
        except (TypeError, ValueError):
            e._accepts = None
    if e._accepts is not None and "kwargs" not in e._accepts:
        kw = {k: v for k, v in kw.items() if k in e._accepts}
    if _TRACE:
        with open(os.path.join(_TRACE, "trace.%d" % os.getpid()), "w") as fh:
            fh.write(repr(kw))
    return e.battle(**kw)


ITEM_ALIAS = {"LEEK": "STICK"}
_ITEMS = None


def _known_item(name):
    """Held item usable by the engine (alias fixes, unknown items dropped)."""
    global _ITEMS
    if not name:
        return None
    name = ITEM_ALIAS.get(name, name)
    if _ITEMS is None:
        try:
            from ..engine import consts
            _ITEMS = set(consts().items)
        except Exception:
            _ITEMS = False
    return name if _ITEMS is False or name in _ITEMS else None


def make_mons(dicts, tp_spent=None, hp=None, exps=None):
    Mon = mon_cls()
    out = []
    for i, d in enumerate(dicts):
        args = dict(moves=d.get("moves"), item=_known_item(d.get("item")))
        if tp_spent is not None:
            args["tp_spent"] = tp_spent[i]
        try:
            m = Mon(d["species"], d["level"], **args)
            if hp is not None and hp[i] is not None:
                m.hp = hp[i]
            if d.get("awakened") and hasattr(m, "awakened"):
                m.awakened = True
            if exps is not None and exps[i] is not None and hasattr(m, "exp"):
                m.exp = exps[i]
        except TypeError:
            m = Mon(d["species"], d["level"], **{k: v for k, v in args.items() if k != "tp_spent"})
        out.append(m)
    return out


def summ(res):
    """Reduce an engine result to what the signals need (small, picklable)."""
    pl = res["player"]
    mx = sum(m["max_hp"] for m in pl) or 1
    hp = sum(max(0, m["hp"]) for m in pl)
    tpf = sum(1 - m["tp"] / m["tp_max"] for m in pl) / max(1, len(pl)) if pl and pl[0].get("tp_max") else 0.0
    it = res.get("items_used") or {}
    return {"w": res["winner"], "turns": res["turns"], "hp": hp / mx, "tp": tpf,
            "items": sum(it.values()) if isinstance(it, dict) else int(it or 0), "item_by": it if isinstance(it, dict) else {},
            "moves": res.get("moves_used") or {}, "talk": res.get("talk_used", 0), "special": res.get("special_used", 0),
            "struggles": res.get("struggles", 0), "min_tp": res.get("min_tp"),
            "player": pl}


def _call(args):
    fn, job = args
    return fn(job)


def pmap(fn, jobs, ctx, label=""):
    """Run fn(job) over jobs in a fork pool (the Ctx is inherited, not pickled)."""
    global _G
    _G = ctx
    jobs = list(jobs)
    if not jobs:
        return []
    ctx.progress(label, 0, len(jobs))
    if ctx.jobs <= 1 or len(jobs) == 1:
        out = []
        for i, j in enumerate(jobs):
            out.append(fn(j))
            if i % 8 == 0:
                ctx.progress(label, i + 1, len(jobs))
        ctx.progress(label, len(jobs), len(jobs))
        return out
    from concurrent.futures import ProcessPoolExecutor
    from concurrent.futures.process import BrokenProcessPool
    out = []
    try:
        with ProcessPoolExecutor(ctx.jobs, mp_context=mp.get_context("fork")) as pool:
            for i, r in enumerate(pool.map(_call, [(fn, j) for j in jobs], chunksize=max(1, len(jobs) // (ctx.jobs * 8)))):
                out.append(r)
                if i % 8 == 0:
                    ctx.progress(label, i + 1, len(jobs))
    except BrokenProcessPool:
        raise RuntimeError("an engine worker crashed during %r; rerun with MINASIM_TRACE=<dir> to capture the battle that did it" % label)
    ctx.progress(label, len(jobs), len(jobs))
    return out


def worker_ctx():
    return _G


class Section:
    """What a signal returns: report rows under `data[key]` and a list of flags."""

    def __init__(self):
        self.data = {}
        self.flags = []
        self.battles = 0
        self.notes = []


def flag(id, kind, severity, score, title, detail, stage=None, band="both", subject=None, metric=None,
         suggestion=None, repro=None):
    return {"id": id, "kind": kind, "severity": severity, "score": round(float(score), 3), "stage": stage,
            "band": band, "subject": dict({"species": None, "move": None, "item": None, "trainer": None}, **(subject or {})),
            "title": title, "detail": detail, "metric": metric, "change": {"vs": None, "delta": None, "status": "new"},
            "suggestion": suggestion, "repro": repro}


def outside_conf(value, ci, lo, hi):
    """(distance outside [lo, hi], confidence that the true value is outside) from a CI."""
    if value < lo:
        dist, edge, sign = lo - value, ci[1], 1
        gap = lo - ci[1]
    elif value > hi:
        dist, edge, sign = value - hi, ci[0], 1
        gap = ci[0] - hi
    else:
        return 0.0, 0.0
    se = max(1e-6, (ci[1] - ci[0]) / (2 * 1.96))
    conf = stats.norm_sf(-(gap) / se) if gap > 0 else stats.norm_sf(-(gap) / se)
    # conf = P(true value beyond the band edge) ~ Phi((distance of the near CI side ... ))
    near = (lo - value) if value < lo else (value - hi)
    return dist, max(0.0, min(1.0, stats.norm_sf(-(near) / se)))


GYM_AI = ["CHECK_BAD_MOVE", "TRY_TO_FAINT", "CHECK_VIABILITY"]
BOSS_CLASS = {"gym": "LEADER", "boss": "LEADER", "rival": "RIVAL"}


def charisma_at(ctx, stage):
    c = ctx.targets["charisma"]
    return int(min(100, c["base"] + c["per_badge"] * (stage // 2)))


def foe_kwargs(fight, ctx=None):
    """Trainer foes use the ROM trainer AI (gym leaders the strong script set) and the right trainer class (Talk is
    30 points harder on leaders and the rival); wild mons do not. Charisma follows the stage."""
    ctx = ctx or worker_ctx()
    if fight["class"] == "wild":
        return {"charisma": charisma_at(ctx, fight["stage"])}
    return {"is_trainer": True, "ai_flags": GYM_AI if fight["class"] in ("gym", "boss", "rival") else ["CHECK_BAD_MOVE"],
            "trainer_class": BOSS_CLASS.get(fight["class"], "YOUNGSTER"), "charisma": charisma_at(ctx, fight["stage"])}
