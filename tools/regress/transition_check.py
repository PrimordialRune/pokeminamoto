#!/usr/bin/env python3
"""Plays every Gen 1 battle transition and measures it.

    python3 tools/regress/transition_check.py ROM ELF OUTDIR [--video]

ROM from `make release REGRESS=1 DINFO=1`. From the regress setup savestate
it starts a wild battle (and a trainer battle) once per transition, forcing
it through gRegress.battleTransition, and reports for the frames the
transition runs (callback2 == CB2_OverworldBasic): frame count, lag frames
and the CPU busy lines (228 per frame; above 160 the main loop runs into the
next frame's display, above 228 the game drops a frame). Also plays the
default (unforced) wild and trainer transitions. Screenshots of the half-way
frame go to OUTDIR/shots, a video of all of them to OUTDIR/transitions.mp4.
"""
import json
import re
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402

# B_TRANSITION_GEN1_* in include/battle_transition.h, in order
GEN1 = ["double_circle", "circle", "spiral_in", "spiral_out",
        "horizontal_stripes", "vertical_stripes", "shrink", "split"]
REGRESS_TRANSITION_OFFSET = 40 + 2 * 1024 + 4   # gRegress.battleTransition


def gen1_first_id():
    with open(os.path.join(R.ROOT, "include", "battle_transition.h")) as fh:
        text = fh.read()
    names = re.findall(r"^\s+(B_TRANSITION_\w+)", text[text.index("B_TRANSITION_BLUR"):], re.M)
    return names.index("B_TRANSITION_GEN1_DOUBLE_CIRCLE") + 1   # +1: BLUR itself was cut off


def main():
    rom, elf, out = sys.argv[1:4]
    video = "--video" in sys.argv
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    emu = R.Emu(rom, elf, out, False)
    emu.run(3400)
    n = 0
    while not emu.in_overworld():
        emu.press("START" if n % 40 == 39 else "A", hold=4, after=8)
        n += 1
    for _ in range(4):
        emu.press("B", hold=4, after=8)
    R.wait_field(emu)
    emu.command(R.CMD_SETUP)
    R.wait_field(emu)
    base = os.path.join(out, "base.ss")
    emu.cmd(f"savestate {base}")
    basic = emu.sym["CB2_OverworldBasic"]
    gen1_first = gen1_first_id()
    runs = [("wild_default", R.CMD_WILD, 0), ("trainer_default", R.CMD_TRAINER, 0)]
    for i, name in enumerate(GEN1):
        cmd = R.CMD_TRAINER if "spiral" in name or name in ("shrink", "split") else R.CMD_WILD
        runs.append((name, cmd, gen1_first + i + 1))
    report = {}
    frame_no = 0
    frames_dir = os.path.join(out, "frames")
    os.makedirs(frames_dir, exist_ok=True)
    for name, cmd, override in runs:
        emu.cmd(f"loadstate {base}")
        emu.cmd(f"write {emu.rs + REGRESS_TRANSITION_OFFSET:#x} {override:02x}")
        emu.command(cmd)
        for _ in range(900):
            if emu.callback2() == basic:
                break
            emu.run(1)
        emu.reset_stats()
        shots = []
        frames = 0
        while emu.callback2() == basic and frames < 900:
            emu.run(1)
            frames += 1
            if video:
                p = os.path.join(frames_dir, f"{frame_no:06d}.png")
                emu.cmd(f"screenshot {p}")
                shots.append(p)
                frame_no += 1
        st = emu.regress()
        f = max(st["frames"], 1)
        entry = dict(frames=frames, lagFrames=st["lagFrames"], busyAvg=round(st["busySum"] / f, 1),
                     busyMax=st["busyMax"], heapPeak=st["heapPeak"], heapFails=st["heapFails"])
        if shots:
            mid = shots[len(shots) * 3 // 5]
            os.makedirs(os.path.join(out, "shots"), exist_ok=True)
            shutil.copy(mid, os.path.join(out, "shots", name + ".png"))
        report[name] = entry
        print(f"{name:20s} frames {frames:4d} lag {entry['lagFrames']:3d} busy {entry['busyAvg']}/{entry['busyMax']} "
              f"heap {entry['heapPeak']} fails {entry['heapFails']}", flush=True)
    with open(os.path.join(out, "report.json"), "w") as fh:
        json.dump(report, fh, indent=1)
    if video and frame_no:
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "60", "-i",
                        os.path.join(frames_dir, "%06d.png"), "-vf", "scale=480:320:flags=neighbor",
                        "-pix_fmt", "yuv420p", os.path.join(out, "transitions.mp4")], check=True)
    shutil.rmtree(frames_dir)


if __name__ == "__main__":
    main()
