#!/usr/bin/env python3
"""Where the CPU time goes, by function, in the overworld and in a wild battle.

    python3 tools/regress/profile.py [--rom ...] [--scene walk|battle] [--top 25]

Boots the REGRESS ROM (see run.py), then runs a scene under gbaemu's
`profile` command, which single-steps and adds up emulated cycles per PC.
Prints the hottest functions. Time in the BIOS (including the Halt that
waits for VBlank) is shown as <bios>. Slow: a few minutes per scene.
"""
import argparse
import bisect
import collections
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run  # noqa: E402


def load_symbols(elf):
    syms = []
    out = subprocess.run(["arm-none-eabi-nm", "-S", "--numeric-sort", elf], capture_output=True,
                         text=True, check=True).stdout
    for line in out.splitlines():
        p = line.split()
        if len(p) == 4 and p[2] in "tTwW":
            syms.append((int(p[0], 16) & ~1, int(p[1], 16), p[3]))
    return syms


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default="pokeemerald_modern_release_regress.gba")
    ap.add_argument("--elf", default=None)
    ap.add_argument("--scene", choices=["walk", "battle"], default="walk")
    ap.add_argument("--frames", type=int, default=600)
    ap.add_argument("--top", type=int, default=25)
    args = ap.parse_args()
    elf = args.elf or args.rom.replace(".gba", ".elf")
    out = tempfile.mkdtemp()
    emu = run.Emu(args.rom, elf, out, False)

    emu.run(3400)
    n = 0
    while not emu.in_overworld():
        emu.press("START" if n % 40 == 39 else "A", hold=4, after=8)
        n += 1
    for _ in range(4):
        emu.press("B", hold=4, after=8)
    run.wait_field(emu)
    emu.command(run.CMD_SETUP)
    run.wait_field(emu)
    emu.run(60)

    path = os.path.join(out, "prof.txt")
    total = collections.Counter()

    def chunk(frames, key=None):
        emu.cmd(f"keys {run.KEYS[key] if key else 0}")
        emu.cmd(f"profile {frames} {path}")
        for line in open(path):
            pc, cyc = line.split()
            total[int(pc, 16)] += int(cyc)

    if args.scene == "walk":
        for i in range(args.frames // 32):
            chunk(32, ["DOWN", "RIGHT", "UP", "LEFT"][i % 4])
    else:
        emu.command(run.CMD_WILD)
        while not emu.in_battle():
            emu.run(10)
        emu.run(300)
        for i in range(args.frames // 6):
            chunk(6, "A" if i % 2 == 0 else None)

    syms = load_symbols(elf)
    starts = [s[0] for s in syms]
    by_func = collections.Counter()
    for pc, cyc in total.items():
        if pc < 0x4000:
            by_func["<bios>"] += cyc
            continue
        i = bisect.bisect_right(starts, pc) - 1
        if i >= 0 and pc < syms[i][0] + max(syms[i][1], 2):
            by_func[syms[i][2]] += cyc
        else:
            by_func["<%08x>" % pc] += cyc
    grand = sum(by_func.values())
    print(f"{args.scene}: {grand / args.frames:.0f} cycles per frame (280896 available)")
    for name, cyc in by_func.most_common(args.top):
        print(f"{cyc / args.frames:9.0f} {100 * cyc / grand:5.1f}%  {name}")
    emu.cmd("quit")


if __name__ == "__main__":
    main()
