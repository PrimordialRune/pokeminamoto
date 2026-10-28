#!/usr/bin/env python3
"""Power-cut and partial-save checks on a slice save made mid-route, after B1 (ROM from make release REGRESS=1 DINFO=1).

    python3 tools/regress/slice_save_check.py powercut|partialsaves|both ROM ELF OUTDIR

Runs save_check.py's `powercut` and `partialsaves` with one change: after REGRESS setup the game is put into the
state of a player who has just beaten Yujirou and is walking Route 2 to the forest (B1, the Tokiwa quest flags
and Charisma set, the badge held, a rest taken, standing on Route 2), so the saved blocks hold the slice's flags,
vars and map and not just the setup's. Both checks compare the full game-state dump before and after the save,
the power cut and the reset, so a slice flag that did not survive a cut save shows up as a failure.

Also prints the save layout inputs the ROM was built with (the literal format version and layout hash in the
ROM, which must equal /mnt/project-files/studio/base-ram.md: format 3, hash 0xD93A).
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402
import save_check as SC  # noqa: E402
import slice_lag as S  # noqa: E402

BASE_HASH = 0xD93A
BASE_VERSION = 3


def slice_state(rig):
    C = S.CONSTS
    for f in ("FLAG_Q_CARD", "FLAG_Q_YUJI_MET", "FLAG_Q_YUJI_SLEPT", "FLAG_BADGE01_GET"):
        rig.set_flag(C[f])
    rig.set_var(C["VAR_CHARISMA"], 7)
    rig.set_var(C["VAR_REST_COUNT"], 3)
    st, _ = S.start_tile("Route2")
    rig.warp("Route2", *st)


def main():
    mode, rom, elf, out = sys.argv[1:5]
    S.CONSTS.update(S.consts(["FLAG_Q_CARD", "FLAG_Q_YUJI_MET", "FLAG_Q_YUJI_SLEPT", "FLAG_BADGE01_GET",
                              "VAR_CHARISMA", "VAR_REST_COUNT"]))
    data = open(rom, "rb").read()
    word = struct.pack("<I", (BASE_HASH << 16) | BASE_VERSION)
    print(f"layout hash/version word {BASE_HASH << 16 | BASE_VERSION:#x} (format {BASE_VERSION}, hash {BASE_HASH:#06x}) "
          f"found {data.count(word)}x in the ROM")
    if not data.count(word):
        print("FAIL: the ROM does not carry the base save format version and layout hash")
        return 1

    orig_command = R.Emu.command

    def command(self, c, wait=600, arg=0):
        orig_command(self, c, wait=wait, arg=arg)
        if c == R.CMD_SETUP:
            R.wait_field(self)
            rig = S.Rig.__new__(S.Rig)
            rig.emu, rig.sym, rig.out = self, self.sym, self.out
            slice_state(rig)
            self.run(30)
            where = rig.where()
            print("slice state set, player at", where, flush=True)
    R.Emu.command = command

    orig_dump = R.decode_dump

    def decode_dump(emu):
        """The regress dump plus the whole flag block and var table, so a lost slice flag fails the compare."""
        d = orig_dump(emu)
        sb1 = emu.u32(emu.sym["gSaveBlock1Ptr"])
        d["flagBlock"] = emu.read(sb1 + 3904, 300).hex()     # offsetof(struct SaveBlock1, flags)
        d["varTable"] = emu.read(sb1 + 4204, 512).hex()      # offsetof(struct SaveBlock1, vars)
        return d
    R.decode_dump = decode_dump
    rc = 0
    if mode in ("powercut", "both"):
        rc |= SC.powercut(rom, elf, os.path.join(out, "powercut"))
    if mode in ("partialsaves", "both"):
        rc |= SC.partialsaves(rom, elf, os.path.join(out, "partialsaves"))
    return rc


if __name__ == "__main__":
    try:
        sys.exit(main())
    except R.Fail as e:
        print("FAIL:", e)
        sys.exit(2)
