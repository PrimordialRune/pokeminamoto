#!/usr/bin/env python3
"""Save format checks for pokeminamoto (ROM from make release REGRESS=1 DINFO=1).

    save_check.py makesav ROM ELF OUT.sav
        New game, regress setup, save from the start menu, write the flash to
        OUT.sav. Run it on another build (e.g. the base commit) to get a save
        of an older format.

    save_check.py incompatible ROM ELF SAV OUTDIR
        Boot ROM with SAV (a save of another format) and screenshot the main
        menu: it must show the incompatible-save message and offer only NEW
        GAME. Then start a new game, save, and check that saving asks before
        overwriting the old file and that the new save loads.
"""
import os
import shutil
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R


def boot_new_game(emu):
    emu.run(3400)
    n = 0
    while not emu.in_overworld():
        emu.press("START" if n % 40 == 39 else "A", hold=4, after=8)
        n += 1
        if n > 2000:
            raise R.Fail("never reached the overworld")
    for _ in range(4):
        emu.press("B", hold=4, after=8)
    R.wait_field(emu)


def save_from_menu(emu, overwrite_shot=None):
    R.menu_to(emu, "SAVE")
    emu.run(200)                      # "Would you like to save the game?" prints
    emu.press("A", after=60)          # yes, save
    if overwrite_shot:
        emu.run(120)                  # "WARNING!": three pages and two scrolls wait for a button
        for _ in range(5):
            emu.press("A", after=250)
        emu.run(200)
        emu.screenshot(overwrite_shot) # "There is a different game file..." (default NO)
        emu.press("UP", after=20)
        emu.press("A", after=60)      # yes, overwrite
    emu.run(300)
    emu.press("A", after=40)
    R.wait_field(emu, mash="B")


def continue_game(emu):
    """From power-on with a save: title, CONTINUE, until the player can move."""
    emu.run(3400)
    n = 0
    while not emu.in_overworld():
        emu.press("A", hold=4, after=8)
        n += 1
        if n > 400:
            raise R.Fail("continue never reached the overworld")
    R.wait_field(emu)


def makesav(rom, elf, out):
    d = os.path.dirname(os.path.abspath(out))
    work = os.path.join(d, "makesav_work")
    emu = R.Emu(rom, elf, work, False)
    boot_new_game(emu)
    emu.command(R.CMD_SETUP)
    R.wait_field(emu)
    save_from_menu(emu)
    print(emu.cmd(f"dumpsav {os.path.abspath(out)}"))
    emu.cmd("quit")
    shutil.rmtree(work)


def incompatible(rom, elf, sav, out):
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    emu = R.Emu(rom, elf, out, False)
    emu.cmd(f"loadsav {os.path.abspath(sav)}")
    emu.run(3400)
    status = emu.u8(emu.sym["gSaveFileStatus"])
    ok = True
    print("gSaveFileStatus after boot:", status, "(3 = SAVE_STATUS_INCOMPATIBLE)")
    ok &= status == 3
    emu.run(600)
    emu.press("A", hold=4, after=240) # title screen -> main menu
    emu.screenshot("01_main_menu_message")
    emu.press("A", after=60)          # dismiss the message
    emu.run(30)
    emu.screenshot("02_main_menu_new_game_only")
    # The old save is still on the flash
    emu.cmd(f"dumpsav {os.path.join(out, 'after_menu.sav')}")
    same = open(os.path.join(out, "after_menu.sav"), "rb").read() == open(sav, "rb").read()
    print("old save untouched by booting:", same)
    ok &= same
    # NEW GAME, save, overwrite prompt, then the new save loads
    n = 0
    while not emu.in_overworld():
        emu.press("START" if n % 40 == 39 else "A", hold=4, after=8)
        n += 1
        if n > 2000:
            raise R.Fail("new game never reached the overworld")
    for _ in range(4):
        emu.press("B", hold=4, after=8)
    R.wait_field(emu)
    emu.command(R.CMD_SETUP)
    R.wait_field(emu)
    saved = R.decode_dump(emu)
    save_from_menu(emu, overwrite_shot="03_overwrite_prompt")
    emu.cmd("reset")
    continue_game(emu)
    status = emu.u8(emu.sym["gSaveFileStatus"])
    reloaded = R.decode_dump(emu)
    emu.screenshot("04_new_save_continued")
    print("new save loads:", status == 1 and saved == reloaded)
    ok &= status == 1 and saved == reloaded
    emu.cmd("quit")
    print("incompatible-save check:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    try:
        if sys.argv[1] == "makesav":
            makesav(*sys.argv[2:5])
            sys.exit(0)
        if sys.argv[1] == "incompatible":
            sys.exit(incompatible(*sys.argv[2:6]))
        print(__doc__)
        sys.exit(2)
    except R.Fail as e:
        print("FAIL:", e)
        sys.exit(2)
