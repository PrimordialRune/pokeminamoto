#!/usr/bin/env python3
"""Headless regression run for pokeminamoto.

Builds nothing itself. Point it at a `make release REGRESS=1` ROM and its ELF:

    make release REGRESS=1 -j$(nproc)
    python3 tools/regress/run.py --out build/regress/<name>

It plays a fixed route in tools/gba-emu (new game, setup, overworld walk,
Pokedex, party + summary, bag, trainer card, PC, wild battle, trainer battle,
in-game save, reset, continue) and writes into --out:

    report.json   per-scene heap peak, DMA queue peak, lag frames, CPU busy
                  lines, heap in use back in the overworld (leak check: it
                  must equal the amount after setup), plus the game-state
                  dumps and checks. A heap error that stops the game (debug
                  and regress builds, src/malloc.c) fails the run.
    shots/*.png   one screenshot per checkpoint
    run.mp4       video of the whole route after setup (needs ffmpeg)

Compare two runs with tools/regress/compare.py.
"""
import argparse
import json
import os
import shutil
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EMU = os.path.join(ROOT, "tools", "gba-emu", "gbaemu")

KEYS = {"A": 1, "B": 2, "SELECT": 4, "START": 8, "RIGHT": 16, "LEFT": 32,
        "UP": 64, "DOWN": 128, "R": 256, "L": 512}

CMD_SETUP, CMD_WILD, CMD_TRAINER, CMD_PC, CMD_DUMP, CMD_HEAL, CMD_TV, CMD_CABLE_CLUB, CMD_SCENE = range(1, 10)

# Fields appended to struct RegressState after dump[REGRESS_DUMP_WORDS]
EXT = 40 + 2 * 1024 + 32  # heap error fields, after the resource limits and stackLowest
HEAP_FATAL = {1: "heap full", 2: "Free of a bad pointer", 3: "Free of a free block"}
(SCENE_EVOLUTION, SCENE_NAMING, SCENE_EGG_HATCH, SCENE_INGAME_TRADE, SCENE_HALL_OF_FAME,
 SCENE_DOUBLE_BATTLE, SCENE_SUBSTITUTE, SCENE_BEDROOM_PC, SCENE_POKENAV, SCENE_SAVE_FAILED,
 SCENE_SAVE_FAILED_HOF, SCENE_LOAD_HOF) = range(1, 13)

# Start menu order with the Pokedex and Pokemon flags set and no PokeNav
MENU = ["POKEDEX", "POKEMON", "BAG", "PLAYER", "SAVE", "OPTION", "EXIT"]


# struct RegressState: the resource-limit fields follow dump[] (40 + 2 * REGRESS_DUMP_WORDS)
LIMITS_OFF = 40 + 2 * 1024
LIMIT_FAILS = ("spriteFails", "paletteFails", "tileFails", "windowFails")


class Fail(Exception):
    pass


def limits_text(lim):
    """One line for a scene's resource limits."""
    def v(x):
        return "-" if x is None else str(x)
    return (f"obj tiles high {lim['tileHigh']:4d} free run {v(lim['tileFreeRunMin']):>4s} pal free {v(lim['palFreeMin']):>2s} "
            f"sprites {lim['spritesMax']:3d} objevents {lim['objEventsMax']:2d} "
            f"fails spr {lim['spriteFails']} pal {lim['paletteFails']} tiles {lim['tileFails']} win {lim['windowFails']}")


def limit_failures(name, lim):
    """Messages for every nonzero failure counter of a scene's limits dict."""
    return [f"{name}: {k} = {lim[k]}" for k in LIMIT_FAILS if lim.get(k)]


