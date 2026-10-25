#!/usr/bin/env python3
"""Records video with sound from the headless emulator.

    python3 tools/regress/record_av.py ROM ELF OUT.mp4 [--seconds 40] [--walk]
                                     [--state FILE] [--raw OUT.raw]

Boots a new game (mashing through the intro, as run.py does) to the first
overworld map, or loads --state, then records --seconds of the field with
gbaemu's `audio` capture, every frame, and muxes both with ffmpeg. --walk
walks the player around while recording.
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rom")
    ap.add_argument("elf")
    ap.add_argument("out")
    ap.add_argument("--seconds", type=float, default=40)
    ap.add_argument("--walk", action="store_true")
    ap.add_argument("--state")
    ap.add_argument("--raw", help="also keep the raw s16le stereo 32768 Hz audio here")
    ap.add_argument("--save-state", help="write a savestate of the start point here")
    args = ap.parse_args()
    tmp = tempfile.mkdtemp(prefix="record_av_")
    try:
        emu = R.Emu(args.rom, args.elf, tmp, False)
        if args.state:
            emu.cmd(f"loadstate {os.path.abspath(args.state)}")
        else:
            emu.run(3400)
            n = 0
            while not emu.in_overworld():
                emu.press("START" if n % 40 == 39 else "A", hold=4, after=8)
                n += 1
            for _ in range(4):
                emu.press("B", hold=4, after=8)
            R.wait_field(emu)
        if args.save_state:
            emu.cmd(f"savestate {os.path.abspath(args.save_state)}")
        raw = args.raw or os.path.join(tmp, "audio.raw")
        frames = int(args.seconds * 59.7275)
        emu.cmd(f"audio {os.path.abspath(raw)}")
        path = ["RIGHT"] * 48 + ["DOWN"] * 32 + ["LEFT"] * 48 + ["UP"] * 32
        for f in range(frames):
            if args.walk:
                key = path[(f // 2) % len(path)] if (f // 600) % 2 == 0 else None
                emu.cmd(f"keys {R.KEYS[key] if key else 0}")
            emu.cmd("run 1")
            emu.cmd(f"screenshot {tmp}/{f:06d}.png")
        emu.cmd("keys 0")
        emu.cmd("audio off")
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "59.7275",
                        "-i", os.path.join(tmp, "%06d.png"),
                        "-f", "s16le", "-ar", "32768", "-ac", "2", "-i", raw,
                        "-vf", "scale=480:320:flags=neighbor", "-pix_fmt", "yuv420p",
                        "-c:a", "aac", "-b:a", "192k", "-shortest", args.out], check=True)
        print(args.out)
    finally:
        shutil.rmtree(tmp)


if __name__ == "__main__":
    main()
