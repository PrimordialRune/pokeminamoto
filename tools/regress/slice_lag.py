#!/usr/bin/env python3
"""Lag, inn and arena profile of the Masara-to-Nibi slice (ROM from make release REGRESS=1 DINFO=1).

    python3 tools/regress/slice_lag.py --out DIR [--rom ROM] [--elf ELF]
        [--sections walk,inn,arena] [--maps A,B] [--hours 7,12,22] [--frames N] [--norTC]

Boots a new game, runs REGRESS setup and then, per section:

  walk   every outdoor slice map (MasaraTown, Route1, ViridianCity, Route22, Route2, ViridianForest,
         PewterCity, Route3) in MORN (hour 7), DAY (12) and NITE (22): the player is warped in, the clock
         is forced with REGRESS_CMD_SET_HOUR first so the map loads with that phase's palettes, then walks
         (and runs) random far-apart goals on the map's collision grid for --frames frames each.
  inn    both inns (Tokaido-ya, Akagi-ya): a UME rest and a TAKE rest, mashing A through the keeper's
         dialogue, the fade and the fanfare.
  arena  Yujirou and Takeshi: the real fight, mashing A through the intro, every turn's messages and the
         badge scene; a fight that outlasts the cap is forced to a win (REGRESS_CMD_WIN_NEXT_BATTLE).

Per scene it reports what src/regress.c counts: frames, lag frames (VBlanks the main loop missed), scanlines
of a frame the main loop used (avg/max, 228 per frame; the budget is 228), heap peak and sprite/window
failures. --norTC sets the RTC absent first (the ROM's game code is patched so the emulator does not
attach the cartridge clock); the day/night phase and the timed NPCs then run on play time.
Writes DIR/slice_lag.json and prints a table. Map walking is modelled on studio walkvid.py.
"""
import argparse
import json
import os
import random
import re
import shutil
import struct
import subprocess
import sys
from collections import deque

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402

ROOT = R.ROOT
DIRV = {"UP": (0, -1), "DOWN": (0, 1), "LEFT": (-1, 0), "RIGHT": (1, 0)}
CMD_SET_FLAG, CMD_SET_VAR, CMD_WARP, CMD_WIN_NEXT, CMD_SET_HOUR, CMD_RUN_SCRIPT = 11, 12, 13, 14, 15, 16
CMDARG_OFF = 2144
WIN_NEXT_OFF = 2153
WIN_FORCED_OFF = 2154
HOURS = {"MORN": 7, "DAY": 12, "NITE": 22}
WALK_MAPS = ["MasaraTown", "Route1", "ViridianCity", "Route22", "Route2", "ViridianForest", "PewterCity", "Route3"]


# ---------------------------------------------------------------- constants and map grids
def consts(names):
    """Numeric value of each flag/var constant, by running the C preprocessor over the headers."""
    src = ('#include "constants/flags.h"\n#include "constants/vars.h"\n#include "constants/slice/shared.h"\n'
           + "".join(f"@@ {n}\n" for n in names))
    out = subprocess.run(["arm-none-eabi-gcc", "-E", "-P", "-x", "c", "-I", os.path.join(ROOT, "include"), "-"],
                         input=src, capture_output=True, text=True, check=True).stdout
    exprs = [l[3:] for l in out.splitlines() if l.startswith("@@")]
    return {n: eval(e) for n, e in zip(names, exprs)}


_groups = json.load(open(os.path.join(ROOT, "data/maps/map_groups.json")))
MAP_IDS = {m: (gi, ni) for gi, gn in enumerate(_groups["group_order"]) for ni, m in enumerate(_groups[gn])}
MAP_NAMES = {v: k for k, v in MAP_IDS.items()}

_bnames = re.findall(r"^\s*(MB_\w+)", open(os.path.join(ROOT, "include/constants/metatile_behaviors.h")).read().split("enum", 1)[1], re.M)
_B = {n: i for i, n in enumerate(_bnames)}
JUMP = {_B["MB_JUMP_EAST"]: (1, 0), _B["MB_JUMP_WEST"]: (-1, 0), _B["MB_JUMP_NORTH"]: (0, -1), _B["MB_JUMP_SOUTH"]: (0, 1)}
WATER = {_B[n] for n in _B if "WATER" in n and "SHALLOW" not in n and "PUDDLE" not in n}
_hdr = open(os.path.join(ROOT, "src/data/tilesets/headers.h")).read()
_meta = open(os.path.join(ROOT, "src/data/tilesets/metatiles.h")).read()
_layouts = {l["id"]: l for l in json.load(open(os.path.join(ROOT, "data/layouts/layouts.json")))["layouts"] if "id" in l}


