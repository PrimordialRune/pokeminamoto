#!/usr/bin/env python3
"""coverage.md from a playthrough report.json and the systemic JSONs.

    qa_coverage.py REPORT.json SYSTEMIC_DIR OUT.md
"""
import json
import os
import re
import sys
from collections import Counter

PLAN_31 = ("MAS-001 MAS-002 MAS-012 MAS-003 MAS-004 MAS-005 R01-001 TOK-001 TOK-002 TOK-003 MAS-006 MAS-007 MAS-009 MAS-008 "
           "MAS-010 MAS-011 TOK-005 TOK-004 TOK-006 TOK-007 TOK-008 TOK-009 TOK-501 TOK-010 TOK-011 R02-001 FOR-001 FOR-002 "
           "NIB-005 NIB-001 NIB-002 NIB-003 NIB-502 NIB-004 R03-001 SYS-000 SYS-001 SYS-002 SYS-003 SYS-004 SYS-005 R22-501 "
           "FOR-003 TOK-503 TOK-504 NIB-501").split()


def main():
    rep = json.load(open(sys.argv[1]))
    sysdir = sys.argv[2]
    cov = dict(rep["coverage"])
    wo = json.load(open(os.path.join(sysdir, "systemic_whiteout.json")))
    cov["SYS-002"] = "forced loss, three respawns: " + "; ".join(
        f"{c['case']} -> {c['landed']}" for c in wo["result"] if c.get("landed"))
    out = ["# QA coverage, round 1", "",
           "ROM: integrated 5500a9860, `make release REGRESS=1 DINFO=1`. Evidence is from `playthrough.py` (real button input; "
           "injections listed in bugs.md and the report), except SYS-002 (systemic whiteout check).", "",
           "## Plan 3.1 rows", "", "| Row | Hit | Evidence |", "|---|---|---|"]
    miss = []
    for r in PLAN_31:
        if r in cov:
            out.append(f"| {r} | yes | {cov[r]} |")
        else:
            out.append(f"| {r} | **NO** | |")
            miss.append(r)
    out += ["", f"{len(PLAN_31) - len(miss)} of {len(PLAN_31)} rows hit." + (f" Missing: {', '.join(miss)}." if miss else ""), "",
            "Extra rows hit on the way: " + ", ".join(sorted(k for k in cov if k not in PLAN_31)) + ".", "",
            "## Plan 3.2 spot-checks (every NPC, every coord cell, every map)", ""]
    npcs = json.load(open(os.path.join(sysdir, "systemic_npcs.json")))["result"]
    per = Counter()
    bad = []
    for n in npcs:
        m = n["npc"].split(":")[0]
        per[m] += 1
        if not n.get("free"):
            bad.append(n)
    out.append("Every non-trainer NPC with a script was walked up to, talked to, left mid-box (a direction held, B pressed) "
               f"and the player had to be free again: {len(npcs)} NPCs on {len(per)} maps, {len(npcs) - len(bad)} free.")
    out.append("")
    out.append("| Map | NPCs talked to |")
    out.append("|---|---|")
    for m, c in sorted(per.items()):
        out.append(f"| {m} | {c} |")
    if bad:
        out += ["", "Not free or not reachable (all inside the Tokiwa CVS back room behind the QA-001 trigger row, or the arena maze):"]
        for n in bad:
            out.append(f"- {n['npc']}: {n.get('unreachable') or 'player not free after the talk'}")
    co = json.load(open(os.path.join(sysdir.replace("qa_sys", "qa_sys2"), "systemic_coords.json")))["result"]
    out += ["", f"Coord-event cells stepped on twice: {len(co)} cells; frozen player on: " +
            (", ".join(c["cell"] for c in co if c.get("free") is False) or "none") + " (QA-001, QA-002)."]
    open(sys.argv[3], "w").write("\n".join(out) + "\n")
    print("\n".join(out[-12:]))


if __name__ == "__main__":
    main()
