#!/usr/bin/env python3
"""Timing estimate per plan 1.2 from a playthrough.py report.json.

    qa_timing.py REPORT.json [--md OUT.md]

Two numbers per segment:
  scripted   emulated frames of the bot's run / 59.7275 (the bot mashes text at full speed)
  human      steps x 16 frames + text boxes x 2 s + battle turns x 6 s (the brief's model), plus
             the new-game introduction (3 min) and the grinding the bot injected, modelled from the
             experience curve: MEDIUM SLOW, wild Lv 3-5 Kanto mons worth about 31 EXP each, about 28 s
             per wild battle (8 steps, 3 turns, 4 text boxes).
Trainer EXP is not credited, so the grind figure is an upper bound for Routes 1, 2 and 22.
"""
import argparse
import json
import re

FPS = 59.7275
INTRO_S = 180
WILD_EXP = 31
WILD_S = 8 * 16 / FPS + 3 * 6 + 4 * 2


def exp_medium_slow(n):
    if n <= 1:
        return 0
    return int(1.2 * n ** 3 - 15 * n ** 2 + 100 * n - 140)


def grind_seconds(report, seg):
    total = 0.0
    detail = []
    for inj in report["injections"]:
        if inj["seg"] != seg or inj["kind"] != "level":
            continue
        m = re.search(r"L(\d+) -> L(\d+)", inj["detail"])
        if not m:
            continue
        lo, hi = int(m.group(1)), int(m.group(2))
        xp = exp_medium_slow(hi) - exp_medium_slow(lo)
        n = xp / WILD_EXP
        s = n * WILD_S
        total += s
        detail.append((lo, hi, xp, n, s))
    return total, detail


PLAN = [   # (label, segment names, target min, target max)
    ("Wake-up to the lab battle", ["01_masara_to_lab_battle"], 10, 12),
    ("Route 1, Parcel, Index, Town Map, seal", ["02_route1_and_parcel", "03_index_and_seal"], 12, 15),
    ("Registration to the Wing Badge (with ~10 min of training)", ["04_tokiwa_to_b1"], 35, 45),
    ("North exit, Route 2, forest, Ken", ["05_route22_route2_forest"], 25, 30),
    ("Nibi to the Boulder Badge", ["06_nibi_to_b2"], 30, 40),
    ("East exit, Route 3 edge", ["07_route3_wall"], 3, 3),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("report")
    ap.add_argument("--md")
    a = ap.parse_args()
    r = json.load(open(a.report))
    segs = {s["name"]: s for s in r["segments"]}
    lines = []
    out = lines.append
    out("| Plan segment | Target (min) | Scripted run (min) | Human model, no grind (min) | Grind the bot injected (min) | Human model with grind (min) |")
    out("|---|---|---|---|---|---|")
    cum = dict(scripted=0.0, human=0.0, grind=0.0)
    wing = None
    rows = []
    for label, names, lo, hi in PLAN:
        sc = hu = gr = 0.0
        for n in names:
            s = segs.get(n)
            if not s:
                continue
            sc += s["frames"] / FPS
            h = s["steps"] * 16 / FPS + s["text_boxes"] * 2 + s["battle_turns"] * 6
            if n.startswith("01"):
                h += INTRO_S
            hu += h
            g, _ = grind_seconds(r, n)
            gr += g
        cum["scripted"] += sc
        cum["human"] += hu
        cum["grind"] += gr
        tgt = f"{lo}-{hi}" if lo != hi else str(lo)
        out(f"| {label} | {tgt} | {sc / 60:.1f} | {hu / 60:.1f} | {gr / 60:.1f} | {(hu + gr) / 60:.1f} |")
        rows.append((label, sc, hu, gr))
        if names[-1] == "04_tokiwa_to_b1":
            wing = dict(cum)
    out("")
    if wing:
        out(f"**Wing Badge reached** (target 60-75 min): scripted {wing['scripted'] / 60:.1f} min; human model {wing['human'] / 60:.1f} min "
            f"without grinding, {(wing['human'] + wing['grind']) / 60:.1f} min with the injected grinding.")
    out(f"**Boulder Badge reached** (target 120-150 min, including Route 3): scripted {cum['scripted'] / 60:.1f} min; human model "
        f"{cum['human'] / 60:.1f} min without grinding, {(cum['human'] + cum['grind']) / 60:.1f} min with the injected grinding.")
    out("")
    out("Grinding detail (EXP from the MEDIUM SLOW curve, wild Lv 3-5 at ~31 EXP and ~%d s per battle):" % WILD_S)
    for s in r["segments"]:
        _, det = grind_seconds(r, s["name"])
        for lo, hi, xp, n, sec in det:
            out(f"- {s['name']}: Lv {lo} to {hi} = {xp} EXP = about {n:.0f} wild battles = {sec / 60:.0f} min")
    txt = "\n".join(lines)
    print(txt)
    if a.md:
        open(a.md, "w").write(txt + "\n")


if __name__ == "__main__":
    main()
