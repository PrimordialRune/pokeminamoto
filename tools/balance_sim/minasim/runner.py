"""Batch runner: battle() specs in a process pool, one Engine per worker (the game code is
full of globals, so parallelism is by process).

    from minasim.runner import run_batch
    results = run_batch([dict(seed=1, player=[Mon(...)], foe=[Mon(...)], player_policy="greedy"), ...])

Specs are the keyword arguments of Engine.battle(); Mon objects and plain dicts both work, and
policies must be names (callables don't pickle). Results come back in spec order. A spec that
raises returns {"error": "...", "winner": 2} instead of killing the batch.
"""
import multiprocessing as mp
import os

_ENGINE = None


def _init():
    global _ENGINE
    from .engine import Engine
    _ENGINE = Engine(build=False)


def _run(spec):
    try:
        return _ENGINE.battle(**spec)
    except Exception as e:  # noqa: BLE001
        return {"error": "%s: %s" % (type(e).__name__, e), "winner": 2, "turns": 0, "player": [], "foe": []}


def _run_chunk(chunk):
    return [_run(s) for s in chunk]


def run_batch(specs, jobs=None, chunk=64):
    """Runs every spec; jobs=1 runs in-process (no fork)."""
    specs = [_plain(s) for s in specs]
    jobs = jobs or os.cpu_count() or 1
    if jobs == 1 or len(specs) <= 1:
        _init_local()
        return [_run(s) for s in specs]
    from .engine import Engine
    Engine()  # build/refresh the library once, before forking
    chunks = [specs[i:i + chunk] for i in range(0, len(specs), chunk)]
    ctx = mp.get_context("fork")
    with ctx.Pool(jobs, initializer=_init) as pool:
        out = []
        for part in pool.imap(_run_chunk, chunks):
            out.extend(part)
    return out


def _init_local():
    global _ENGINE
    if _ENGINE is None:
        from .engine import Engine
        _ENGINE = Engine()


def _plain(spec):
    """Mon objects -> dicts so the spec pickles cheaply."""
    s = dict(spec)
    for k in ("player", "foe"):
        if s.get(k) is not None:
            s[k] = [m.to_dict() if hasattr(m, "to_dict") else m for m in s[k]]
    return s
