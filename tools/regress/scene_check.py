#!/usr/bin/env python3
"""Heap check for the screens the main run doesn't reach: evolution, nickname
screen, egg hatch, in-game trade, Hall of Fame + credits, double battle,
Substitute plus the party menu and summary inside a battle, the Pokédex area
map and the bedroom PC, plus power-on through the intro. Each scene starts from the same savestate (new game + regress
setup) and reports its heap peak, alloc failures and DMA queue peak, with
screenshots, plus the bytes it leaves on the heap once back in the overworld
(leak, must be 0) and any heap error that stopped the game. Usage: scene_check.py ROM ELF OUTDIR [--video]
(ROM from make release REGRESS=1 DINFO=1). Also reports each scene's OBJ tile, palette,
sprite and object event use, and exits 1 if any sprite, palette, tile or window allocation failed."""
import sys, os, json, shutil, subprocess
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R

rom, elf, out = sys.argv[1:4]
video = "--video" in sys.argv
if os.path.exists(out):
    shutil.rmtree(out)
os.makedirs(out)
emu = R.Emu(rom, elf, out, video)
emu.run(3400)
n = 0
while not emu.in_overworld():
    emu.press("START" if n % 40 == 39 else "A", hold=4, after=8); n += 1
for _ in range(4): emu.press("B", hold=4, after=8)
R.wait_field(emu)
boot = emu.regress()   # stats count from power-on: title, Birch's intro, truck
print(f"{'boot_intro':22s} heap {boot['heapPeak']:6d} fails {boot['heapFails']} dma {boot['dmaPeak']} full {boot['dmaFull']}", flush=True)
failures = []
boot_limits = emu.limits()
print(f"{'':22s} {R.limits_text(boot_limits)}", flush=True)
failures += R.limit_failures("boot_intro", boot_limits)
emu.command(R.CMD_SETUP); R.wait_field(emu)
base_state = os.path.join(out, "base.ss")
emu.cmd(f"savestate {base_state}")
heap_baseline = {}                # what each scene must leave on the heap (leak check), by pop-up state
used, popup = emu.heap_used()
heap_baseline[popup] = used
emu.record = video

report = {"boot_intro": dict(heapPeak=boot["heapPeak"], heapFails=boot["heapFails"], dmaPeak=boot["dmaPeak"],
                             dmaFull=boot["dmaFull"], backToField=True)}
shots = []


def snap(name):
    shots.append(emu.screenshot(name))


