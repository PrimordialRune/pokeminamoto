"""python3 -m minasim run [--quick] [--jobs N] [--baseline RUN] | duel ... | journey SCENARIO | gui"""
import argparse
import json
import random
import sys

from . import policies
from .fights import Catalog, STARTERS


_last = [-1]


def _progress(done, total, phase):
    if done // 10 == _last[0] and phase != "done":
        return
    _last[0] = done // 10
    sys.stderr.write("%-18s %3d%%\n" % (phase, 100 * done // max(1, total)))
    sys.stderr.flush()


def cmd_run(a):
    from .report import run_report
    opts = {"quick": a.quick, "jobs": a.jobs, "baseline": a.baseline}
    if a.bands:
        opts["bands"] = a.bands.split(",")
    if a.signals:
        opts["signals"] = a.signals.split(",")
    if a.scale:
        opts["scale"] = a.scale
    rep = run_report(run_id=a.run_id, options=opts, progress=_progress)
    m = rep["meta"]
    sys.stderr.write("\n")
    print("run %s: %d battles in %.0fs on %d jobs (%s engine)" % (m["run_id"], m["battles"], m["seconds"], m["jobs"], m["engine"]))
    print("fights: %s" % m["fight_sources"])
    reds = [f for f in rep["flags"] if f["severity"] == "red"][:10]
    amb = sum(f["severity"] == "amber" for f in rep["flags"])
    print("flags: %d red, %d amber, %d total" % (sum(f["severity"] == "red" for f in rep["flags"]), amb, len(rep["flags"])))
    for f in reds:
        print("  [red %.2f] %s: %s" % (f["score"], f["title"], f["detail"][:140]))
    print("report: build/balance/runs/%s/report.json" % m["run_id"])


def _parse_team(spec):
    out = []
    for part in spec.split(","):
        sp, _, lv = part.partition(":")
        out.append({"species": sp.strip().upper(), "level": int(lv or 5), "moves": None, "item": None})
    return out


def cmd_duel(a):
    from .signals import _common as C
    from . import stats
    from .report import Ctx
    ctx = Ctx(jobs=1)
    n = a.n
    wins, turns, sample = 0, 0, None
    for k in range(n):
        seed = a.seed + k if a.seed is not None else random.randrange(1 << 30)
        if a.fight:
            f = ctx.fight_by_id[a.fight]
            st = a.starter or "CHARMANDER"
            draw = ctx.teams.player_team(f, a.band, st, random.Random(seed))
            player, foe, bag = draw["mons"], ctx.teams.foe_mons(f, st), draw["bag"]
        else:
            player, foe, bag = _parse_team(a.player), _parse_team(a.foe), {}
        res = C.battle(None, seed=seed, player=C.make_mons(player), foe=C.make_mons(foe), player_policy=a.player_policy or a.band,
                       foe_policy=a.foe_policy, bag=bag, log=(k == 0))
        wins += res["winner"] == 0
        turns += res["turns"]
        if k == 0:
            sample = res
    ci = stats.wilson(wins, n)
    print(json.dumps({"wins": wins, "n": n, "win_rate": round(wins / n, 4), "ci": [round(ci[0], 4), round(ci[1], 4)],
                      "turns": round(turns / n, 2), "player": player, "foe": foe,
                      "first_battle": {"winner": sample["winner"], "turns": sample["turns"], "player": sample["player"], "log": sample.get("log")}},
                     indent=1, default=str))


def cmd_lint(a):
    from .report import Ctx
    ctx = Ctx(jobs=1)
    for kind, subj, msg in dict.fromkeys(ctx.stages.lint + ctx.catalog.lint):
        print("%-16s %-34s %s" % (kind, subj, msg))
    print(ctx.catalog.stats())


def main(argv=None):
    p = argparse.ArgumentParser(prog="minasim")
    sub = p.add_subparsers(dest="cmd", required=True)
    r = sub.add_parser("run", help="full balance report")
    r.add_argument("--quick", action="store_true", help="about a minute on 4 cores")
    r.add_argument("--jobs", type=int)
    r.add_argument("--baseline", help="run id to diff against")
    r.add_argument("--bands")
    r.add_argument("--signals", help="comma list: " + ",".join(["difficulty", "playthrough", "species", "moves", "items", "special_talk", "dominance"]))
    r.add_argument("--scale", type=float)
    r.add_argument("--run-id")
    r.set_defaults(fn=cmd_run)
    d = sub.add_parser("duel", help="replay or probe one fight")
    d.add_argument("--fight")
    d.add_argument("--band", default="average")
    d.add_argument("--starter")
    d.add_argument("--player", help="SPECIES:LEVEL,...")
    d.add_argument("--foe", help="SPECIES:LEVEL,...")
    d.add_argument("--player-policy")
    d.add_argument("--foe-policy", default="rom")
    d.add_argument("--seed", type=int)
    d.add_argument("-n", "--n", type=int, default=1)
    d.set_defaults(fn=cmd_duel)
    l = sub.add_parser("lint", help="list unstaged maps, unplaced trainers, unknown species")
    l.set_defaults(fn=cmd_lint)
    g = sub.add_parser("gui", help="local web GUI")
    g.add_argument("rest", nargs=argparse.REMAINDER)
    j = sub.add_parser("journey", help="play one scripted chapter (scenarios/*.yaml) battle after battle")
    j.add_argument("rest", nargs=argparse.REMAINDER)
    a = p.parse_args(argv)
    if a.cmd == "journey":
        from . import journey
        return journey.main(a.rest)
    if a.cmd == "gui":
        from .gui import server
        sys.argv = ["minasim gui"] + a.rest
        return server.main()
    return a.fn(a)


if __name__ == "__main__":
    main()
