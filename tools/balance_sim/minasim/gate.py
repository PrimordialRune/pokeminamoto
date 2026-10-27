"""python3 -m minasim gate [scenario] [--runs 60] [--out gates.md] [--json out.json] [--levers]

Grades every fight of the slice journey (scenarios/masara_nibi.yaml) against the fixed bands of slice-plan 7.3 and writes
gates.md (default /mnt/project-files/studio/lab/gates.md when that folder exists, else ./gates.md). The journey reads
src/data/trainers.party and src/data/wild_encounters.json, so the gate always grades the repo's own data; the sources
table says which fights still play the scenario's fallback team. A run is: one journey per starter and band (the first-try
win rate of every fight), then the same journeys again for the Signature check, for the status-move what-if and for the
Kit what-if. 60 runs per starter and band take about 15 s on 4 cores.

Verdicts use the average player's mean over the three starters (equal weights) on the point estimate; the 95% interval of
that mean is printed so a "FAIL" within noise is visible. Skilled-band numbers are reported against targets.yaml's
skilled reference bands and never fail the gate. Exit status: 0 when every gating fight passes, 1 otherwise.
"""
import argparse
import copy
import json
import math
import os
import sys
import time

from . import arena as ARENA, economy as ECON, journey as J, policies, sigdata as SIG, stats
from .fights import STARTERS
from .gamedata import Game, ROOT

DEFAULT_OUT = "/mnt/project-files/studio/lab/gates.md"

# fight id -> (rule, label). Bands are slice-plan 7.3, exactly (percent): mean of the three starters and the per-starter floor.
RULES = {
    "regular": {"label": "Regular trainer", "mean": (85, 97), "each": (80, None), "gating": True},
    "ken": {"label": "Ken (no-whiteout)", "mean": (60, 80), "each": (50, None), "aim": (70, 80), "gating": True},
    "lab": {"label": "Lab Shigeru (no-whiteout, Lv 5 vs 5)", "mean": None, "each": (30, 70), "gating": False},
    "r22": {"label": "Route 22 Shigeru (optional)", "mean": None, "each": None, "flag_below": 10, "gating": False},
    "gym": {"label": "Gym leader", "mean": (55, 75), "each": (40, 90), "gating": True},
}
FIGHT_RULE = {
    "TRAINER_CIRCUIT_TOKIWA_1": "regular", "TRAINER_CIRCUIT_TOKIWA_2": "regular",
    "TRAINER_BUG_CATCHER_FOREST_1": "regular", "TRAINER_BUG_CATCHER_FOREST_2": "regular", "TRAINER_BUG_CATCHER_FOREST_3": "regular",
    "TRAINER_CAMPER_NIBI_PMC": "regular", "TRAINER_BUG_CATCHER_ROUTE3_1": "regular",
    "TRAINER_KEN_FOREST": "ken", "TRAINER_SHIGERU_LAB": "lab", "TRAINER_SHIGERU_ROUTE22": "r22",
    "TRAINER_YUJIROU": "gym", "TRAINER_TAKESHI": "gym",
}
SKILLED_REF = {"regular": (92, 100), "ken": (75, 95), "lab": (75, 95), "r22": (75, 95), "gym": (75, 95)}   # targets.yaml bands.trainer/rival/gym skilled
SIG_LIMIT = 5.0                                                     # plan 7.3: a Signature may move no slice fight by more than 5 points
KIT_EXTRA = 2                                                       # the Kit what-if: Potions added to the Welcome Kit
LEVER_ORDER = ["arena knobs", "moves", "AI flags (existing only)", "levels (never Yujirou's, never an Omar party)"]


# ------------------------------------------------------------------ numbers from a summary
def cell(summary, fid, band, starter):
    f = next((f for f in summary["fights"] if f["id"] == fid), None)
    return (f or {}).get("by", {}).get("%s/%s" % (band, starter))


