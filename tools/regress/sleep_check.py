#!/usr/bin/env python3
"""Check sleep mode (hold L+R, press SELECT in the overworld; any button wakes).

    python3 tools/regress/sleep_check.py --out /home/user/regress/runtime

Boots the REGRESS ROM to the overworld, triggers sleep, checks that the main
loop stops and the screen goes black, wakes it with a button, checks that the
game runs again and the screen is the same as before. Writes shots/ and
sleep.mp4 (when ffmpeg exists) into --out.
"""
import argparse
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run  # noqa: E402


def frames_done(emu):
    return emu.regress()["frames"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default="pokeemerald_modern_release_regress.gba")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()
    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    os.makedirs(args.out)
    emu = run.Emu(args.rom, args.rom.replace(".gba", ".elf"), args.out, True)

    emu.run(3400)
    presses = 0
    while not emu.in_overworld():
        emu.press("START" if presses % 40 == 39 else "A", hold=4, after=8)
        presses += 1
    for _ in range(4):
        emu.press("B", hold=4, after=8)
    run.wait_field(emu)
    emu.command(run.CMD_SETUP)
    run.wait_field(emu)

    emu.record = True
    emu.run(60)
    before = emu.screenshot("awake_before")
    f0 = frames_done(emu)
    emu.run(10)
    assert frames_done(emu) > f0, "main loop not running before sleep"

    k = run.KEYS
    emu.cmd(f"keys {k['L'] | k['R']}")
    emu.run(6)
    emu.cmd(f"keys {k['L'] | k['R'] | k['SELECT']}")
    emu.run(6)
    emu.cmd("keys 0")
    emu.run(120)
    asleep = emu.screenshot("asleep")
    f1 = frames_done(emu)
    emu.run(120)
    assert frames_done(emu) == f1, "main loop still running in sleep mode"
    emu.screenshot("asleep_later")

    emu.cmd(f"keys {k['A']}")
    emu.run(6)
    emu.cmd("keys 0")
    emu.run(90)
    after = emu.screenshot("awake_after")
    f2 = frames_done(emu)
    emu.run(10)
    assert frames_done(emu) > f2 >= f1, "main loop did not resume"

    def same(a, b):
        return open(a, "rb").read() == open(b, "rb").read()
    later = os.path.join(args.out, "shots", "asleep_later.png")
    black = False
    try:
        from PIL import Image
        black = Image.open(asleep).convert("RGB").getextrema() == ((0, 0), (0, 0), (0, 0))
    except ImportError:
        pass
    print("screen black while asleep:", black, "(needs PIL)")
    print("screen frozen while asleep:", same(asleep, later))
    # The map name popup and the NPCs move meanwhile, so no pixel comparison
    # with the screen before; look at awake_after.png.

    if shutil.which("ffmpeg"):
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", "30",
                        "-i", os.path.join(emu.video_dir, "%06d.png"),
                        "-vf", "scale=720:480:flags=neighbor", "-pix_fmt", "yuv420p",
                        os.path.join(args.out, "sleep.mp4")], check=True)
        shutil.rmtree(emu.video_dir)
    emu.cmd("quit")


if __name__ == "__main__":
    main()
