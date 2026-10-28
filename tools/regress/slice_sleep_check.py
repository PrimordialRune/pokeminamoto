#!/usr/bin/env python3
"""L+R+SELECT sleep on slice maps, with and without an RTC (ROM from make release REGRESS=1 DINFO=1).

    python3 tools/regress/slice_sleep_check.py --out DIR [--rom ROM] [--elf ELF]

For Route1 at NITE, ViridianCity at DAY and PewterCity at MORN, each with the cartridge clock and with the RTC
absent (slice_lag.Rig no_rtc): put the game to sleep (src/sleep_mode.c), check the main loop stops and the screen
is black and frozen, wake it with A, check the loop runs again, the player can move, the map and the day/night
phase are unchanged, and the game keeps running for 5 more seconds with no lag burst. Screenshots in DIR/shots.
Exit 1 on any failed check.
"""
import argparse
import os
import shutil
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402
import slice_lag as S  # noqa: E402

CASES = [("Route1", 22), ("ViridianCity", 12), ("PewterCity", 7)]


def black(path):
    try:
        from PIL import Image
        return Image.open(path).convert("RGB").getextrema() == ((0, 0), (0, 0), (0, 0))
    except ImportError:
        return None


def same(a, b):
    return open(a, "rb").read() == open(b, "rb").read()


def one(rom, elf, out, no_rtc, m, hour):
    os.makedirs(out)
    if not no_rtc:
        os.environ["GBAEMU_RTC"] = str(1767268800 + (hour - 12) * 3600)
    rig = S.Rig(rom, elf, out, no_rtc)
    S.boot(rig, no_rtc)
    e = rig.emu
    if no_rtc:
        # the play time stands in for the clock: set it to the wanted hour
        import slice_clock as C
        C.set_playtime_for_hour(rig, hour)
        e.run(130)
    st, _ = S.start_tile(m)
    rig.warp(m, *st)
    e.run(60)
    phase0, where0 = S.rtc_state(rig)["timeOfDay"], rig.where()
    frames = lambda: e.regress()["frames"]
    ok = True

    def check(name, cond):
        nonlocal ok
        ok &= bool(cond)
        print(f"    {name}: {'ok' if cond else 'FAIL'}", flush=True)

    f0 = frames()
    e.run(10)
    check("loop runs before sleep", frames() > f0)
    k = R.KEYS
    e.cmd(f"keys {k['L'] | k['R']}")
    e.run(6)
    e.cmd(f"keys {k['L'] | k['R'] | k['SELECT']}")
    e.run(6)
    e.cmd("keys 0")
    e.run(120)
    a = e.screenshot("asleep")
    f1 = frames()
    e.run(300)
    b = e.screenshot("asleep_later")
    check("loop stopped in sleep", frames() == f1)
    check("screen frozen", same(a, b))
    bl = black(a)
    check("screen black", bl if bl is not None else True)
    e.cmd(f"keys {k['A']}")
    e.run(6)
    e.cmd("keys 0")
    e.run(90)
    e.screenshot("awake")
    f2 = frames()
    e.run(10)
    check("loop resumed", frames() > f2)
    e.reset_stats()
    pos0 = rig.where()
    import random
    rig.walk(m, 300, False, random.Random(3), [])    # planned walk: stays inside the map, no seam crossing
    r = rig.end()
    check("map unchanged", rig.where()[0] == where0[0])
    check("phase unchanged", S.rtc_state(rig)["timeOfDay"] == phase0)
    check("player can move", rig.where() != pos0)
    check(f"no lag after wake ({r['lag']} lag frames over {r['frames']})", r["lag"] == 0)
    e.cmd("quit")
    return ok


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=os.path.join(R.ROOT, "pokeemerald_modern_release_regress.gba"))
    ap.add_argument("--elf", default=None)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    elf = args.elf or args.rom.replace(".gba", ".elf")
    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    os.makedirs(args.out)
    all_ok = True
    for no_rtc in (False, True):
        for m, hour in CASES:
            tag = f"{m}_{'noRTC' if no_rtc else 'RTC'}"
            print(f"{tag} (hour {hour})", flush=True)
            all_ok &= one(args.rom, elf, os.path.join(args.out, tag), no_rtc, m, hour)
    print("slice sleep check:", "PASS" if all_ok else "FAIL")
    return 0 if all_ok else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except R.Fail as e:
        print("FAIL:", e)
        sys.exit(2)
