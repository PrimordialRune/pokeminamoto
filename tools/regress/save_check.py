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

    save_check.py powercut ROM ELF OUTDIR
        Power-cut test. Counts the flash writes of a first save on a blank cart
        and of a save over a loaded save, then for every write n of each, with
        the write not started and half done: load a savestate, save with the
        power cut at write n (REGRESS hook in src/regress.c), reset, and check
        that the old save loads (or none, on the blank cart) and that saving
        again works. Writes OUTDIR/powercut.json.

    save_check.py partialsaves ROM ELF OUTDIR
        Runs the save paths of link screens without a link (LinkFullSave_*,
        WriteSaveBlock2/WriteSaveBlock1Sector, TrySavingData(SAVE_LINK)) after
        a change to the game data, and checks that the change loads back.
"""
import json
import os
import shutil
import subprocess
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


def incompatible(rom, elf, sav, out, video=False):
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    emu = R.Emu(rom, elf, out, video)
    emu.cmd(f"loadsav {os.path.abspath(sav)}")
    emu.run(3400)
    status = emu.u8(emu.sym["gSaveFileStatus"])
    ok = True
    print("gSaveFileStatus after boot:", status, "(3 = SAVE_STATUS_INCOMPATIBLE)")
    ok &= status == 3
    emu.record = video # title screen to the main menu without the old save
    emu.run(600)
    emu.press("A", hold=4, after=240) # title screen -> main menu
    emu.screenshot("01_main_menu_message")
    emu.press("A", after=60)          # dismiss the message
    emu.run(30)
    emu.screenshot("02_main_menu_new_game_only")
    emu.run(60)
    emu.record = False
    if video and shutil.which("ffmpeg"):
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", "30",
                        "-i", os.path.join(emu.video_dir, "%06d.png"),
                        "-vf", "scale=480:320:flags=neighbor", "-pix_fmt", "yuv420p",
                        os.path.join(out, "incompatible.mp4")], check=True)
        shutil.rmtree(emu.video_dir)
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


def boot_status(emu):
    """Power on and return gSaveFileStatus once the boot has loaded the save."""
    emu.run(600)
    return emu.u8(emu.sym["gSaveFileStatus"])


def save_until_cut(emu, limit=3000):
    """Save from the start menu. Returns True if the power was cut on the way,
    False if the save finished."""
    R.menu_to(emu, "SAVE")
    emu.run(200)
    for _ in range(limit // 60):
        if emu.save_stats()["cutDone"]:
            return True
        if emu.field_ready() and emu.save_stats()["flashOps"]:
            return False
        emu.press("A", after=56)    # yes / overwrite / close the message
    raise R.Fail("save neither finished nor got cut")


def powercut(rom, elf, out):
    """Cut the power at every flash write of a save, in two cases (first save
    on a blank cart, save over a loaded save), each with the write not started
    and half done. After every cut: reset, and the game must load the old save
    (or find none, on a blank cart), and a new save after that must work."""
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    emu = R.Emu(rom, elf, out, False)
    st = lambda name: os.path.join(out, name + ".ss")
    boot_new_game(emu)
    emu.command(R.CMD_SETUP)
    R.wait_field(emu)
    emu.cmd(f"savestate {st('blank')}")
    first = R.decode_dump(emu)

    # Uncut saves: count the flash writes, and make the loaded state
    emu.reset_save_stats()
    if save_until_cut(emu):
        raise R.Fail("cut without a cut set")
    first_ops = emu.save_stats()
    emu.cmd("reset")
    continue_game(emu)
    if R.decode_dump(emu) != first:
        raise R.Fail("first save did not load back")
    loaded = R.decode_dump(emu)
    change_save_data(emu)
    emu.cmd(f"savestate {st('loaded')}")
    changed = R.decode_dump(emu)
    if changed == loaded:
        raise R.Fail("the changes did not show in the dump")
    emu.reset_save_stats()
    save_until_cut(emu)
    second_ops = emu.save_stats()
    emu.cmd("reset")
    continue_game(emu)
    if R.decode_dump(emu) != changed:
        raise R.Fail("second save did not load back")
    print(f"first save: {first_ops['sectorWrites']} sectors, {first_ops['flashOps']} flash writes; "
          f"save after a load: {second_ops['sectorWrites']} sectors, {second_ops['flashOps']} flash writes", flush=True)

    results = []
    cases = [("blank", first_ops["flashOps"], None, first), ("loaded", second_ops["flashOps"], loaded, changed)]
    ok = True
    for case, ops, old, new in cases:
        for k in range(1, ops + 1):
            for half in (0, 1):
                emu.cmd(f"loadstate {st(case)}")
                emu.reset_save_stats(cut_at=k, cut_half=half)
                cut = save_until_cut(emu)
                emu.cmd("reset")
                status = boot_status(emu)
                got = None
                if status in (1, 0xFF):
                    continue_game(emu)
                    got = R.decode_dump(emu)
                # Before the last write finishes the save is not made: the
                # old one (or none) must load. Emerald's two slots report a
                # cut save as corrupt (0xFF: "the previous save file will be
                # loaded", 2: "the save file has been erased"); the
                # copy-on-write save has nothing to report (1, or 0: no save).
                if old is None:
                    good = cut and status in (0, 2)
                else:
                    good = cut and status in (1, 0xFF) and got == old
                # And saving again afterwards must work
                again = None
                if good:
                    if status in (0, 2):
                        # Back to the new game, keeping the flash as the cut left it
                        emu.cmd(f"loadstate {st('blank')} keepsave")
                        target = first
                    else:
                        target = got
                    emu.reset_save_stats()
                    save_until_cut(emu)
                    emu.cmd("reset")
                    continue_game(emu)
                    again = R.decode_dump(emu) == target
                    good = again
                line = (f"{case:6s} cut at write {k:2d}/{ops} {'half done' if half else 'not started'}: "
                        f"status {status}, {'old save' if got == old and got else 'no save' if status in (0, 2) else 'NEW SAVE' if got == new else 'OTHER'}"
                        f", saving again {'works' if again else 'FAILS' if again is not None else '-'} -> {'ok' if good else 'FAIL'}")
                print(line, flush=True)
                results.append(dict(case=case, cutAt=k, ops=ops, half=half, status=status, ok=bool(good)))
                ok &= bool(good)
    with open(os.path.join(out, "powercut.json"), "w") as f:
        json.dump(dict(firstSave=first_ops, saveAfterLoad=second_ops, results=results), f, indent=1)
    emu.cmd("quit")
    print("power-cut test:", "PASS" if ok else "FAIL", f"({len(results)} cuts)")
    return 0 if ok else 1


def change_save_data(emu):
    """Change a few sectors' worth of game data: money (SaveBlock1) and the
    first PC mon moved to box 4 (PokemonStorage). Play time (SaveBlock2)
    changes anyway."""
    sb1 = emu.u32(emu.sym["gSaveBlock1Ptr"])
    emu.cmd(f"write {sb1 + 0x490:#x} 39300500")
    pc = emu.u32(emu.sym["gPokemonStoragePtr"])
    mon = emu.read(pc + 4, 80)
    emu.cmd(f"write {pc + 4 + 80 * 100:#x} {mon.hex()}")
    emu.cmd(f"write {pc + 4:#x} {'00' * 80}")
    emu.run(30)


def partialsaves(rom, elf, out):
    """The save paths link screens use (LinkFullSave_* as Task_LinkFullSave
    runs them, WriteSaveBlock2 + WriteSaveBlock1Sector, TrySavingData(SAVE_LINK)),
    each after a normal save and a change: the change must load back."""
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)
    emu = R.Emu(rom, elf, out, False)
    boot_new_game(emu)
    emu.command(R.CMD_SETUP)
    R.wait_field(emu)
    save_from_menu(emu)
    saved = os.path.join(out, "saved.ss")
    emu.cmd(f"savestate {saved}")
    ok = True
    for name, scene in (("LinkFullSave", R.SCENE_LINK_FULL_SAVE), ("WriteSaveBlock2/1", R.SCENE_SAVE_BLOCKS),
                        ("SAVE_LINK", R.SCENE_SAVE_LINK)):
        emu.cmd(f"loadstate {saved}")
        emu.run(2)
        change_save_data(emu)
        want = R.decode_dump(emu)
        emu.reset_save_stats()
        emu.command(R.CMD_SCENE, arg=scene)
        stats = emu.save_stats()
        damaged = emu.u32(emu.sym["gDamagedSaveSectors"])
        emu.cmd("reset")
        continue_game(emu)
        got = R.decode_dump(emu)
        good = got == want and not damaged
        ok &= good
        print(f"{name:18s} wrote {stats['sectorWrites']} sectors ({stats['flashOps']} flash writes), "
              f"damaged {damaged:#x}, change loads back: {got == want}", flush=True)
    emu.cmd("quit")
    print("partial-save check:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    try:
        if sys.argv[1] == "makesav":
            makesav(*sys.argv[2:5])
            sys.exit(0)
        if sys.argv[1] == "incompatible":
            sys.exit(incompatible(*sys.argv[2:6], video="--video" in sys.argv))
        if sys.argv[1] == "powercut":
            sys.exit(powercut(*sys.argv[2:5]))
        if sys.argv[1] == "partialsaves":
            sys.exit(partialsaves(*sys.argv[2:5]))
        print(__doc__)
        sys.exit(2)
    except R.Fail as e:
        print("FAIL:", e)
        sys.exit(2)
