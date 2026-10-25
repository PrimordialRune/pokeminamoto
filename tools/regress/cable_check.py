#!/usr/bin/env python3
"""Cable Club check: new game, regress setup, then in Oldale's Pokémon Center 2F
ask the Cable Club attendant for the Trade Center and then a Colosseum single
battle with no partner plugged in, cancel each wait with B, and soft reset.
Screenshots every step. Usage: cable_check.py ROM ELF OUTDIR
(ROM from make release REGRESS=1 DINFO=1)."""
import sys, os, json, shutil, subprocess
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R
rom, elf, out = sys.argv[1:4]
os.makedirs(out, exist_ok=True)
emu = R.Emu(rom, elf, out, True)
emu.run(3400)
n = 0
while not emu.in_overworld():
    emu.press("START" if n % 40 == 39 else "A", hold=4, after=8); n += 1
for _ in range(4): emu.press("B", hold=4, after=8)
R.wait_field(emu)
emu.command(R.CMD_SETUP); R.wait_field(emu)
emu.record = True
emu.command(R.CMD_CABLE_CLUB); R.wait_field(emu)
shots = [emu.screenshot("cc_00_room")]
k = 1
def step(key, frames=60, name=None):
    global k
    emu.press(key, after=frames)
    shots.append(emu.screenshot(f"cc_{k:02d}_{name or key.lower()}")); k += 1
# Cable Club attendant, twice: Trade Center, then Colosseum single battle.
# Each time: save, wait for a partner (none is plugged in), cancel with B.
step("UP", 30, "face")
ROOMS = (("trade", ["A", "A", "A", "A"]),                # talk, welcome, Trade Center, save: yes
         ("colosseum", ["A", "A", "DOWN", "A", "A", "A", "A"]))  # talk, welcome, Colosseum, single, save: yes, overwrite: yes
for room, keys in ROOMS:
    for i, key in enumerate(keys):
        step(key, 240, f"{room}_{i}_{key.lower()}")
    emu.run(600)
    shots.append(emu.screenshot(f"cc_{k:02d}_{room}_waiting")); k += 1
    step("B", 300, room + "_cancel")
    step("A", 200, room + "_closed")
    R.wait_field(emu, mash="B")
    shots.append(emu.screenshot(f"cc_{k:02d}_{room}_field")); k += 1
# Soft reset: A + B + START + SELECT.
emu.cmd(f"keys {R.KEYS['A'] | R.KEYS['B'] | R.KEYS['START'] | R.KEYS['SELECT']}")
emu.run(10); emu.cmd("keys 0"); emu.run(300)
shots.append(emu.screenshot(f"cc_{k:02d}_soft_reset")); k += 1
if shutil.which("ffmpeg"):
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", "30",
                    "-i", os.path.join(emu.video_dir, "%06d.png"),
                    "-vf", "scale=720:480:flags=neighbor", "-pix_fmt", "yuv420p",
                    os.path.join(out, "cable.mp4")], check=True)
    shutil.rmtree(emu.video_dir)
emu.cmd("quit")
print(json.dumps(dict(ok=True, shots=shots)))
