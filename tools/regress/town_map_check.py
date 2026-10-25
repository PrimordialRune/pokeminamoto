#!/usr/bin/env python3
"""Opens the wall Town Map and the Fly map from the field and records them.

    python3 tools/regress/town_map_check.py ROM ELF OUTDIR

ROM from `make release REGRESS=1 DINFO=1`. Boots a new game to the field
(the art branch starts in Masara), opens each map through REGRESS_CMD_SCENE,
records every frame of the opening zoom into OUTDIR/town_map.mp4 and reports
heap peak, lag frames and CPU busy lines for each map.
"""
import json
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402

SCENE_TOWN_MAP, SCENE_FLY_MAP = 13, 14


def main():
    rom, elf, out = sys.argv[1:4]
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
    frames_dir = os.path.join(out, "frames")
    os.makedirs(frames_dir, exist_ok=True)
    frame_no = 0
    report = {}
    for name, scene in (("town_map", SCENE_TOWN_MAP), ("fly_map", SCENE_FLY_MAP)):
        emu.reset_stats()
        emu.cmd(f"write {emu.rs + 7:#x} {scene:02x}")
        emu.cmd(f"write {emu.rs + 4:#x} {R.CMD_SCENE:02x}")
        for f in range(150):
            emu.cmd("run 1")
            emu.cmd(f"screenshot {frames_dir}/{frame_no:06d}.png")
            frame_no += 1
            if f in (20, 30, 40):
                shutil.copy(f"{frames_dir}/{frame_no - 1:06d}.png", os.path.join(out, f"{name}_{f}.png"))
        st = emu.regress()
        emu.screenshot(name)
        for _ in range(3):
            emu.press("B", after=40)
        R.wait_field(emu, mash="B")
        frames = max(st["frames"], 1)
        report[name] = dict(heapPeak=st["heapPeak"], heapFails=st["heapFails"], lagFrames=st["lagFrames"],
                            busyAvg=round(st["busySum"] / frames, 1), busyMax=st["busyMax"])
        print(name, report[name], flush=True)
    with open(os.path.join(out, "report.json"), "w") as fh:
        json.dump(report, fh, indent=1)
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-framerate", "60", "-i",
                    os.path.join(frames_dir, "%06d.png"), "-vf", "scale=480:320:flags=neighbor",
                    "-pix_fmt", "yuv420p", os.path.join(out, "town_map.mp4")], check=True)
    shutil.rmtree(frames_dir)


if __name__ == "__main__":
    main()
