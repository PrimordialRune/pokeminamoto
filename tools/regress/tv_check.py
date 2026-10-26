#!/usr/bin/env python3
"""TV check: new game, regress setup, run the TV script (REGRESS_CMD_TV) and
screenshot each text box. Usage: tv_check.py ROM ELF OUTDIR (ROM from make release REGRESS=1 DINFO=1)."""
import sys, os, json
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
shots = []
emu.command(R.CMD_TV, wait=60)
for i in range(6):
    emu.run(60)
    shots.append(emu.screenshot(f"tv_{i}"))
    emu.press("A", after=30)
R.wait_field(emu, mash="A")
shots.append(emu.screenshot("tv_after"))
print(json.dumps(dict(ok=True, shots=shots, fieldReady=emu.field_ready())))
