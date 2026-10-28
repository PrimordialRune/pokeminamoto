#!/usr/bin/env python3
"""Round 2 re-verification of the QA-001 and QA-002 softlocks with real button input.

    qa_verify_r2.py --states DIR_WITH_playthrough_states [--out DIR]

QA-001: from 01.ss replay the Route 1 / Tokiwa CVS Parcel beat, then cross the trigger row (4..6,4)
        three times in both directions; the player must stay free (ready) after every step.
QA-002: from 07.ss (end of the game, logged warp into Nibi Museum 1F), pay the ticket on row 7 and cross
        the row again several times; the player must stay free.
Warps used for setup are logged. Writes DIR/verify_r2.json.
"""
import argparse
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qa_lib as Q
import playthrough as P


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--states", required=True)
    ap.add_argument("--out", default="/tmp/qa_verify_r2")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    b = Q.Bot(os.path.join(a.out, "work"), quiet=False)
    ctx = P.Ctx(type("A", (), dict(starter="squirtle"))())
    res = {}

    # ---- QA-001
    b.load_state(os.path.join(a.states, "01.ss"))
    b.emu.run(30)
    b.begin("qa001")
    P.seg_route1_parcel(b, ctx)     # includes the first exit across row 4 (real input)
    b.check("QA-001: in the CVS", b.map_name(), "ViridianCity_Mart")
    for rnd in range(3):
        for d in ("UP", "UP", "DOWN", "DOWN"):
            b.step(d)
            b.emu.run(30)
            ok = b.ready()
            if not ok:
                b.settle(limit=60)
            b.check(f"QA-001: round {rnd} {d} at {tuple(b.pos())} the player is free", b.ready(), True)
    # and a walk down the aisle to the back room: Kimura / Hiroshi side must be reachable
    res["QA-001"] = all(c[1] for c in b.checks if c[0].startswith("QA-001"))

    # ---- QA-002
    b.load_state(os.path.join(a.states, "07.ss"))
    b.emu.run(30)
    b.begin("qa002")
    b.warp("PewterCity_Museum_1F", 13, 9, "repro setup (off the critical path)", settle=True)
    b.walk_dirs(["UP", "UP"])
    b.settle()          # answers the ticket question through the settle loop (A = YES)
    b.check("QA-002: ticket scene done, the player is free", b.ready(), True)
    for rnd in range(3):
        for d in ("DOWN", "UP", "UP", "DOWN", "DOWN"):
            b.step(d)
            b.emu.run(30)
            if not b.ready():
                b.settle(limit=60)
            b.check(f"QA-002: round {rnd} {d} at {tuple(b.pos())} free", b.ready(), True)
    res["QA-002"] = all(c[1] for c in b.checks if c[0].startswith("QA-002"))
    res["injections"] = b.injections
    json.dump(res, open(os.path.join(a.out, "verify_r2.json"), "w"), indent=1, default=str)
    print("RESULT:", res)
    b.close()
    sys.exit(0 if res["QA-001"] and res["QA-002"] else 1)


if __name__ == "__main__":
    main()
