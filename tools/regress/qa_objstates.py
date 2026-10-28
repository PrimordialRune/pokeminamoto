#!/usr/bin/env python3
"""Dump the story states of a playthrough as an objcount.py states.json (nonzero flags and vars).

    qa_objstates.py STATES_DIR OUT.json

States: newgame (empty), then the end of each playthrough segment (NN.ss) so objcount.py samples every map
in the flag/var situation a real player has there (hidden NPCs, quest objects, B1/B2 changes).
"""
import json
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import qa_lib as Q

NAMES = {"01": "lab_done", "02": "parcel", "03": "index_seal", "04": "after_B1", "05": "after_forest", "06": "after_B2", "07": "route3_wall"}


def main():
    sd, out = sys.argv[1], sys.argv[2]
    b = Q.Bot(os.path.join(os.path.dirname(out) or ".", "objstates_work"), quiet=True)
    states = [{"name": "newgame", "flags": {}, "vars": {}}]
    for n in sorted(f for f in os.listdir(sd) if f[:2].isdigit() and f.endswith(".ss")):
        b.load_state(os.path.join(sd, n))
        b.emu.run(30)
        sb1 = b.sb1()
        fl = b.emu.read(sb1 + 3904, 0x120)
        flags = {hex(i * 8 + k): 1 for i, byte in enumerate(fl) for k in range(8) if byte >> k & 1}
        vr = b.emu.read(sb1 + 4204, 0x100 * 2)
        vs = {hex(0x4000 + i): v for i, v in enumerate(struct.unpack("<256H", vr)) if v}
        states.append(dict(name=NAMES.get(n[:2], n), flags=flags, vars=vs))
    json.dump(states, open(out, "w"), indent=0)
    print("wrote", out, [(s["name"], len(s["flags"]), len(s["vars"])) for s in states])
    b.close()


if __name__ == "__main__":
    main()