def rates(summary, fid, band):
    """({starter: pct}, mean pct, 95% half-width of the mean, n total) for one fight and band."""
    per, ns, ws = {}, 0, 0
    for s in STARTERS:
        c = cell(summary, fid, band, s)
        if c:
            per[s] = 100.0 * c["win"]
            ns += c["n"]
            ws += round(c["win"] * c["n"])
    if not per:
        return {}, None, None, 0
    mean = sum(per.values()) / len(per)
    lo, hi = stats.wilson(ws, ns) if ns else (0, 0)
    return per, mean, 100.0 * (hi - lo) / 2, ns


def judge(rule, per, mean, half):
    """-> (verdict, [reasons]). verdict: PASS, FAIL, REPORT (non-gating), FLAG."""
    R = RULES[rule]
    why = []
    if rule == "r22":
        low = [s for s, v in per.items() if v < R["flag_below"]]
        if low:
            return "FLAG", ["%s below %d%% (release-notes item for Omar)" % (", ".join(low), R["flag_below"])]
        return "REPORT", []
    if R["mean"]:
        lo, hi = R["mean"]
        if mean < lo:
            why.append("mean %.0f%% below %d%%" % (mean, lo))
        elif mean > hi:
            why.append("mean %.0f%% above %d%%" % (mean, hi))
    if R["each"]:
        lo, hi = R["each"]
        for s, v in per.items():
            if v < lo:
                why.append("%s %.0f%% below the per-starter floor %d%%" % (s.title(), v, lo))
            elif hi is not None and v > hi:
                why.append("%s %.0f%% above the per-starter ceiling %d%%" % (s.title(), v, hi))
    if not R["gating"]:
        return "REPORT", why
    return ("FAIL" if why else "PASS"), why


# ------------------------------------------------------------------ the runs
def _variant(sc, **over):
    v = copy.copy(sc)
    v.update(over)
    return v


def kit_what_if(sc, extra):
    steps = copy.deepcopy(sc["steps"])
    for st in steps:
        if st.get("kit") and st.get("give") == "POTION":
            st["n"] = st.get("n", 1) + extra
    return _variant(sc, steps=steps)


def run_all(sc, runs, jobs):
    """The gate's journeys: base, Signature-awake, no status policy, Kit + extra Potions. -> dict of summaries."""
    out = {}
    t0 = time.time()
    out["base"] = J.summarise(sc, J.simulate(sc, runs, jobs=jobs))
    out["sig"] = J.summarise(sc, J.simulate(_variant(sc, signature="awake"), runs, jobs=jobs))
    out["nostatus"] = J.summarise(sc, J.simulate(_variant(sc, no_status=True), runs, bands=("average",), jobs=jobs))
    kit = kit_what_if(sc, KIT_EXTRA)
    out["kit"] = J.summarise(kit, J.simulate(kit, runs, bands=("average",), jobs=jobs))
    out["seconds"] = round(time.time() - t0, 1)
    return out


# ------------------------------------------------------------------ levers
def knob_sweep(sc, fid, runs, jobs):
    """For one failing fight: each knob of its arena at x0.5 and x1.5 (0/1 flags toggled), average band only.
    -> [(knob, value, mean pct, delta vs default)], sorted by the move toward the band."""
    st = next(s for s in sc["steps"] if s.get("fight") == fid)
    arena = st.get("arena", "NONE")
    if arena == "NONE":
        return []
    prefix = {"LUCKY_STRIKE": ("vent", "stillFlyer", "firstMover", "leek", "windRestarts", "sandAttack"),
              "GRUNGE_ROCK": ("rock", "wet", "gust", "seed")}.get(arena, ())
    base = J.summarise(sc, J.simulate(sc, runs, bands=("average",), jobs=jobs))
    _, m0, _, _ = rates(base, fid, "average")
    knobs = ARENA.apply(J.C.engine(), sc.get("arena_tuning"))
    rows = []
    for k, v in knobs.items():
        if prefix and not k.startswith(prefix):
            continue
        for nv in ((0 if v else 1,) if v in (0, 1) else (max(0, int(v * 0.5)), int(v * 1.5))):
            if nv == v:
                continue
            tuned = _variant(sc, arena_tuning=dict(sc.get("arena_tuning") or {}, **{k: nv}))
            s = J.summarise(tuned, J.simulate(tuned, runs, bands=("average",), jobs=jobs))
            _, m, _, _ = rates(s, fid, "average")
            rows.append((k, nv, m, m - m0))
    return m0, rows


