#!/usr/bin/env python3
"""Make the repro savestates for bugs.md from a playthrough.py run.

    qa_repro.py [--states DIR] [--out DIR]

QA-001: right after the Parcel scene in the Tokiwa CVS (step DOWN three times from the state: the third crossing of row 4 freezes).
QA-002: Nibi Museum 1F in the finished game, standing south of the ticket row (step UP twice).
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qa_lib as Q


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--states", default="/tmp/qa_full2/states")
    ap.add_argument("--out", default="/tmp/qa_repro")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    b = Q.Bot(os.path.join(a.out, "work"), quiet=True)
    b.load_state(os.path.join(a.states, "01.ss"))
    b.emu.run(30)
    # QA-001: replay segment 2 up to the parcel (the playthrough's own path), then save
    import playthrough as P
    ctx = P.Ctx(type("A", (), dict(starter="squirtle"))())
    b.begin("repro")
    orig = b.workaround
    def stop(bug, *args, **kw):
        b.save_state(os.path.join(a.out, "QA-001_after_parcel.ss"))
        raise SystemExit
    b.workaround = stop
    try:
        P.seg_route1_parcel(b, ctx)
    except SystemExit:
        pass
    b.load_state(os.path.join(a.states, "07.ss"))
    b.emu.run(30)
    b.warp("PewterCity_Museum_1F", 13, 9, "repro setup", settle=True)
    b.save_state(os.path.join(a.out, "QA-002_museum_floor.ss"))
    print("wrote", sorted(os.listdir(a.out)))
    b.close()


if __name__ == "__main__":
    main()