def mash_until_field(key, limit, every=8):
    for i in range(limit // every):
        if emu.field_ready():
            emu.run(10)
            if emu.field_ready():
                return True
        emu.press(key, hold=4, after=every - 4)
    return False


def wait_battle():
    # Frame by frame, so the RNG can be pinned on the first battle frame: the
    # game advances it every frame, and a build that loads the battle a frame
    # sooner would otherwise roll different moves and the screens would drift.
    for f in range(1200):
        if emu.in_battle():
            emu.cmd("keys 0")
            emu.cmd(f"write {emu.sym['gRngValue']:#x} 78563412")
            return
        emu.cmd(f"keys {R.KEYS['A'] if f % 10 < 4 else 0}")   # intro text, if any
        emu.run(1)
    raise R.Fail("battle never started")


def wait_action_menu(limit=300):
    for _ in range(limit):
        if emu.player_choosing_action():
            return True
        emu.press("B", hold=4, after=6)
    return False


def evolution():
    emu.command(R.CMD_SCENE, arg=R.SCENE_EVOLUTION)
    emu.run(400); snap("evo_start")
    emu.run(600); snap("evo_mid")
    return mash_until_field("A", 8000)


def naming():
    emu.command(R.CMD_SCENE, arg=R.SCENE_NAMING)
    emu.run(120); snap("naming")
    emu.press("A", after=30)      # type a letter
    emu.press("START", after=30)  # jump to OK
    emu.press("A", after=60)
    return mash_until_field("B", 2000)


def egg_hatch():
    emu.command(R.CMD_SCENE, arg=R.SCENE_EGG_HATCH)
    emu.run(300); snap("egg_start")
    emu.run(500); snap("egg_hatching")
    return mash_until_field("B", 6000)   # B: no nickname


def ingame_trade():
    emu.command(R.CMD_SCENE, arg=R.SCENE_INGAME_TRADE)
    emu.run(600); snap("trade_start")
    emu.run(900); snap("trade_mid")
    return mash_until_field("A", 12000)


def hall_of_fame():
    emu.command(R.CMD_SCENE, arg=R.SCENE_HALL_OF_FAME)
    emu.run(400); snap("hof_start")
    for i in range(60):               # Hall of Fame, then the credits
        emu.press("A", hold=4, after=296)
        if i in (10, 30, 50):
            snap(f"hof_credits_{i}")
    return True


def double_battle():
    emu.command(R.CMD_SCENE, arg=R.SCENE_DOUBLE_BATTLE)
    wait_battle()
    wait_action_menu(); snap("double_action")
    return mash_until_field("A", 40000)


def substitute_and_party():
    emu.command(R.CMD_SCENE, arg=R.SCENE_SUBSTITUTE)
    wait_battle()
    wait_action_menu(); snap("sub_action")
    emu.press("A", after=30)          # FIGHT
    emu.press("A", after=30)          # Substitute
    wait_action_menu(); snap("sub_up")
    emu.press("RIGHT", after=20)      # PKMN
    emu.press("A", after=90); snap("battle_party")
    emu.press("A", after=30)          # first mon -> submenu
    emu.press("DOWN", after=20)       # SUMMARY
    emu.press("A", after=120); snap("battle_summary")
    emu.press("RIGHT", after=40)
    emu.press("B", after=90)
    emu.press("B", after=30)
    emu.press("B", after=90)
    wait_action_menu(); snap("back_to_action")
    emu.press("DOWN", after=20)       # RUN
    emu.press("RIGHT", after=20)
    emu.press("A", after=60)
    return mash_until_field("B", 6000)


def dex_area():
    R.menu_to(emu, "POKEDEX")
    emu.run(150)
    for _ in range(66):               # scroll down the list, one entry per press
        emu.press("DOWN", hold=2, after=12)
    emu.run(30); snap("dex_list")     # lands on No.067 Machoke, seen
    emu.press("A", after=150)
    snap("dex_entry")
    emu.press("A", after=150); snap("dex_area")       # AREA is the first tab
    for _ in range(4):
        emu.press("B", after=60)
    return mash_until_field("B", 2000)


def bedroom_pc():
    emu.command(R.CMD_SCENE, arg=R.SCENE_BEDROOM_PC)
    emu.run(120); snap("pc_menu")
    emu.press("A", after=60)          # ITEM STORAGE
    emu.press("A", after=120); snap("pc_withdraw")
    emu.press("B", after=60)
    emu.press("DOWN", after=20)       # DEPOSIT ITEM: opens the bag
    emu.press("A", after=150); snap("pc_deposit_bag")
    emu.press("B", after=120)
    return mash_until_field("B", 2000)


def pokenav():
    emu.command(R.CMD_SCENE, arg=R.SCENE_POKENAV)
    emu.run(240); snap("pokenav_menu")
    emu.press("A", after=200); snap("pokenav_map")
    emu.press("B", after=150)
    emu.press("DOWN", after=30)
    emu.press("A", after=150); snap("pokenav_condition_menu")
    emu.press("A", after=240); snap("pokenav_condition_party")   # PARTY: graphs + mon pic
    emu.press("B", after=150)
    emu.press("B", after=150)
    emu.press("B", after=150)                                    # switch off
    return mash_until_field("B", 3000)


def save_failed(arg, name):
    emu.command(R.CMD_SCENE, arg=arg)
    emu.run(150); snap(name)
    emu.run(600); snap(name + "_done")  # check completed, saved again
    emu.press("A", after=60)
    return mash_until_field("B", 2000)


def save_failed_normal():
    return save_failed(R.SCENE_SAVE_FAILED, "save_failed")


def save_failed_hof():
    ok = save_failed(R.SCENE_SAVE_FAILED_HOF, "save_failed_hof")
    emu.command(R.CMD_SCENE, arg=R.SCENE_LOAD_HOF)
    emu.run(10)
    got = emu.read(emu.sym["gDecompressionBuffer"], 2 * 3968)   # 2 sectors of save data
    want = bytes((i * 7 + 3) & 0xFF for i in range(len(got)))
    report["hof_record_intact"] = got == want
    print("hall of fame record saved intact:", got == want, flush=True)
    return ok and got == want


SCENES = [("evolution", evolution), ("naming", naming), ("egg_hatch", egg_hatch),
          ("ingame_trade", ingame_trade), ("double_battle", double_battle),
          ("substitute_party", substitute_and_party), ("dex_area", dex_area), ("bedroom_pc", bedroom_pc),
          ("pokenav", pokenav), ("save_failed", save_failed_normal), ("save_failed_hof", save_failed_hof),
          ("hall_of_fame_credits", hall_of_fame)]
only = [a for a in sys.argv[4:] if not a.startswith("--")]
for name, fn in SCENES:
    if only and name not in only:
        continue
    emu.cmd(f"loadstate {base_state}")
    emu.run(2)
    emu.reset_stats()
    ok = fn()
    fatal = emu.heap_fatal()
    st = emu.regress()
    snap(f"{name}_end")
    leak = None
    if ok and not fatal and emu.field_ready():
        used, popup = emu.heap_used()
        leak = used - heap_baseline.setdefault(popup, used)
    report[name] = dict(heapPeak=st["heapPeak"], heapFails=st["heapFails"], dmaPeak=st["dmaPeak"],
                        dmaFull=st["dmaFull"], backToField=bool(ok), heapFatal=fatal, heapLeak=leak,
                        limits=emu.limits())
    failures += R.limit_failures(name, report[name]["limits"])
    print(f"{name:22s} heap {st['heapPeak']:6d} fails {st['heapFails']} dma {st['dmaPeak']} full {st['dmaFull']} field {ok}"
          f" leak {leak}" + (f" HEAP ERROR {fatal}" if fatal else ""), flush=True)
    print(f"{'':22s} {R.limits_text(report[name]['limits'])}", flush=True)

with open(os.path.join(out, "scenes.json"), "w") as f:
    json.dump(report, f, indent=1)
if video and shutil.which("ffmpeg"):
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y", "-framerate", "30",
                    "-i", os.path.join(emu.video_dir, "%06d.png"),
                    "-vf", "scale=720:480:flags=neighbor", "-pix_fmt", "yuv420p",
                    os.path.join(out, "scenes.mp4")], check=True)
    shutil.rmtree(emu.video_dir)
emu.cmd("quit")
report["boot_intro"]["limits"] = boot_limits
with open(os.path.join(out, "scenes.json"), "w") as f:
    json.dump(report, f, indent=1)
for f in failures:
    print("RESOURCE FAILURE:", f)
bad = [n for n, r in report.items() if isinstance(r, dict) and (r.get("heapFatal") or r.get("heapLeak"))]
print("heap errors or leaks:", ", ".join(bad) if bad else "none")
sys.exit(1 if bad or failures else 0)