def _attrs(ts):
    body = re.search(re.escape(ts) + r" =\s*\{(.*?)\};", _hdr, re.S).group(1)
    s = re.search(r"\.metatileAttributes = (\w+)", body).group(1)
    pth = re.search(re.escape(s) + r'\[\] = INCBIN_U16\("([^"]+)"', _meta).group(1)
    raw = open(os.path.join(ROOT, pth), "rb").read()
    return struct.unpack("<%dH" % (len(raw) // 2), raw)


def grid(m):
    j = json.load(open(os.path.join(ROOT, "data/maps", m, "map.json")))
    L = _layouts[j["layout"]]
    W, H = L["width"], L["height"]
    c = struct.unpack("<%dH" % (W * H), open(os.path.join(ROOT, L["blockdata_filepath"]), "rb").read())
    P, S = _attrs(L["primary_tileset"]), _attrs(L["secondary_tileset"])
    beh = [((P[v & 0x3FF] if (v & 0x3FF) < 512 else S[(v & 0x3FF) - 512]) & 0xFF) for v in c]
    col = [(v >> 10) & 3 for v in c]
    objs = {(o["x"], o["y"]) for o in j.get("object_events", [])}
    # Warps and script triggers end a walk loop (a map change, a cutscene): keep off them
    trig = {(w["x"], w["y"]) for w in j.get("warp_events", [])}
    trig |= {(c_["x"], c_["y"]) for c_ in j.get("coord_events", [])}
    return W, H, col, beh, objs, trig


def reachable(m, start, avoid=()):
    """Distances (steps) from start over walkable tiles, and the predecessor map."""
    W, H, col, beh, objs, trig = grid(m)
    block = objs | trig | set(avoid)
    prev = {start: None}
    dist = {start: 0}
    q = deque([start])
    while q:
        c = q.popleft()
        for k, (dx, dy) in DIRV.items():
            nx, ny = c[0] + dx, c[1] + dy
            if not (0 <= nx < W and 0 <= ny < H) or (nx, ny) in prev:
                continue
            b = beh[ny * W + nx]
            if b in JUMP or b in WATER or col[ny * W + nx] != 0 or (nx, ny) in block:
                continue
            prev[(nx, ny)] = (c, k)
            dist[(nx, ny)] = dist[c] + 1
            q.append((nx, ny))
    return dist, prev


def path_to(prev, goal):
    out, c = [], goal
    while prev[c]:
        c, k = prev[c]
        out.append(k)
    return out[::-1]


def start_tile(m):
    """A walkable tile in the largest open region, away from objects and triggers."""
    W, H, col, beh, objs, trig = grid(m)
    best = None
    seen = set()
    for y in range(H):
        for x in range(W):
            t = (x, y)
            if t in seen or col[y * W + x] != 0 or t in objs or t in trig or beh[y * W + x] in WATER or beh[y * W + x] in JUMP:
                continue
            dist, _ = reachable(m, t)
            seen |= set(dist)
            if best is None or len(dist) > best[0]:
                best = (len(dist), t)
    return best[1], best[0]


# ---------------------------------------------------------------- emulator helpers
class Rig:
    def __init__(self, rom, elf, out, no_rtc=False):
        self.out = out
        if no_rtc:
            # mGBA attaches its clock to the GPIO port whatever the header says, so the ROM is patched
            # instead: the SiiRtc read routines (Thumb code) return 0 at once, as on a cart whose port reads
            # nothing. RtcInit then sees the probe fail (sErrorStatus = RTC_INIT_ERROR) and every later
            # read leaves the caller's struct as it was.
            patched = os.path.join(out, "no_rtc.gba")
            data = bytearray(open(rom, "rb").read())
            nm = subprocess.run(["arm-none-eabi-nm", elf], capture_output=True, text=True, check=True).stdout
            for line in nm.splitlines():
                p = line.split()
                if len(p) == 3 and p[2] in ("SiiRtcGetStatus", "SiiRtcGetDateTime", "SiiRtcGetTime", "SiiRtcGetDate"):
                    off = int(p[0], 16) - 0x08000000
                    data[off:off + 4] = struct.pack("<HH", 0x2000, 0x4770)   # movs r0, #0 ; bx lr (Thumb)
            open(patched, "wb").write(data)
            rom = patched
        self.emu = R.Emu(rom, elf, out, False)
        self.sb1 = None
        self.sym = self.emu.sym

    # --- raw memory ---
    def rs_write(self, off, data):
        self.emu.cmd(f"write {self.emu.rs + off:#x} {data.hex()}")

    def where(self):
        e = self.emu
        sb = e.u32(e.sym["gSaveBlock1Ptr"])
        x, y = struct.unpack("<hh", e.read(sb, 4))
        loc = struct.unpack("<bb", e.read(sb + 4, 2))
        return MAP_NAMES.get(loc, str(loc)), x, y

    # --- regress commands with arguments ---
    def cmd_args(self, cmd, a0=0, a1=0, a2=0, a3=0, arg=0, wait=900):
        self.rs_write(CMDARG_OFF, struct.pack("<4H", a0, a1, a2, a3))
        self.emu.command(cmd, arg=arg, wait=wait)

    def set_flag(self, flag, on=1):
        self.cmd_args(CMD_SET_FLAG, flag, on)

    def get_var(self, var):
        e = self.emu
        sb = e.u32(e.sym["gSaveBlock1Ptr"])
        return struct.unpack("<H", e.read(sb + 0x106C + (var - 0x4000) * 2, 2))[0]

    def where_code(self):
        """Names of the code the game is in: callback2 and, in a battle, the battle main/controller functions."""
        e = self.emu
        names = []
        for label, addr in (("cb2", e.callback2()), ("battleMain", e.u32(e.sym["gBattleMainFunc"]) & ~1),
                            ("ctrl0", e.u32(e.sym["gBattlerControllerFuncs"]) & ~1)):
            names.append(f"{label}={self.symname(addr)}")
        return " ".join(names)

    def symname(self, addr):
        if not hasattr(self, "_by"):
            self._by = sorted((a, n) for n, aa in self.emu.all_syms.items() for a in aa)
        import bisect
        i = bisect.bisect_right(self._by, (addr, "\uffff")) - 1
        return self._by[i][1] if i >= 0 else hex(addr)

    def set_var(self, var, val):
        self.cmd_args(CMD_SET_VAR, var, val)

    def set_hour(self, hour):
        self.cmd_args(CMD_SET_HOUR, arg=hour)

    def warp(self, m, x, y):
        gi, ni = MAP_IDS[m]
        self.cmd_args(CMD_WARP, gi, ni, x, y)
        R.wait_field(self.emu, limit=3600, mash="B")    # B closes an arrival script's message, if one runs
        self.emu.run(30)

    def arm_win(self):
        self.rs_write(WIN_NEXT_OFF, b"\x01")

    def forced_wins(self):
        return self.emu.u8(self.emu.rs + WIN_FORCED_OFF)

    # --- measuring ---
    def begin(self):
        self.emu.reset_stats()

    def end(self):
        e = self.emu
        e.check_fatal()
        st = e.regress()
        lim = e.limits()
        frames = max(st["frames"], 1)
        return dict(frames=st["frames"], lag=st["lagFrames"], busyAvg=round(st["busySum"] / frames, 1),
                    busyMax=st["busyMax"], heapPeak=st["heapPeak"], heapFails=st["heapFails"], dmaFull=st["dmaFull"],
                    fails=sum(lim[k] for k in R.LIMIT_FAILS), sprites=lim["spritesMax"], objevents=lim["objEventsMax"])

    # --- walking ---
    def step(self, key, run_shoes):
        e = self.emu
        before = self.where()
        mask = R.KEYS[key] | (R.KEYS["B"] if run_shoes else 0)
        e.cmd(f"keys {mask}")
        for _ in range(12):
            e.run(2)
            if self.where() != before:
                break
        e.cmd("keys 0")
        return self.where() != before

    def recover(self, tag, note):
        """Back to the field after a battle or a script took over. The counters of that detour are
        dropped (a battle is measured by its own section); the note records it."""
        e = self.emu
        e.cmd("keys 0")
        saved = e.read(e.rs + 8, 28)
        kind = "script/other"
        for i in range(1200):
            if e.field_ready():
                e.run(10)
                if e.field_ready():
                    break
            if e.in_battle():
                if kind != "battle":
                    kind = "battle"
                    self.arm_win()
                e.press("A", hold=4, after=6)
            else:
                e.press("B", hold=4, after=6)
            if i % 10 == 0:
                e.check_fatal()
        else:
            e.screenshot("FAIL_recover")
            raise R.Fail(f"{tag}: field never came back after a {kind} in {self.where()[0]}")
        note.append(f"{tag}: {kind} ({self.where()[0]})")
        e.cmd(f"write {e.rs + 8:#x} {saved.hex()}")

    def walk(self, m, frames, run_shoes, rng, note):
        e = self.emu
        steps = 0
        start = e.regress()["frames"]
        sidx = 0
        while e.regress()["frames"] - start < frames:
            cur = self.where()
            if cur[0] != m:
                note.append(f"left {m} for {cur[0]}")
                return steps, False
            dist, prev = reachable(m, (cur[1], cur[2]))
            far = [t for t, d in dist.items() if d >= 8]
            if not far:
                note.append("no goals")
                return steps, False
            goal = rng.choice(far)
            for k in path_to(prev, goal):
                if e.regress()["frames"] - start >= frames:
                    break
                if not self.step(k, run_shoes):
                    if not e.field_ready():
                        self.recover("walk", note)
                        break
                    e.run(20)
                    if not e.field_ready():
                        break
                    break
                steps += 1
                if not e.field_ready():
                    self.recover("walk", note)
                    break
            sidx += 1
            if sidx > 200:
                break
        e.run(8)
        if not e.field_ready():
            self.recover("walk-end", note)
        return steps, True


def boot(rig, no_rtc):
    e = rig.emu
    e.run(3400)
    n = 0
    while not e.in_overworld():
        e.press("START" if n % 40 == 39 else "A", hold=4, after=8)
        n += 1
        if n > 2000:
            raise R.Fail("never reached the overworld")
    for _ in range(4):
        e.press("B", hold=4, after=8)
    R.wait_field(e)
    e.command(R.CMD_SETUP)
    R.wait_field(e)
    e.run(60)


def rtc_state(rig):
    e = rig.emu
    s = e.sym
    return dict(errorStatus=struct.unpack("<H", e.read(s["sErrorStatus"], 2))[0],
                probe=e.u8(s["sProbeResult"]), hourFromRtc=e.u8(s["sHourFromRtc"]),
                hourPlusOne=e.u8(s["sHourPlusOne"]), timeOfDay=e.u8(s["sTimeOfDay"]))


def row(name, r, extra=""):
    print(f"  {name:34s} frames {r['frames']:5d} lag {r['lag']:3d} busy {r['busyAvg']:5.1f}/{r['busyMax']:3d} "
          f"heap {r['heapPeak']:6d} fails {r['fails']} spr {r['sprites']:3d} obj {r['objevents']:2d} {extra}", flush=True)


# ---------------------------------------------------------------- sections
def sec_walk(rig, args, report):
    rng = random.Random(1)
    maps = args.maps.split(",") if args.maps else WALK_MAPS
    hours = [h for h in args.hours.split(",")] if args.hours else ["MORN", "DAY", "NITE"]
    rig.set_var(CONSTS["VAR_REPEL_STEP_COUNT"], 255)
    res = report.setdefault("walk", {})
    for m in maps:
        st, size = start_tile(m)
        for hname in hours:
            hour = HOURS.get(hname, None)
            if hour is None:
                hour = int(hname)
            rig.set_hour(hour)
            rig.warp(m, *st)
            for mode, shoes in (("walk", False), ("run", True)):
                rig.set_var(CONSTS["VAR_REPEL_STEP_COUNT"], 255)
                rig.begin()
                note = []
                steps, ok = rig.walk(m, args.frames, shoes, rng, note)
                r = rig.end()
                r.update(steps=steps, notes=note, tod=rtc_state(rig)["timeOfDay"])
                res[f"{m}/{hname}/{mode}"] = r
                row(f"{m} {hname} {mode}", r, f"steps {steps} {' '.join(note)}")
                if not ok:
                    rig.warp(m, *st)
    rig.set_hour(0xFF)


def talk(rig, key_plan, frames, name, report_key, report, win_after=None):
    """Mash A (with extra keys from key_plan) until the field is ready again, measuring."""
    e = rig.emu
    rig.begin()
    start = e.regress()["frames"]
    n = 0
    forced = False
    seen_battle = False
    lagwas = e.regress()["lagFrames"]
    events = []
    while True:
        for k in key_plan(n):
            e.press(k, hold=4, after=10)
        n += 1
        lagnow = e.regress()["lagFrames"]
        if lagnow != lagwas:
            events.append(dict(frame=e.regress()["frames"] - start, lag=lagnow - lagwas, at=rig.where_code()))
            lagwas = lagnow
        if e.in_battle():
            seen_battle = True
        if win_after and seen_battle and not forced and e.regress()["frames"] - start > win_after:
            rig.arm_win()
            forced = True
        if e.field_ready() and n > 6:
            e.run(20)
            if e.field_ready():
                break
        if e.regress()["frames"] - start > frames:
            e.screenshot("FAIL_" + name.replace(" ", "_"))
            raise R.Fail(f"{name}: field never came back")
        if n % 20 == 0:
            e.check_fatal()
    r = rig.end()
    r["forcedWin"] = bool(forced)
    r["lagEvents"] = events
    r["rests"] = rig.get_var(CONSTS["VAR_REST_COUNT"])
    for ev in events[:12]:
        print(f"      lag +{ev['lag']} at frame {ev['frame']}: {ev['at']}", flush=True)
    report[report_key] = r
    row(name, r, ("(win forced) " if forced else "") + f"rests {r['rests']}")
    return r


def sec_inn(rig, args, report):
    res = report.setdefault("inn", {})
    inns = [("TokaidoYa", "ViridianCity_PokemonCenter_1F", (7, 3), "FLAG_Q_CARD"),
            ("AkagiYa", "PewterCity_PokemonCenter_1F", (7, 3), "FLAG_Q_CARD")]
    rig.set_var(CONSTS["VAR_CHARISMA"], 12)
    for name, m, (x, y), _ in inns:
        for hname, hour in HOURS.items():
            for room, downs in (("UME", 0), ("TAKE", 1)):
                rig.set_hour(hour)
                rig.warp(m, x, y)
                rig.emu.press("UP", hold=4, after=10)   # face the keeper
                plan = lambda n, d=downs: ["DOWN"] * d if n == 4 and d else []
                # A: talk, yes; room list; pick; messages; fade; fanfare; messages
                talk(rig, lambda n, d=downs: (["DOWN"] if (n == 3 and d) else []) + ["A"], 2400,
                     f"inn {name} {hname} {room}", f"{name}/{hname}/{room}", res)
    rig.set_hour(0xFF)


def sec_arena(rig, args, report):
    res = report.setdefault("arena", {})
    C = CONSTS
    rig.set_flag(C["FLAG_Q_CARD"])
    rig.set_flag(C["FLAG_Q_YUJI_MET"])
    rig.set_flag(C["FLAG_Q_YUJI_SLEPT"])
    for name, m, pos, face, var in (("Yujirou", "ViridianCity_Gym", (2, 3), "UP", "VAR_BATTLE_ARENA"),
                                    ("Takeshi", "PewterCity_Gym", (6, 4), "UP", "VAR_BATTLE_ARENA")):
        for hname, hour in (("DAY", 12),):
            rig.set_hour(hour)
            rig.warp(m, *pos)
            rig.emu.press(face, hold=4, after=10)
            talk(rig, lambda n: ["A"], 7200, f"arena {name} {hname}", f"{name}/{hname}", res, win_after=3600)
    rig.set_hour(0xFF)


def sec_battle(rig, args, report):
    """Reference: the plain wild and trainer battles of the REGRESS setup, same mashing as the arena fights."""
    res = report.setdefault("battle", {})
    e = rig.emu
    for name, c in (("wild", R.CMD_WILD), ("trainer", R.CMD_TRAINER)):
        e.command(R.CMD_HEAL)
        R.wait_field(e)
        rig.begin()
        e.cmd(f"write {e.rs + 4:#x} {c:02x}")
        talk(rig, lambda n: ["A"], 7200, f"battle {name}", name, res, win_after=3000)


def sec_transit(rig, args, report):
    """Map seams (connections) and doors: walk across and back, lag frames per crossing."""
    import json as _json
    res = report.setdefault("transit", {})
    e = rig.emu
    rig.set_hour(12)
    rig.set_var(CONSTS["VAR_REPEL_STEP_COUNT"], 255)
    for m in (args.maps.split(",") if args.maps else WALK_MAPS):
        j = _json.load(open(os.path.join(ROOT, "data/maps", m, "map.json")))
        W, H, col, beh, objs, trig = grid(m)
        st, _ = start_tile(m)
        targets = []
        for c in j.get("connections") or []:
            targets.append((f"seam {c['direction']}", c["direction"].upper(), None))
        for w in j.get("warp_events", []):
            targets.append((f"door ({w['x']},{w['y']}) -> {w['dest_map'][4:]}", None, (w["x"], w["y"])))
        seen_doors = set()
        def one_target(label, edge, door):
                if door:
                    key = (door[0] // 2, door[1], label.split("-> ")[1])
                    if key in seen_doors:
                        return
                    seen_doors.add(key)
                if not e.field_ready():
                    rig.recover("transit", [])
                rig.warp(m, *st)
                dist, prev = reachable(m, st)
                goal = push = None
                if edge:
                    dx, dy = DIRV[edge]
                    edge_tiles = [t for t in dist if (dy < 0 and t[1] == 0) or (dy > 0 and t[1] == H - 1)
                                  or (dx < 0 and t[0] == 0) or (dx > 0 and t[0] == W - 1)]
                    if edge_tiles:
                        goal, push = min(edge_tiles, key=lambda t: dist[t]), edge
                    else:
                        # The seam is not reachable from the start tile (another region of the map): start beside it
                        cand = [(x, y) for y in range(H) for x in range(W)
                                if ((dy < 0 and y == 0) or (dy > 0 and y == H - 1) or (dx < 0 and x == 0) or (dx > 0 and x == W - 1))
                                and col[y * W + x] == 0 and (x, y) not in objs and (x, y) not in trig
                                and beh[y * W + x] not in WATER and beh[y * W + x] not in JUMP]
                        if cand:
                            goal, push = cand[len(cand) // 2], edge
                            rig.warp(m, *goal)
                            dist, prev = {goal: 0}, {goal: None}
                else:
                    for k, (dx, dy) in DIRV.items():
                        nb = (door[0] - dx, door[1] - dy)
                        if nb in dist and (goal is None or dist[nb] < dist[goal]):
                            goal, push = nb, k
                if goal is None:
                    res[f"{m}/{label}"] = dict(skipped="no reachable approach")
                    print(f"  {m:14s} {label:34s} skipped (no reachable approach)", flush=True)
                    return
                note = []
                for k in path_to(prev, goal):
                    if not rig.step(k, False):
                        break
                rig.begin()
                lag0 = e.regress()["lagFrames"]
                m0 = rig.where()[0]
                for _ in range(4):
                    rig.step(push, False)
                    if rig.where()[0] != m0:
                        break
                for _ in range(30):
                    e.run(10)
                    if e.field_ready():
                        break
                e.run(30)
                m1 = rig.where()[0]
                lag_in = e.regress()["lagFrames"] - lag0
                lag_out = None
                if m1 != m0:
                    lag1 = e.regress()["lagFrames"]
                    back = {"UP": "DOWN", "DOWN": "UP", "LEFT": "RIGHT", "RIGHT": "LEFT"}[push]
                    for key in (["DOWN"] * 3 if door else [back] * 3):
                        rig.step(key, False)
                        if rig.where()[0] == m0:
                            break
                    for _ in range(30):
                        e.run(10)
                        if e.field_ready():
                            break
                    e.run(30)
                    lag_out = e.regress()["lagFrames"] - lag1 if rig.where()[0] == m0 else "stayed in " + rig.where()[0]
                r = rig.end()
                res[f"{m}/{label}"] = dict(to=m1, lagIn=lag_in, lagOut=lag_out, busyMax=r["busyMax"])
                print(f"  {m:14s} {label:34s} -> {m1:30s} lag in {lag_in} out {lag_out} busyMax {r['busyMax']}", flush=True)
                if rig.where()[0] not in (m0,) and not e.field_ready():
                    rig.recover("transit", note)

        for label, edge, door in targets:
            try:
                one_target(label, edge, door)
            except R.Fail as ex:
                res[f"{m}/{label}"] = dict(error=str(ex))
                print(f"  {m:14s} {label:34s} ERROR {ex}", flush=True)
                e.cmd("keys 0")
                R.wait_field(e, limit=3600, mash="B")
    rig.set_hour(0xFF)


SLOT_MAPS = ["MasaraTown", "ViridianCity", "ViridianCity_Mart", "ViridianCity_School", "ViridianCity_PokemonCenter_1F",
             "ViridianCity_PokemonCenter_2F", "PewterCity", "PewterCity_CVS", "PewterCity_Museum_1F",
             "PewterCity_PokemonCenter_1F", "PewterCity_PokemonCenter_2F", "Route1", "Route3"]


def sec_slots(rig, args, report):
    """Every hour 0-23 in the maps with clock-timed NPCs: warp in (so the ON_TRANSITION slots apply) and walk 240
    frames. Reports lag, busy lines and the most object events and sprites alive, i.e. the cost of each slot."""
    res = report.setdefault("slots", {})
    rng = random.Random(7)
    rig.set_var(CONSTS["VAR_REPEL_STEP_COUNT"], 255)
    for m in (args.maps.split(",") if args.maps else SLOT_MAPS):
        st, _ = start_tile(m)
        worst = None
        for hour in range(24):
            rig.set_hour(hour)
            rig.warp(m, *st)
            rig.begin()
            note = []
            rig.walk(m, 240, False, rng, note)
            r = rig.end()
            res[f"{m}/{hour:02d}"] = dict(lag=r["lag"], busyMax=r["busyMax"], objevents=r["objevents"], sprites=r["sprites"], notes=note)
            if r["lag"] or r["fails"]:
                print(f"  {m} hour {hour:02d}: lag {r['lag']} fails {r['fails']} {note}", flush=True)
        rows = [v for k, v in res.items() if k.startswith(m + "/")]
        print(f"  {m:34s} lag {sum(v['lag'] for v in rows):3d} over 24 hours, busyMax {max(v['busyMax'] for v in rows):3d}, "
              f"objevents max {max(v['objevents'] for v in rows):2d}, sprites max {max(v['sprites'] for v in rows):2d}", flush=True)
    rig.set_hour(0xFF)


CONSTS = {}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=os.path.join(ROOT, "pokeemerald_modern_release_regress.gba"))
    ap.add_argument("--elf", default=None)
    ap.add_argument("--out", required=True)
    ap.add_argument("--sections", default="walk,inn,arena")
    ap.add_argument("--maps", default="")
    ap.add_argument("--hours", default="")
    ap.add_argument("--frames", type=int, default=900)
    ap.add_argument("--noRTC", "--norTC", dest="no_rtc", action="store_true")
    args = ap.parse_args()
    elf = args.elf or args.rom.replace(".gba", ".elf")
    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    os.makedirs(args.out)
    CONSTS.update(consts(["VAR_REPEL_STEP_COUNT", "VAR_CHARISMA", "FLAG_Q_CARD", "FLAG_Q_YUJI_MET", "FLAG_Q_YUJI_SLEPT",
                          "FLAG_BADGE01_GET", "FLAG_BADGE02_GET", "VAR_TIME_OF_DAY_OVERRIDE", "VAR_REST_COUNT"]))
    rig = Rig(args.rom, elf, args.out, args.no_rtc)
    boot(rig, args.no_rtc)
    report = dict(rtc=rtc_state(rig), noRtc=args.no_rtc)
    print("RTC state after boot:", report["rtc"], flush=True)
    for s in args.sections.split(","):
        print(f"[{s}]", flush=True)
        dict(walk=sec_walk, inn=sec_inn, arena=sec_arena, battle=sec_battle, transit=sec_transit, slots=sec_slots)[s](rig, args, report)
    with open(os.path.join(args.out, "slice_lag.json"), "w") as f:
        json.dump(report, f, indent=1)
    lag = sum(v.get("lag", 0) for sec in ("walk", "inn", "arena", "slots") for v in report.get(sec, {}).values())
    worst = max((v.get("busyMax", 0) for sec in ("walk", "inn", "arena", "transit", "slots") for v in report.get(sec, {}).values()), default=0)
    print(f"total lag frames {lag}, worst busy lines {worst}/228")
    rig.emu.cmd("quit")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except R.Fail as e:
        print("FAIL:", e)
        sys.exit(2)