# ------------------------------------------------------------------ the report
def build(sc, res, runs, levers=None):
    base, sig = res["base"], res["sig"]
    g = Game.get()
    lines = []
    w = lines.append
    fights = [(f["id"], f["name"]) for f in base["fights"]]
    results, failing = [], []
    for fid, name in fights:
        rule = FIGHT_RULE.get(fid)
        per, mean, half, n = rates(base, fid, "average")
        if rule is None or not per:
            results.append((fid, name, None, "REPORT", [], per, mean, half))
            continue
        v, why = judge(rule, per, mean, half)
        results.append((fid, name, rule, v, why, per, mean, half))
        if v == "FAIL":
            failing.append(fid)
    npass = sum(r[3] == "PASS" for r in results)
    nfail = sum(r[3] == "FAIL" for r in results)
    sigs = SIG.signatures()
    w("# Balance gates: Masara to Nibi (slice-plan 7.3)")
    w("")
    w("Generated by `python3 -m minasim gate --runs %d` (%.0f s for %d journeys on %d cores). Model `%s`, wild tables: repo JSON." % (
        runs, res["seconds"], 18 * runs, os.cpu_count() or 1, sc["model"]))
    w("Verdict: **%d PASS, %d FAIL** of %d gating fights (average player, first-try win rate, mean of the three starters)." % (
        npass, nfail, sum(RULES[r[2]]["gating"] for r in results if r[2])))
    w("")
    w("## 1. Fights against the bands")
    w("")
    w("| Fight | Rule | Band | Bulbasaur | Charmander | Squirtle | Mean (95% ±) | Verdict | Skilled (B/C/S, ref band) |")
    w("|---|---|---|---|---|---|---|---|---|")
    for fid, name, rule, v, why, per, mean, half in results:
        if not per:
            continue
        R = RULES.get(rule)
        band = "-"
        if R:
            parts = []
            if R["mean"]:
                parts.append("mean %d-%d" % R["mean"])
            if R["each"]:
                parts.append("each %s%d%s" % (">=" if R["each"][1] is None else "", R["each"][0], "" if R["each"][1] is None else "-%d" % R["each"][1]))
            if rule == "r22":
                parts.append("exempt, flag <10")
            if R.get("aim"):
                parts.append("aim %d-%d" % R["aim"])
            band = "; ".join(parts) + ("" if R["gating"] else " (report only)")
        sper, smean, _, _ = rates(base, fid, "skilled")
        ref = SKILLED_REF.get(rule)
        sk = "%s %s" % ("/".join("%.0f" % sper.get(s, float("nan")) for s in STARTERS),
                        ("in %d-%d" % ref if ref and smean is not None and ref[0] <= smean <= ref[1] else "out %d-%d" % ref) if ref else "") if sper else "-"
        w("| %s | %s | %s | %s | %.0f%% (±%.0f) | **%s**%s | %s |" % (
              name, rule or "-", band, " | ".join("%.0f%%" % per.get(s, float("nan")) for s in STARTERS), mean, half, v,
              (" " + "; ".join(why)) if why else "", sk))
    w("")
    w("Bands are slice-plan 7.3: regular trainers 85-97 (each starter >= 80); Ken 60-80 (aim 70-80, each >= 50); lab Shigeru report only (each 30-70);")
    w("Route 22 Shigeru exempt (flag below 10); Yujirou and Takeshi 55-75 mean, each 40-90 (Bulbasaur vs Yujirou may sit at 40, Q93).")
    w("")
    w("### Where the data comes from")
    w("")
    w("| Fight | Bulbasaur | Charmander | Squirtle |")
    w("|---|---|---|---|")
    for fid, name in fights:
        src = base["sources"].get(fid, {})
        w("| %s | %s |" % (name, " | ".join({"party": "trainers.party", "yaml": "scenario fallback"}.get(src.get(s), "-") for s in STARTERS)))
    wsrc = {k[5:]: v for k, v in base["sources"].items() if k.startswith("wild:")}
    w("")
    w("Wild tables: " + ", ".join("%s = %s" % (k, "wild_encounters.json" if v == "json" else "scenario fallback") for k, v in wsrc.items()) + ".")
    w("A fight on `scenario fallback` plays a PLACEHOLDER team until its area owner fills the trainers.party block.")
    w("")

    # ---- failing fights and levers
    w("## 2. Lever suggestions")
    w("")
    w("Lever order (plan 7.3): %s. Omar's canon parties (Yujirou: Spearow 7 [Peck/Growl/Leer], Pidgey 8 [Tackle/Sand Attack], Farfetch'd 9 [Cut/Sand Attack/Leer] with LEEK SLAP and no item) never change." % ", ".join(
        "(%d) %s" % (i + 1, l) for i, l in enumerate(LEVER_ORDER)))
    w("")
    if not failing:
        w("No gating fight fails.")
    for fid in failing:
        r = next(x for x in results if x[0] == fid)
        _, name, rule, v, why, per, mean, half = r
        low = mean < (RULES[rule]["mean"][0] if RULES[rule]["mean"] else 0)
        w("- **%s**: %s." % (name, "; ".join(why)))
        st = next(s for s in sc["steps"] if s.get("fight") == fid)
        if rule == "gym":
            w("  1. Arena knobs (%s): %s." % (st.get("arena", "none"), "ease the arena for the weakest starter" if low else "tighten the arena"))
        w("  2. Moves: %s." % ("give the foe fewer high-power or TP-cheap moves / the player an earlier answer" if low else "add a status or coverage move"))
        if rule in ("regular", "ken") and not low:
            w("  4. Levels (a fallback party: its owner sets the real one): raise the party by 1-2 levels." )
        elif rule in ("regular", "ken"):
            w("  4. Levels: lower the party by 1-2 levels.")
        if fid == "TRAINER_YUJIROU":
            w("  Yujirou's party is Omar's canon and is never moved; use levers 1-3, or the economy levers below (Kit Potions, a Route 1 Antidote).")
        if levers and fid in levers:
            m0, rows = levers[fid]
            want = 1 if low else -1
            rows = sorted(rows, key=lambda x: -want * x[3])[:4]
            w("  Measured knob sweeps (average mean now %.0f%%): %s." % (m0, "; ".join("%s=%d gives %+.0f pts" % (k, v_, d) for k, v_, _m, d in rows)))
    w("")
    w("What-if levers measured this run (average band): ")
    w("")
    w("| Lever | " + " | ".join("%s" % n for _f, n in fights if FIGHT_RULE.get(_f) in ("gym", "ken", "lab")) + " |")
    w("|---|" + "---|" * sum(FIGHT_RULE.get(f) in ("gym", "ken", "lab") for f, _n in fights))
    for label, key in (("Base (this gate)", "base"), ("Kit +%d Potions" % KIT_EXTRA, "kit"), ("No status-move policy", "nostatus")):
        cols = []
        for fid, _n in fights:
            if FIGHT_RULE.get(fid) in ("gym", "ken", "lab"):
                _p, m, _h, _n2 = rates(res[key], fid, "average")
                cols.append("%.0f%%" % m if m is not None else "-")
        w("| %s | %s |" % (label, " | ".join(cols)))
    w("")

    # ---- signature check
    w("## 3. Signature check (plan 7.3)")
    w("")
    w("Signatures in the built data (src/data/pokemon/special_moves.h, `#ifdef` read against include/config.h): %s." % (
        ", ".join("%s (%s)" % (m.replace("_", " "), ", ".join(sp.title() for sp, mv in sigs.items() if mv == m)) for m in sorted(set(sigs.values()))) or "none"))
    w("The check awakens every Signature line in the player's party for the whole journey and compares each fight with the base run (same seeds; the average player uses it when it beats the best normal move). GREEDY PLANT and SKULL BASH appear only when the data workstream's `STARTER_SIGNATURES_PROPOSAL` switch is on and `make balance-lib` has been re-run; without them those starters show 0 (Bond fallback, no move).")
    w("")
    w("| Fight | Average: base -> awake (pts) | Skilled: base -> awake (pts) | Verdict |")
    w("|---|---|---|---|")
    sig_fail = 0
    for fid, name in fights:
        cols, bad = [], False
        for band in ("average", "skilled"):
            _p, m0, _h, _n = rates(base, fid, band)
            _p, m1, _h, _n = rates(sig, fid, band)
            if m0 is None:
                cols.append("-")
                continue
            d = m1 - m0
            bad |= abs(d) > SIG_LIMIT
            cols.append("%.0f -> %.0f (%+.1f)" % (m0, m1, d))
        sig_fail += bad
        w("| %s | %s | %s | %s |" % (name, cols[0], cols[1], "**over %.0f pts**" % SIG_LIMIT if bad else "ok"))
    w("")
    w("Result: %s. Noise: each mean covers 3 starters x %d runs, so a difference under about 5 points can be sampling; re-run with `--runs 200` before acting." % (
        "%d fight(s) move more than %.0f points" % (sig_fail, SIG_LIMIT) if sig_fail else "no fight moves more than %.0f points" % SIG_LIMIT, runs))
    w("")

    # ---- report-only metrics
    w("## 4. Report-only metrics (money model, plan 3.2 and 7.3)")
    w("")
    gyms = [f for f, _n in fights if FIGHT_RULE.get(f) == "gym"]
    names = dict(fights)
    w("Money model: start ₽%d (new_game.c); prize = 4 x last level x class value (battle_main.c); prices from items.h (Poke Ball ₽%d, Potion ₽%d, Antidote ₽%d); museum ticket ₽%d; Omura buys the Sandshrew for ₽%d; whiteout halves money (PLACEHOLDER); Mom does not heal; only inns restore HP and TP; no-whiteout losses restore HP and status, not TP." % (
        ECON.start_money(), ECON.item_price("POKE_BALL"), ECON.item_price("POTION"), ECON.item_price("ANTIDOTE"), ECON.MUSEUM_TICKET, ECON.SANDSHREW_SALE))
    w("")
    w("| Metric | Band | Bulbasaur | Charmander | Squirtle |")
    w("|---|---|---|---|---|")
    for band in ("average", "skilled"):
        M = {s: base["metrics"].get("%s/%s" % (band, s)) for s in STARTERS}
        for fid in gyms:
            w("| TP rests before %s (mean per run; share of runs with at least 1) | %s | %s |" % (
                names[fid], band, " | ".join("%.2f (%d%%)" % (M[s]["tp_rests"][fid], 100 * M[s]["tp_rest_share"][fid]) for s in STARTERS)))
        for fid in gyms:
            w("| ₽ at %s | %s | %s |" % (names[fid], band, " | ".join("%d" % M[s]["money_at"][fid] for s in STARTERS)))
        w("| Poke Balls at Nibi (arrival) / at the end | %s | %s |" % (band, " | ".join("%.1f / %.1f" % (M[s]["balls_at_nibi"], M[s]["balls_end"]) for s in STARTERS)))
        w("| ₽ at the end of the chapter | %s | %s |" % (band, " | ".join("%d" % M[s]["money_end"] for s in STARTERS)))
    w("")
    w("Potions used per segment (mean per run):")
    w("")
    w("| Segment | Band | Bulbasaur | Charmander | Squirtle |")
    w("|---|---|---|---|---|")
    for band in ("average", "skilled"):
        for seg in base["metrics"]["%s/%s" % (band, STARTERS[0])]["potions_used"]:
            w("| %s | %s | %s |" % (seg, band, " | ".join("%.2f" % base["metrics"]["%s/%s" % (band, s)]["potions_used"][seg] for s in STARTERS)))
    w("")
    tp_t = [base["metrics"]["average/%s" % s]["tp_rest_share"].get("TRAINER_TAKESHI", 0) for s in STARTERS]
    w("Target: at least 1 TP rest before Takeshi: average player reaches it in %s of runs (Bulbasaur/Charmander/Squirtle)." % "/".join("%d%%" % (100 * x) for x in tp_t))
    w("")

    # ---- status policy
    w("## 5. Status-move policy (average player)")
    w("")
    sh = [base["metrics"]["average/%s" % s]["status_share"] for s in STARTERS]
    w("Rate: `status_use_rate` = %.2f in targets.yaml: on a decision where a sleep, paralysis, poison, Leech Seed or stat-drop move is known, the foe is above half HP and not already afflicted, an average player picks it over an attack with that probability. **PLACEHOLDER**: no player telemetry exists; the value is a judgement for a player of Pokemon ROM hacks, and the lab measures its effect instead of assuming it." % policies.policy_params("average").get("status_use_rate", 0))
    w("")
    w("Measured this run: status moves are %s of the average player's turns (Bulbasaur/Charmander/Squirtle; the share includes status moves forced by empty TP and mistakes). Without the policy the same journeys give the `No status-move policy` row in section 2." % "/".join("%.0f%%" % (100 * x) for x in sh))
    w("")
    w("## 6. Both bands")
    w("")
    w("Skilled numbers are in the table of section 1 (starter order B/C/S) against targets.yaml's skilled reference bands; they never fail the gate. A big skilled-minus-average gap (above 25 points) means the fight rewards knowledge too much: %s." % (
        ", ".join("%s (%+.0f)" % (names[f], rates(base, f, "skilled")[1] - rates(base, f, "average")[1]) for f in names
                  if rates(base, f, "skilled")[1] is not None and rates(base, f, "skilled")[1] - rates(base, f, "average")[1] > 25) or "none this run"))
    w("")
    return "\n".join(lines), results, failing