class Emu:
    def __init__(self, rom, elf, out, record):
        env = dict(os.environ, GBAEMU_RTC=os.environ.get("GBAEMU_RTC", "1767268800"))  # 2026-01-01 12:00 UTC
        self.p = subprocess.Popen([EMU], env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1, cwd=ROOT)
        self.sym = {}
        self.all_syms = {}
        for line in subprocess.run(["arm-none-eabi-nm", elf], capture_output=True,
                                   text=True, check=True).stdout.splitlines():
            parts = line.split()
            if len(parts) == 3:
                self.sym.setdefault(parts[2], int(parts[0], 16))
                self.all_syms.setdefault(parts[2], []).append(int(parts[0], 16))
        self.out = out
        self.record = False
        self.recordable = record
        self.frame_no = 0
        self.video_dir = os.path.join(out, "frames")
        os.makedirs(self.video_dir, exist_ok=True)
        os.makedirs(os.path.join(out, "shots"), exist_ok=True)
        self.cmd("load " + os.path.abspath(rom))
        self.rs = self.sym["gRegress"]

    def cmd(self, line):
        self.p.stdin.write(line + "\n")
        reply = json.loads(self.p.stdout.readline())
        if not reply.get("ok"):
            raise Fail(f"emu: {line}: {reply}")
        return reply

    # --- time and input -------------------------------------------------
    def run(self, frames):
        if not (self.record and self.recordable):
            self.cmd(f"run {frames}")
            return
        while frames > 0:
            step = min(2, frames)
            self.cmd(f"run {step}")
            frames -= step
            self.cmd(f"screenshot {self.video_dir}/{self.frame_no:06d}.png")
            self.frame_no += 1

    def press(self, key, hold=6, after=24):
        self.cmd(f"keys {KEYS[key]}")
        self.run(hold)
        self.cmd("keys 0")
        self.run(after)

    def hold(self, key, frames):
        self.cmd(f"keys {KEYS[key]}")
        self.run(frames)
        self.cmd("keys 0")

    # --- memory ---------------------------------------------------------
    def read(self, addr, n):
        return bytes.fromhex(self.cmd(f"read {addr:#x} {n}")["hex"])

    def u8(self, addr):
        return self.read(addr, 1)[0]

    def u32(self, addr):
        return struct.unpack("<I", self.read(addr, 4))[0]

    def callback2(self):
        return self.u32(self.sym["gMain"] + 4) & ~1

    def in_overworld(self):
        return self.callback2() == self.sym["CB2_Overworld"]

    def in_battle(self):
        return self.callback2() == self.sym["BattleMainCB2"]

    def regress(self):
        raw = self.read(self.rs, 40)
        (magic, cmd, done, ready, _, heap_peak, heap_fails, dma_peak, dma_full,
         frames, lag, busy_sum, busy_max, dump_words, _last) = struct.unpack("<IBBBBIIHHIIIHHI", raw)
        return dict(magic=magic, cmd=cmd, done=done, ready=ready, heapPeak=heap_peak,
                    heapFails=heap_fails, dmaPeak=dma_peak, dmaFull=dma_full, frames=frames,
                    lagFrames=lag, busySum=busy_sum, busyMax=busy_max, dumpWords=dump_words)

    def limits(self):
        """Resource limits of the scene so far (struct RegressState, appended after dump[])."""
        raw = self.read(self.rs + LIMITS_OFF, 20)
        (tile_high, run_min, pal_min, sprites, objs, sprite_f, pal_f, tile_f, win_f, _) = struct.unpack("<10H", raw)
        return dict(tileHigh=tile_high,
                    tileFreeRunMin=run_min - 1 if run_min else None,
                    palFreeMin=pal_min - 1 if pal_min else None,
                    spritesMax=sprites, objEventsMax=objs,
                    spriteFails=sprite_f, paletteFails=pal_f, tileFails=tile_f, windowFails=win_f)
    def heap_fatal(self):
        """None, or a description of the heap error that stopped the game."""
        kind, value, caller = struct.unpack("<III", self.read(self.rs + EXT, 12))
        if not kind:
            return None
        return f"{HEAP_FATAL.get(kind, kind)}: {'size' if kind == 1 else 'pointer'} {value:#x}, called from {caller:#x}"

    def check_fatal(self):
        fatal = self.heap_fatal()
        if fatal:
            self.screenshot("FAIL_heap_fatal")
            raise Fail("heap error stopped the game: " + fatal)

    def heap_used(self):
        """Bytes in allocated heap blocks, headers included (GetHeapUsed in
        src/malloc.c), read from memory so no frame runs, and whether the map
        name pop-up (which holds a window buffer while it is up) was up."""
        heap = self.sym["gHeap"]
        pos, used = heap, 0
        for _ in range(4096):
            flag, magic, size, _prev, nxt = struct.unpack("<HHIII", self.read(pos, 16))
            if magic != 0xA3A3:
                raise Fail(f"heap block at {pos:#x} has a bad header")
            if flag:
                used += 16 + size
            pos = nxt
            if pos == heap:
                break
        popup = self.u8(self.sym["sMapNamePopupWindowId"]) != 0xFF   # WINDOW_NONE
        return used, popup

    def player_choosing_action(self):
        # The player controller's HandleInputChooseAction is the static one
        # closest below its HandleInputChooseMove (another controller has one too).
        move = self.sym["HandleInputChooseMove"]
        act = max(a for a in self.all_syms["HandleInputChooseAction"] if a < move)
        return (self.u32(self.sym["gBattlerControllerFuncs"]) & ~1) == act

    def field_ready(self):
        return self.in_overworld() and self.u8(self.rs + 6) == 1

    def reset_stats(self):
        # heapPeak .. busyLinesMax (offsets 8..35)
        self.cmd(f"write {self.rs + 8:#x} {'00' * 28}")
        self.cmd(f"write {self.rs + LIMITS_OFF:#x} {'00' * 20}")

    def command(self, c, wait=600, arg=0):
        done = self.u8(self.rs + 5)
        self.cmd(f"write {self.rs + 7:#x} {arg:02x}")
        self.cmd(f"write {self.rs + 4:#x} {c:02x}")
        for _ in range(wait // 2):
            self.run(2)
            if self.u8(self.rs + 5) != done:
                return
        raise Fail(f"command {c} never ran")

    def screenshot(self, name):
        path = os.path.join(self.out, "shots", name + ".png")
        self.cmd(f"screenshot {path}")
        return path


def wait_field(emu, limit=1800, mash=None):
    """Run until the player can move again, optionally pressing a key."""
    for i in range(limit // 10):
        if emu.field_ready():
            emu.run(10)
            if emu.field_ready():
                return
        if i % 10 == 0:
            emu.check_fatal()
        if mash:
            emu.press(mash, hold=4, after=6)
        else:
            emu.run(10)
    emu.check_fatal()
    emu.screenshot("FAIL_field_not_ready")
    raise Fail(f"field never became ready (callback2 {emu.callback2():#x})")


def decode_dump(emu):
    emu.command(CMD_DUMP)
    st = emu.regress()
    words = struct.unpack(f"<{st['dumpWords']}H", emu.read(emu.rs + 40, st["dumpWords"] * 2))
    w = list(words)
    if w[0] != 0xD00D or w[-1] != 0xE0D0:
        raise Fail("bad dump framing")
    i = 1
    def take(n=1):
        nonlocal i
        v = w[i:i + n]
        i += n
        return v if n > 1 else v[0]
    def u32():
        lo, hi = take(2)
        return lo | (hi << 16)
    d = {"party": [], "pc": [], "bag": []}
    for _ in range(take()):
        m = dict(zip(["species", "level", "hp", "maxHP", "atk", "def", "spe", "spatk", "spdef"], take(9)))
        m["moves"] = take(4)
        m["pp"] = take(4)
        m["exp"], m["personality"], m["otId"] = u32(), u32(), u32()
        m["item"], m["friendship"], m["status"], m["ivs"] = take(4)
        d["party"].append(m)
    count = take()
    d["pcCount"] = count
    for _ in range(min(count, 64)):
        boxpos = take()
        m = {"box": boxpos >> 8, "pos": boxpos & 0xFF, "species": take(), "level": take(),
             "personality": u32(), "moves": take(4)}
        d["pc"].append(m)
    for _ in range(5):
        n = take()
        d["bag"].append([take(2) for _ in range(n)])
    d["money"] = u32()
    d["name"] = take(4)
    d["gender"], d["tid_lo"], d["tid_hi"], d["map"], d["x"], d["y"] = take(6)
    d["dexSeen"] = take(10)
    d["dexCaught"] = take(10)
    d["flags"], d["varOldale"], d["varRival"] = take(3)
    return d


def scene(emu, report, name, fn):
    os.makedirs(os.path.join(emu.out, "states"), exist_ok=True)
    emu.cmd(f"savestate {os.path.join(emu.out, 'states', name + '.ss')}")
    emu.reset_stats()
    before = emu.regress()
    fn()
    emu.check_fatal()
    st = emu.regress()
    frames = max(st["frames"], 1)
    entry = {
        "heapPeak": st["heapPeak"], "heapFails": st["heapFails"],
        "dmaPeak": st["dmaPeak"], "dmaFull": st["dmaFull"],
        "frames": st["frames"], "lagFrames": st["lagFrames"],
        "busyAvg": round(st["busySum"] / frames, 1), "busyMax": st["busyMax"],
        "limits": emu.limits(),
    }
    entry["shot"] = os.path.relpath(emu.screenshot(f"{len(report['scenes']):02d}_{name}"), emu.out)
    # Leak check: back in the overworld, the heap holds what it held after setup
    # (one baseline with the map name pop-up up and one without)
    if emu.field_ready():
        entry["heapUsed"], popup = emu.heap_used()
        base = report["checks"].setdefault("heapBaseline", {}).setdefault(
            "popUp" if popup else "noPopUp", entry["heapUsed"])
        if entry["heapUsed"] != base:
            report["checks"].setdefault("heapLeaks", {})[name] = entry["heapUsed"] - base
            print(f"  LEAK: {name} leaves {entry['heapUsed'] - base:+d} bytes on the heap", flush=True)
    report["scenes"][name] = entry
    print(f"  {name:14s} heap {entry['heapPeak']:6d} fails {entry['heapFails']} dma {entry['dmaPeak']:3d} "
          f"full {entry['dmaFull']} lag {entry['lagFrames']:3d} busy {entry['busyAvg']}/{entry['busyMax']}", flush=True)
    print(f"  {'':14s} {limits_text(entry['limits'])}", flush=True)


def menu_to(emu, item):
    emu.press("START", after=30)
    cur = emu.u8(emu.sym["sStartMenuCursorPos"])
    target = MENU.index(item)
    while cur != target:
        emu.press("DOWN", hold=4, after=10)
        cur = emu.u8(emu.sym["sStartMenuCursorPos"])
    emu.press("A", after=60)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default="pokeemerald_modern_release_regress.gba")
    ap.add_argument("--elf", default=None)
    ap.add_argument("--out", required=True)
    ap.add_argument("--no-video", action="store_true")
    args = ap.parse_args()
    elf = args.elf or args.rom.replace(".gba", ".elf")
    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    os.makedirs(args.out)
    emu = Emu(args.rom, elf, args.out, not args.no_video)
    report = {"rom": args.rom, "scenes": {}, "snaps": {}, "dumps": {}, "checks": {}}

    # --- New game, mash through the intro until the truck --------------
    print("boot + intro", flush=True)
    emu.run(3400)
    presses = 0
    while not emu.in_overworld():
        emu.press("START" if presses % 40 == 39 else "A", hold=4, after=8)
        presses += 1
        if presses > 2000:
            raise Fail("never reached the overworld")
    for _ in range(4):
        emu.press("B", hold=4, after=8)
    wait_field(emu)
    report["checks"]["introPresses"] = presses

    def snap(name):
        path = emu.screenshot(f"{len(report['snaps']):02d}_{name}")
        report["snaps"][name] = os.path.relpath(path, emu.out)

    emu.record = True
    scene(emu, report, "setup", lambda: (emu.command(CMD_SETUP), wait_field(emu)))
    report["dumps"]["setup"] = decode_dump(emu)

    def walk():
        for key in ["DOWN", "RIGHT", "UP", "LEFT"]:
            emu.hold(key, 32)
            emu.run(8)
            snap("walk_" + key.lower())
    scene(emu, report, "walk", walk)

    def pokedex():
        menu_to(emu, "POKEDEX")
        emu.run(90)
        snap("dex_list")
        emu.press("DOWN", after=20)
        emu.press("A", after=120)    # open an entry
        snap("dex_entry")
        emu.press("B", after=60)
        emu.press("B", after=90)
        emu.press("B", after=40)     # close start menu
        wait_field(emu, mash="B")
    scene(emu, report, "pokedex", pokedex)

    def party():
        menu_to(emu, "POKEMON")
        emu.run(60)
        snap("party")
        emu.press("A", after=30)     # first mon -> submenu
        emu.press("A", after=90)     # SUMMARY
        snap("summary_info")
        for page in ("skills", "moves", "page4"):
            emu.press("RIGHT", after=40)
            snap("summary_" + page)
        emu.press("DOWN", after=40)  # next mon
        snap("summary_next_mon")
        emu.press("B", after=80)
        emu.press("B", after=60)
        emu.press("B", after=40)
        wait_field(emu, mash="B")
    scene(emu, report, "party_summary", party)

    def bag():
        menu_to(emu, "BAG")
        emu.run(60)
        snap("bag_pocket0")
        for i in range(4):
            emu.press("RIGHT", after=30)
            snap(f"bag_pocket{i + 1}")
        emu.press("B", after=60)
        emu.press("B", after=40)
        wait_field(emu, mash="B")
    scene(emu, report, "bag", bag)

    def card():
        menu_to(emu, "PLAYER")
        emu.run(60)
        snap("card_front")
        emu.press("A", after=60)     # flip
        snap("card_back")
        emu.press("B", after=80)
        emu.press("B", after=40)
        wait_field(emu, mash="B")
    scene(emu, report, "trainer_card", card)

    def pc():
        emu.command(CMD_PC)
        emu.run(40)
        emu.press("DOWN", after=12)
        emu.press("DOWN", after=12)  # MOVE POKEMON
        emu.press("A", after=120)
        emu.press("RIGHT", after=16)
        emu.press("A", after=30)     # menu on a mon
        snap("pc_mon_menu")
        emu.press("B", after=20)
        emu.press("UP", after=16)    # box title
        emu.press("RIGHT", after=50) # box 2
        emu.press("RIGHT", after=50) # box 3
        snap("pc_box3")
        emu.press("B", after=30)     # "Continue BOX operations?"
        emu.press("DOWN", after=12)  # NO
        emu.press("A", after=120)
        emu.press("B", after=40)     # leave PC menu
        wait_field(emu, mash="B")
    scene(emu, report, "pc", pc)

    def wild():
        emu.command(CMD_WILD)
        frames = 0
        while not emu.in_battle() and frames < 600:
            emu.run(10)
            frames += 10
        if not emu.in_battle():
            raise Fail("wild battle never started")
        for _ in range(300):
            if emu.player_choosing_action():
                break
            emu.press("B", hold=4, after=6)
        snap("wild_action_menu")
        emu.press("A", after=30)
        snap("wild_move_menu")
        wait_field(emu, limit=20000, mash="A")
    scene(emu, report, "wild_battle", wild)
    report["dumps"]["after_wild"] = decode_dump(emu)

    def trainer():
        emu.command(CMD_TRAINER)
        frames = 0
        while not emu.in_battle() and frames < 600:
            emu.run(10)
            frames += 10
        if not emu.in_battle():
            raise Fail("trainer battle never started")
        for _ in range(300):
            if emu.player_choosing_action():
                break
            emu.press("B", hold=4, after=6)
        snap("trainer_action_menu")
        wait_field(emu, limit=30000, mash="A")
    scene(emu, report, "trainer_battle", trainer)
    report["dumps"]["after_trainer"] = decode_dump(emu)

    def catch():
        emu.command(CMD_WILD)
        frames = 0
        while not emu.in_battle() and frames < 600:
            emu.run(10)
            frames += 10
        if not emu.in_battle():
            raise Fail("catch battle never started")
        for attempt in range(6):
            for _ in range(300):                   # advance text to the action menu
                if emu.player_choosing_action() or emu.field_ready():
                    break
                emu.press("B", hold=4, after=6)
            if emu.field_ready():
                return
            emu.press("DOWN", after=10)            # ITEM (battle UI v0.1: FIGHT PKMN / ITEM RUN)
            emu.press("A", after=90)
            pocket_addr = emu.sym["gBagPosition"] + 5
            for _ in range(5):
                if emu.u8(pocket_addr) == 1:       # BALLS_POCKET
                    break
                emu.press("RIGHT", after=30)
            snap("battle_bag")
            emu.press("A", after=20)               # Master Ball
            emu.press("A", after=60)               # USE
            emu.run(400)
            snap("catch_throw")
            for _ in range(150):                   # throw, catch, Pokedex page, nickname: No
                emu.press("B", hold=4, after=6)
                if emu.field_ready():
                    return
        wait_field(emu, limit=3000, mash="B")
    if os.environ.get("REGRESS_STOP_BEFORE") == "catch":
        emu.cmd(f"savestate {os.path.join(emu.out, 'states', 'catch.ss')}")
        return 0
    scene(emu, report, "catch", catch)
    report["dumps"]["after_catch"] = decode_dump(emu)

    def save():
        menu_to(emu, "SAVE")
        snap("save_prompt")
        emu.press("A", after=60)     # yes, save
        emu.run(300)
        emu.press("A", after=40)
        wait_field(emu, mash="B")
    scene(emu, report, "save", save)
    report["dumps"]["saved"] = decode_dump(emu)

    # --- Reset and continue ---------------------------------------------
    emu.record = False
    emu.cmd("reset")
    emu.run(3400)
    presses = 0
    while not emu.in_overworld():
        emu.press("A", hold=4, after=8)
        presses += 1
        if presses > 400:
            raise Fail("continue never reached the overworld")
    wait_field(emu)
    emu.record = True
    scene(emu, report, "continue", lambda: (emu.run(60), snap("continued")))
    report["dumps"]["reloaded"] = decode_dump(emu)

    saved, reloaded = report["dumps"]["saved"], report["dumps"]["reloaded"]
    report["checks"]["saveRoundTrip"] = saved == reloaded
    report["checks"]["noHeapLeaks"] = not report["checks"].get("heapLeaks")
    report["checks"]["partyHealedAfterBattles"] = None
    # Stack depth: Regress.stackLowest sits after dump[] and the resource limits (offset 40 + 2 * 1024 + 28).
    # The system stack top is IWRAM_END - 0x1c0 (src/crt0.s).
    low = emu.u32(emu.rs + 40 + 2 * 1024 + 28)
    report["checks"]["stackDepthBytes"] = (0x3008000 - 0x1c0 - low) if low else None
    failures = []
    for name, entry in report["scenes"].items():
        failures += limit_failures(name, entry["limits"])
    report["checks"]["resourceFailures"] = failures
    with open(os.path.join(args.out, "report.json"), "w") as f:
        json.dump(report, f, indent=1)

    if emu.recordable and shutil.which("ffmpeg"):
        subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", "30",
                        "-i", os.path.join(emu.video_dir, "%06d.png"),
                        "-vf", "scale=720:480:flags=neighbor", "-pix_fmt", "yuv420p",
                        os.path.join(args.out, "run.mp4")], check=True)
        shutil.rmtree(emu.video_dir)
    emu.cmd("quit")
    print("save round trip:", report["checks"]["saveRoundTrip"])
    print("stack depth (bytes):", report["checks"]["stackDepthBytes"])
    for f in failures:
        print("RESOURCE FAILURE:", f)
    print("heap back to its baseline after every scene:", report["checks"]["noHeapLeaks"],
          report["checks"].get("heapLeaks", ""))
    return 0 if report["checks"]["saveRoundTrip"] and report["checks"]["noHeapLeaks"] and not failures else 1


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Fail as e:
        print("FAIL:", e)
        sys.exit(2)