def main(argv=None):
    p = argparse.ArgumentParser(prog="minasim gate")
    p.add_argument("scenario", nargs="?", default="scenarios/masara_nibi.yaml")
    p.add_argument("--runs", type=int, default=60, help="runs per starter and band")
    p.add_argument("--jobs", type=int, default=os.cpu_count())
    p.add_argument("--out", help="gates.md path (default %s if its folder exists, else ./gates.md)" % DEFAULT_OUT)
    p.add_argument("--json", help="also write the numbers as JSON")
    p.add_argument("--levers", action="store_true", help="sweep the arena knobs of every failing gym (slower)")
    J.add_scenario_args(p)
    a = p.parse_args(argv)
    sc = J.prepare(a)
    if sc["model"] != "slice":
        sys.exit("gate grades the slice model; --model legacy is for parity runs only")
    res = run_all(sc, a.runs, a.jobs)
    md, results, failing = build(sc, res, a.runs)
    levers = {}
    if a.levers:
        for fid in failing:
            if FIGHT_RULE.get(fid) == "gym":
                levers[fid] = knob_sweep(sc, fid, a.runs, a.jobs)
        md, results, failing = build(sc, res, a.runs, levers)
    out = a.out or (DEFAULT_OUT if os.path.isdir(os.path.dirname(DEFAULT_OUT)) else "gates.md")
    with open(out, "w", encoding="utf-8") as f:
        f.write(md + "\n")
    print(md.split("\n## 1.")[0].strip())
    for fid, name, rule, v, why, per, mean, half in results:
        if per:
            print("%-26s %-6s %-7s %s%s" % (name[:25], rule or "-", v, " ".join("%3.0f" % per.get(s, float("nan")) for s in STARTERS), ("  " + "; ".join(why)) if why else ""))
    print("gates.md: %s (%.1f s of simulation)" % (out, res["seconds"]))
    if a.json:
        with open(a.json, "w") as f:
            json.dump({"results": [{"id": r[0], "rule": r[2], "verdict": r[3], "why": r[4], "per": r[5], "mean": r[6]} for r in results],
                       "base": res["base"], "sig": res["sig"], "nostatus": res["nostatus"], "kit": res["kit"], "seconds": res["seconds"]}, f, indent=1)
    return 1 if failing else 0


if __name__ == "__main__":
    sys.exit(main())
