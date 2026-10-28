#!/usr/bin/env python3
"""Shared library for the QA playthrough tooling (studio QA, round 1).

Drives a `make release REGRESS=1 DINFO=1` ROM in tools/gba-emu with real button input:
  * a map reader (collision, metatile behaviours, object events) read from emulator memory,
  * a path finder (walk_to, enter_door, talk_to) that presses real direction keys,
  * a text/menu masher, a battle driver, an event log for every state injection,
  * frame accounting for the timing model (steps, text boxes, battle turns),
  * an optional video recorder (every frame to ffmpeg, x3 nearest neighbour, with sound).

Nothing here edits game code. State injection (REGRESS commands, memory pokes) goes through
Bot.inject(), which logs every use; a playthrough report lists them all.
"""
import glob
import heapq
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.environ.get("QA_ROOT") or os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import run as R  # noqa: E402
from run import Emu, Fail, KEYS  # noqa: E402

ROM = os.environ.get("QA_ROM") or os.path.join(ROOT, "pokeemerald_modern_release_regress.gba")
ELF = os.environ.get("QA_ELF") or os.path.join(ROOT, "pokeemerald_modern_release_regress.elf")

CMD_SET_FLAG, CMD_SET_VAR, CMD_WARP, CMD_WIN_NEXT, CMD_SET_HOUR, CMD_RUN_SCRIPT = 11, 12, 13, 14, 15, 16
CMD_SETUP = 1

# ---------------------------------------------------------------------------------------------
# constants (#define NAME value) from include/constants
# ---------------------------------------------------------------------------------------------
_defs = {}


def _load_defs():
    files = glob.glob(ROOT + "/include/constants/*.h") + glob.glob(ROOT + "/include/constants/slice/*.h")
    for f in files:
        for line in open(f, errors="replace"):
            m = re.match(r"#define\s+([A-Za-z_]\w*)\s+(.+?)\s*(//.*|/\*.*)?$", line)
            if m:
                _defs.setdefault(m.group(1), m.group(2))


def C(name, _d=0):
    if isinstance(name, int):
        return name
    if not _defs:
        _load_defs()
    e = _defs[name]
    e = re.sub(r"/\*.*?\*/", "", e)
    e = re.sub(r"[A-Za-z_]\w*", lambda m: str(C(m.group(0), _d + 1)) if m.group(0) in _defs else m.group(0), e)
    return eval(e)


def map_ids():
    """{MapDirName: (group, num)} from data/maps/map_groups.json."""
    g = json.load(open(ROOT + "/data/maps/map_groups.json"))
    out = {}
    for gi, gname in enumerate(g["group_order"]):
        for ni, mname in enumerate(g[gname]):
            out[mname] = (gi, ni)
    return out


MAPS = map_ids()
MAP_NAME = {v: k for k, v in MAPS.items()}


def _map_consts():
    out = {}
    for name in MAPS:
        try:
            out[mapjson(name)["id"]] = name
        except Exception:
            pass
    return out


def obj_xy(map_name, script_part):
    """(x, y) of the first object_event of `map_name` whose script contains `script_part`."""
    for o in mapjson(map_name).get("object_events", []):
        if script_part in o.get("script", ""):
            return o["x"], o["y"]
    raise KeyError(f"{map_name}: no object with script {script_part}")


def slice_maps():
    names = []
    for line in open("/mnt/project-files/studio/slice-maps.txt"):
        line = line.strip()
        if line.startswith("# conditional"):
            break
        if line and not line.startswith("#"):
            names.append(line)
    return names


_mapjson = {}


def mapjson(name):
    if name not in _mapjson:
        _mapjson[name] = json.load(open(f"{ROOT}/data/maps/{name}/map.json"))
    return _mapjson[name]


# ---------------------------------------------------------------------------------------------
# text decoding (charmap.txt)
# ---------------------------------------------------------------------------------------------
_cm = {}
for _l in open(ROOT + "/charmap.txt", encoding="utf-8"):
    _m = re.match(r"^'(.)'\s*=\s*([0-9A-F]{2})\s*$", _l)
    if _m:
        _cm.setdefault(int(_m.group(2), 16), _m.group(1))


def decode(b):
    out = []
    nargs = {0x04: 3, 0x09: 0, 0x0A: 0, 0x0B: 2, 0x0F: 0, 0x10: 2}      # EXT_CTRL_CODE_* argument bytes
    i = 0
    while i < len(b):
        c = b[i]
        i += 1
        if c == 0xFF:
            break
        if c == 0xFC and i < len(b):
            out.append(" ")
            i += 1 + nargs.get(b[i], 1)       # the code byte and its arguments
            continue
        if c in (0xFE, 0xFA, 0xFB):
            out.append(" ")
            continue
        out.append(_cm.get(c, "\ufffd"))
    return "".join(out)


MAP_CONST = _map_consts()


# metatile behaviours we care about
def _mb_table():
    t = open(ROOT + "/include/constants/metatile_behaviors.h").read()
    names = re.findall(r"^\s*(MB_\w+)", t, re.M)
    return {n: i for i, n in enumerate(names)}


MB = _mb_table()
MB_NAME = {v: k for k, v in MB.items()}
def _blocked_behaviour(name):
    if "DOOR" in name or "ARROW_WARP" in name:
        return False
    return ("WATER" in name or "CURRENT" in name or "WATERFALL" in name or name.startswith("MB_IMPASSABLE")
            or name in ("MB_ICE", "MB_CRACKED_ICE", "MB_CRACKED_FLOOR", "MB_ISOLATED_VERTICAL_RAIL"))


JUMP = {MB["MB_JUMP_EAST"]: (1, 0), MB["MB_JUMP_WEST"]: (-1, 0), MB["MB_JUMP_NORTH"]: (0, -1), MB["MB_JUMP_SOUTH"]: (0, 1)}
SLIDE = {v for k, v in MB.items() if k.startswith("MB_SLIDE") or k.startswith("MB_SPIN")}
BLOCKED_BEH = {i for i, n in MB_NAME.items() if _blocked_behaviour(n)}
GRASS = {MB["MB_TALL_GRASS"], MB["MB_LONG_GRASS"]}
DIRV = {"UP": (0, -1), "DOWN": (0, 1), "LEFT": (-1, 0), "RIGHT": (1, 0)}
VDIR = {v: k for k, v in DIRV.items()}


class BlockedPath(Exception):
    pass


class Recorder:
    """Streams every emulated frame to ffmpeg as a PNG, with the game's audio."""

    def __init__(self, emu, out, tmpdir, fps=59.7275):
        self.emu = emu
        self.out = out
        self.tmp = tmpdir
        self.raw = os.path.join(tmpdir, "audio.raw")
        self.png = os.path.join("/dev/shm" if os.path.isdir("/dev/shm") else tmpdir, f"qa_frame_{os.getpid()}.png")
        self.vid = os.path.join(tmpdir, "video.mkv")
        self.ff = subprocess.Popen(["ffmpeg", "-y", "-loglevel", "error", "-f", "image2pipe", "-framerate", f"{fps}",
                                    "-c:v", "png", "-i", "-", "-vf", "scale=720:480:flags=neighbor",
                                    "-pix_fmt", "yuv420p", "-c:v", "libx264", "-preset", "veryfast", "-crf", "23",
                                    self.vid], stdin=subprocess.PIPE)
        emu.cmd(f"audio {self.raw}")
        self.frames = 0

    def frame(self):
        self.emu.cmd(f"screenshot {self.png}")
        with open(self.png, "rb") as f:
            self.ff.stdin.write(f.read())
        self.frames += 1

    def close(self):
        self.emu.cmd("audio off")
        self.ff.stdin.close()
        self.ff.wait()
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", self.vid,
                        "-f", "s16le", "-ar", "32768", "-ac", "2", "-i", self.raw,
                        "-c:v", "copy", "-c:a", "aac", "-b:a", "160k", "-shortest",
                        "-movflags", "+faststart", self.out], check=True)
        try:
            os.remove(self.png)
        except OSError:
            pass


class Bot:
    """One emulator session plus everything the playthrough needs."""

    def __init__(self, out, rom=ROM, elf=ELF, video=None, shots=None, quiet=False, keep=False):
        if not keep:
            shutil.rmtree(out, ignore_errors=True)
        os.makedirs(out, exist_ok=True)
        self.out = out
        self.emu = Emu(rom, elf, out, False)
        self.rs = self.emu.rs
        self.sym = self.emu.sym
        self.ARG = self.rs + 2144
        self.shots_dir = shots
        if shots:
            os.makedirs(shots, exist_ok=True)
        self.quiet = quiet
        self.log = []                 # human readable event log
        self.injections = []          # every state injection / forced win
        self.checks = []              # (name, ok, detail)
        self.seen = []                # texts seen since last clear (decoded gStringVar4)
        self.cov = {}                 # script-list row id -> evidence string
        self.anomalies = []           # texts with kana or unknown glyphs (a garbled string)
        self.stats = dict(frames=0, steps=0, text_boxes=0, battle_turns=0, battles=0, turn_frames=0)
        self.cat = "other"
        self.cat_frames = {}
        self.seg = None
        self.seg_stats = {}
        self.rec = None
        self._shot_n = 0
        self.blocked = {}             # (mapkey) -> set of (x, y, dx, dy) edges found blocked at run time
        # frame accounting: wrap emu.run
        self._raw_run = self.emu.run
        self.emu.run = self._run
        if video:
            self.rec = Recorder(self.emu, video, out)

    # --- frames ---------------------------------------------------------------------------
    def _run(self, frames):
        if frames <= 0:
            return
        self.stats["frames"] += frames
        self.cat_frames[self.cat] = self.cat_frames.get(self.cat, 0) + frames
        if self.seg is not None:
            self.seg_stats[self.seg]["frames"] += frames
        if self.rec:
            for _ in range(frames):
                self.emu.cmd("run 1")
                self.rec.frame()
        else:
            self.emu.cmd(f"run {frames}")

    def say(self, *a):
        s = " ".join(str(x) for x in a)
        if not self.quiet:
            print(s, flush=True)
        self.log.append(s)

    def check(self, what, got, want=True):
        ok = (got == want)
        self.checks.append((what, ok, "" if ok else f"got {got!r} want {want!r}"))
        self.say(("PASS " if ok else "FAIL ") + what + ("" if ok else f"  (got {got!r}, want {want!r})"))
        return ok

    def cover(self, rid, note):
        """Record that script-list row `rid` was hit, with a one-line evidence note."""
        self.cov.setdefault(rid, []).append(f"{self.seg}: {note}")
        self.say(f"  COVER {rid}: {note}")

    def inject(self, kind, detail):
        rec = dict(seg=self.seg, kind=kind, detail=detail, frame=self.stats["frames"])
        self.injections.append(rec)
        self.say(f"INJECT [{kind}] {detail}")

    def begin(self, name):
        self.seg = name
        self.seg_stats[name] = dict(frames=0, steps=0, text_boxes=0, battle_turns=0, battles=0, start_frame=self.stats["frames"])
        self.say(f"=== segment: {name}")

    # --- memory ---------------------------------------------------------------------------
    def sb1(self):
        return self.emu.u32(self.sym["gSaveBlock1Ptr"])

    def sb2(self):
        return self.emu.u32(self.sym["gSaveBlock2Ptr"])

    def flag(self, n):
        n = C(n)
        return (self.emu.u8(self.sb1() + 3904 + n // 8) >> (n % 8)) & 1

    def var(self, n):
        n = C(n)
        return struct.unpack("<H", self.emu.read(self.sb1() + 4204 + 2 * (n - 0x4000), 2))[0]

    def loc(self):
        g, n = struct.unpack("<bb", self.emu.read(self.sb1() + 4, 2))
        return (g, n)

    def map_name(self):
        return MAP_NAME.get(self.loc(), str(self.loc()))

    def pos(self):
        return struct.unpack("<hh", self.emu.read(self.sb1(), 4))

    def facing(self):
        # player object event is the one with isPlayer; facingDirection low nibble at +0x18
        for o in self.objects(with_player=True):
            if o["player"]:
                return o["facing"]
        return 0

    def money(self):
        key = self.emu.u32(self.sb2() + 108)
        return self.emu.u32(self.sb1() + 1168) ^ key

    def respawn(self):
        g, n, w, _, x, y = struct.unpack("<bbbbhh", self.emu.read(self.sb1() + 28, 8))
        return (g, n, x, y)

    def bag_count(self, item, pockets=((0x560, 30), (0x5d8, 30), (0x650, 16), (0x690, 58))):
        iid = C(item)
        key = self.emu.u32(self.sb2() + 0x6C) & 0xFFFF
        total = 0
        sb1 = self.sb1()
        for off, n in pockets:
            raw = self.emu.read(sb1 + off, 4 * n)
            for i in range(n):
                a, q = struct.unpack_from("<HH", raw, 4 * i)
                if a == iid:
                    total += q ^ key
        return total

    def has_key_item(self, item):
        return self.bag_count(item, ((0x5d8, 30),)) > 0

    # --- party ----------------------------------------------------------------------------
    PARTY_ORDER = None

    def party_count(self):
        return self.emu.u8(self.sym["gPlayerPartyCount"])

    def mon(self, slot, enemy=False):
        base = self.sym["gEnemyParty" if enemy else "gPlayerParty"] + 100 * slot
        raw = self.emu.read(base, 100)
        pers, otid = struct.unpack_from("<II", raw, 0)
        key = pers ^ otid
        words = [w ^ key for w in struct.unpack_from("<12I", raw, 32)]
        import itertools
        order = list(itertools.permutations("GAEM"))[pers % 24]
        sub = {c: words[3 * order.index(c): 3 * order.index(c) + 3] for c in "GAEM"}
        species = sub["G"][0] & 0xFFFF
        moves = [sub["A"][0] & 0xFFFF, sub["A"][0] >> 16, sub["A"][1] & 0xFFFF, sub["A"][1] >> 16]
        status, level, _mail, hp, maxhp = struct.unpack_from("<IBBHH", raw, 80)
        return dict(species=species, level=level, hp=hp, maxhp=maxhp, moves=moves, status=status, addr=base,
                    exp=sub["G"][1], tp=(sub["A"][2] >> 13) & 0x7F)

    def party(self):
        return [self.mon(i) for i in range(self.party_count())]

    def write_u16(self, addr, v):
        self.emu.cmd(f"write {addr:#x} {struct.pack('<H', v).hex()}")

    # --- REGRESS commands (injections) ----------------------------------------------------
    def _cmd(self, c, a=(0, 0, 0, 0), arg=0, wait=900):
        self.emu.cmd(f"write {self.ARG:#x} {struct.pack('<4H', *a).hex()}")
        self.emu.command(c, wait=wait, arg=arg)

    def set_flag(self, f, v=1, why=""):
        self.inject("flag", f"{f if isinstance(f, str) else hex(f)}={v} {why}")
        self._cmd(CMD_SET_FLAG, (C(f), v, 0, 0))

    def set_var(self, v, val, why=""):
        self.inject("var", f"{v if isinstance(v, str) else hex(v)}={val} {why}")
        self._cmd(CMD_SET_VAR, (C(v), val, 0, 0))

    def set_hour(self, h):
        """Clock control is a test setup, logged but not a game-state change."""
        self.inject("hour", f"game hour {h}")
        self._cmd(CMD_SET_HOUR, arg=h)
        self.emu.run(10)

    def workaround(self, bug, name, x, y, why=""):
        """Warp past an open bug (logged as an injection tagged with the bug id)."""
        self.say(f"  WORKAROUND for {bug}: {why}")
        self.warp(name, x, y, why=f"[workaround {bug}] {why}")

    def warp(self, name, x, y, why="", settle=True):
        g, n = MAPS[name]
        self.inject("warp", f"{name} ({x},{y}) {why}")
        self._cmd(CMD_WARP, (g, n, x, y))
        for _ in range(400):
            if self.loc() == (g, n) and self.emu.in_overworld():
                break
            self.emu.run(6)
        self.emu.run(40)
        if settle:
            self.settle()

    def win_next(self, why=""):
        self.inject("win_next_battle", why)
        self._cmd(CMD_WIN_NEXT)

    def heal_party_cheat(self, why=""):
        self.inject("heal", why)
        for i in range(self.party_count()):
            m = self.mon(i)
            self.write_u16(m["addr"] + 0x56, m["maxhp"])
            self.emu.cmd(f"write {m['addr'] + 0x50:#x} 00000000")

    def regress_state(self):
        return self.emu.regress()

    def win_forced(self):
        return self.emu.u8(self.rs + 2154)

    # --- screenshots ----------------------------------------------------------------------
    def shot(self, name):
        p = self.emu.screenshot(name)
        if self.shots_dir:
            shutil.copy(p, os.path.join(self.shots_dir, name + ".png"))
        return p

    # --- boot -----------------------------------------------------------------------------
    def boot_new_game(self):
        """Title screen to the first overworld frame with real input: START, NEW GAME, A through the
        introduction, START on the empty naming screen (the SATOSHI preset), A to the end."""
        e = self.emu
        self.cat = "boot"
        e.run(3400)
        byaddr = {}
        for k, v in e.sym.items():
            byaddr.setdefault(v & ~1, k)
        e.press("START", hold=4, after=60)
        named = False
        for i in range(600):
            if e.in_overworld():
                break
            if byaddr.get(e.callback2()) == "CB2_NamingScreen" and not named:
                e.run(40)
                e.press("START", hold=4, after=30)
                named = True
                continue
            e.press("A", hold=4, after=30)
        for _ in range(4):
            e.press("B", hold=4, after=8)
        R.wait_field(e)
        e.run(30)
        self.cat = "other"

    def ready(self):
        return self.emu.field_ready()

    def last_text(self):
        return decode(self.emu.read(self.sym["gStringVar4"], 400)).strip()

    def note_text(self):
        t = self.last_text()
        if t and (not self.seen or self.seen[-1] != t):
            self.seen.append(t)
            self.say(f"  TEXT: {t[:110]}")
            if re.search("[\u3040-\u30ff\ufffd]", t) and not t.startswith("TP"):
                self.anomalies.append(dict(seg=self.seg, map=self.map_name(), text=t, shot=self.shot(f"anomaly_{len(self.anomalies):02d}")))
                self.say(f"  ANOMALY: odd characters in text: {t!r}")
            self.stats["text_boxes"] += 1
            if self.seg in self.seg_stats:
                self.seg_stats[self.seg]["text_boxes"] += 1
            return True
        return False

    @staticmethod
    def _norm(t):
        return t.lower().replace("\u2019", "'").replace("\u201c", '"').replace("\u201d", '"')

    def said(self, text):
        n = self._norm(text)
        return any(n in self._norm(t) for t in self.seen)

    def texts_since(self, n0):
        return self.seen[n0:]

    # --- input primitives -----------------------------------------------------------------
    def press(self, key, hold=4, after=12):
        self.emu.press(key, hold=hold, after=after)

    def mash(self, cond=None, n=300, key="A", gap=12, shot=None, battle_ok=False):
        """Press `key` until cond() is true (default: field ready). Notes the texts shown."""
        cond = cond or self.ready
        for i in range(n):
            if cond():
                return True
            if not battle_ok and self.emu.in_battle():
                self.run_battle()
                continue
            if key == "A" and self.no_prompt():
                self.emu.press("B", hold=4, after=gap)
            else:
                self.emu.press(key, hold=4, after=gap)
            self.note_text()
            if shot and i % 10 == 0:
                self.shot(f"{shot}_{i:03d}")
        return cond()

    NO_PROMPTS = ("give a nickname",)

    def no_prompt(self):
        t = self.last_text().lower()
        return any(p in t for p in self.NO_PROMPTS)

    auto_care = False
    lose_mode = False         # QA: throw the battle (weakest move, no items, no running) to test the whiteout path
    wild_policy = "auto"      # 'run' flees wild battles (saves TP, as a player short of TP would)

    def care(self):
        """A player's habit after a fight: Antidote when poisoned, Potion when the lead is under half HP.
        Real bag menus; only items the player owns."""
        if not self.auto_care or not self.emu.in_overworld() or not self.ready():
            return False
        done = False
        for slot, m in enumerate(self.party()):
            if not m["hp"]:
                continue
            if m["status"] & 0x88 and self.bag_count("ITEM_ANTIDOTE"):
                ok = self.use_item_on_mon("ITEM_ANTIDOTE", slot)
                self.say(f"  care: Antidote on slot {slot}: {'ok' if ok else 'FAILED'}")
                done = done or ok
            elif m["hp"] < m["maxhp"] * 0.5 and self.bag_count("ITEM_POTION"):
                ok = self.use_item_on_mon("ITEM_POTION", slot)
                self.say(f"  care: Potion on slot {slot}: {'ok' if ok else 'FAILED'}")
                done = done or ok
        return done

    def settle(self, limit=500):
        """Play through whatever is running (texts, scenes, battles) until the player can move."""
        self.cat = "text"
        for _ in range(limit):
            if self.ready():
                self.emu.run(6)
                if self.ready():
                    self.cat = "other"
                    if self.care():
                        self.cat = "other"
                    return True
            if self.emu.in_battle():
                self.run_battle()
                continue
            self.emu.press("B" if self.no_prompt() else "A", hold=4, after=12)
            self.note_text()
        self.emu.check_fatal()
        self.cat = "other"
        return False

    # --- map ------------------------------------------------------------------------------
    _attr_cache = {}

    def _attrs(self):
        """metatile id -> behaviour for the current layout."""
        layout_id = struct.unpack("<H", self.emu.read(self.sym["gMapHeader"] + 0x12, 2))[0]
        if layout_id in self._attr_cache:
            return self._attr_cache[layout_id]
        ml = self.emu.u32(self.sym["gMapHeader"])
        prim = self.emu.u32(ml + 0x10)
        sec = self.emu.u32(ml + 0x14)
        pa = self.emu.u32(prim + 0x10)
        sa = self.emu.u32(sec + 0x10)
        a1 = struct.unpack("<512H", self.emu.read(pa, 1024))
        a2 = struct.unpack("<512H", self.emu.read(sa, 1024))
        beh = [v & 0xFF for v in a1] + [v & 0xFF for v in a2]
        self._attr_cache[layout_id] = beh
        return beh

    def grid(self):
        """(w, h, cells) where cells[(y+7)*w + x+7] is the raw u16 of the backup map; w includes the 15 border."""
        w = self.emu.u32(self.sym["gBackupMapLayout"])
        h = self.emu.u32(self.sym["gBackupMapLayout"] + 4)
        p = self.emu.u32(self.sym["gBackupMapLayout"] + 8)
        raw = self.emu.read(p, w * h * 2)
        return w, h, struct.unpack(f"<{w * h}H", raw)

    def map_size(self):
        w = self.emu.u32(self.sym["gBackupMapLayout"])
        h = self.emu.u32(self.sym["gBackupMapLayout"] + 4)
        return w - 15, h - 14

    def objects(self, with_player=False):
        base = self.sym["gObjectEvents"]
        raw = self.emu.read(base, 0x24 * 16)
        out = []
        for i in range(16):
            o = raw[i * 0x24:(i + 1) * 0x24]
            bits0, bits1, bits2 = o[0], o[1], o[2]
            if not (bits0 & 1):
                continue
            player = bool(bits2 & 1)
            if player and not with_player:
                continue
            cx, cy = struct.unpack_from("<hh", o, 0x10)
            out.append(dict(i=i, player=player, local=o[8], gfx=struct.unpack_from("<H", o, 6)[0],
                            x=cx - 7, y=cy - 7, facing=o[0x18] & 0xF, invisible=bool(bits1 & 0x20),
                            move_type=o[5], trainer_type=o[0x1D] if False else 0))
        return out

    def snapshot_map(self):
        w, h, cells = self.grid()
        beh = self._attrs()
        mw, mh = w - 15, h - 14
        return dict(w=w, h=h, mw=mw, mh=mh, cells=cells, beh=beh)

    def cell(self, m, x, y):
        """(passable, behaviour) of map cell x,y (map coordinates, may be in the 7-tile border)."""
        cx, cy = x + 7, y + 7
        if cx < 0 or cy < 0 or cx >= m["w"] or cy >= m["h"]:
            return False, 0
        v = m["cells"][cy * m["w"] + cx]
        coll = (v >> 10) & 3
        mid = v & 0x3FF
        b = m["beh"][mid] if mid < 1024 else 0
        if b in JUMP:
            return v != 0x3FF, b      # ledges carry collision bit 1 but can be jumped in their direction
        return coll == 0 and v != 0x3FF and b not in BLOCKED_BEH, b

    @staticmethod
    def doorlike(b):
        n = MB_NAME.get(b, "")
        return "DOOR" in n or "ARROW_WARP" in n or "STAIR" in n or "LADDER" in n or "ESCALATOR" in n

    def plan(self, goal, avoid_grass=True, extra_block=(), through_objects=False, adjacent=False):
        """Dijkstra from the player to `goal` (x, y) (or any of a list). Returns a list of steps
        [(dir, x, y)]; a jump over a ledge is one step with `jump` in the dir name."""
        m = self.snapshot_map()
        start = tuple(self.pos())
        goals = set(goal if isinstance(goal, list) else [goal])
        key = self.loc()
        blocked_edges = self.blocked.get(key, set())
        occupied = set() if through_objects else {(o["x"], o["y"]) for o in self.objects()}
        occupied |= set(extra_block)
        occupied -= goals if not adjacent else set()
        pq = [(0, start)]
        dist = {start: 0}
        prev = {}
        while pq:
            d, cur = heapq.heappop(pq)
            if d > dist.get(cur, 1e9):
                continue
            if cur in goals:
                path = []
                while cur != start:
                    p, step = prev[cur]
                    path.append(step)
                    cur = p
                return list(reversed(path))
            for dn, (dx, dy) in DIRV.items():
                nx, ny = cur[0] + dx, cur[1] + dy
                if (cur[0], cur[1], dx, dy) in blocked_edges:
                    continue
                ok, b = self.cell(m, nx, ny)
                cost = 1
                land = (nx, ny)
                if not ok and (nx, ny) in goals and self.doorlike(b):
                    ok = True          # doors have the collision bit set but can be walked into
                if not ok:
                    continue
                if b in JUMP:
                    if JUMP[b] != (dx, dy):
                        continue
                    land = (nx + dx, ny + dy)
                    ok2, b2 = self.cell(m, *land)
                    if not ok2:
                        continue
                    cost = 2
                elif (nx, ny) in occupied and (nx, ny) not in goals:
                    continue
                if b in GRASS and avoid_grass:
                    cost += 4
                if b in SLIDE:
                    cost += 30        # forced-move tiles push the player: only when nothing else leads there
                if land in occupied and land not in goals:
                    continue
                nd = d + cost
                if nd < dist.get(land, 1e9):
                    dist[land] = nd
                    prev[land] = (cur, (dn, land[0], land[1]))
                    heapq.heappush(pq, (nd, land))
        return None

    def ascii_map(self, margin=0):
        """Debug: the current map as text. # blocked, . floor, g grass, W warp/door, C coord event, O object, @ player."""
        m = self.snapshot_map()
        mw, mh = self.map_size()
        mj = mapjson(self.map_name())
        warps = {(w["x"], w["y"]) for w in mj.get("warp_events", [])}
        coords = {(c["x"], c["y"]) for c in mj.get("coord_events", [])}
        objs = {(o["x"], o["y"]) for o in self.objects()}
        me = tuple(self.pos())
        lines = []
        hdr = "    " + "".join(str((x // 10) % 10) if x % 10 == 0 else " " for x in range(-margin, mw + margin))
        lines.append(hdr)
        lines.append("    " + "".join(str(x % 10) for x in range(-margin, mw + margin)))
        for y in range(-margin, mh + margin):
            row = []
            for x in range(-margin, mw + margin):
                ok, b = self.cell(m, x, y)
                ch = "." if ok else "#"
                if b in GRASS:
                    ch = "g"
                if self.doorlike(b):
                    ch = "D" if not ok else "d"
                if (x, y) in coords:
                    ch = "C"
                if (x, y) in warps:
                    ch = "W"
                if (x, y) in objs:
                    ch = "O"
                if (x, y) == me:
                    ch = "@"
                row.append(ch)
            lines.append(f"{y:3d} " + "".join(row))
        return "\n".join(lines)

    # --- walking --------------------------------------------------------------------------
    def step(self, d, tries=3):
        """Press direction `d` until the position changes (a step) or give up. Returns True if moved."""
        p0 = tuple(self.pos())
        loc0 = self.loc()
        self.cat = "walk"
        for t in range(tries):
            self.emu.hold(d, 16)
            self.stats["steps"] += 1
            if self.seg in self.seg_stats:
                self.seg_stats[self.seg]["steps"] += 1
            if self.loc() != loc0 or tuple(self.pos()) != p0:
                self.cat = "other"
                if self.loc() == loc0 and self.risky_cell(tuple(self.pos())):
                    self.emu.run(3)        # a coord event or a trainer's sight line fires a frame or two after the step
                return True
            if self.emu.in_battle() or not self.emu.in_overworld():
                break
            if t == 0:
                # first press may only turn the player; if a script started, stop
                self.emu.run(2)
                if not self.ready() and self.emu.u8(self.rs + 6) == 0 and self.script_running():
                    break
        self.cat = "other"
        return tuple(self.pos()) != p0 or self.loc() != loc0

    _special = {}

    def risky_cell(self, pos):
        name = self.map_name()
        if name not in self._special:
            try:
                mj = mapjson(name)
            except Exception:
                self._special[name] = (set(), True)
            else:
                cells = {(c["x"], c["y"]) for c in mj.get("coord_events", [])}
                cells |= {(w["x"], w["y"]) for w in mj.get("warp_events", [])}
                trainers = any(o.get("trainer_type", "TRAINER_TYPE_NONE") != "TRAINER_TYPE_NONE" for o in mj.get("object_events", []))
                self._special[name] = (cells, trainers)
        cells, trainers = self._special[name]
        return trainers or pos in cells

    def script_running(self):
        # fieldReady = 0 can also be mid-step; settle a few frames to tell
        for _ in range(4):
            if self.ready():
                return False
            self.emu.run(4)
        return True

    def face(self, d):
        """Turn to face `d` without moving (tap)."""
        self.emu.hold(d, 3)
        self.emu.run(8)

    def goto(self, x, y, avoid_grass=True, adjacent=False, max_replans=14):
        """Walk to map cell (x, y) with real direction presses; replans when blocked or interrupted.
        Returns False when the map changed on the way (a warp or connection)."""
        goal = (x, y)
        key0 = self.loc()
        self.blocked.pop(key0, None)
        for attempt in range(max_replans):
            if self.loc() != key0:
                return False
            path = self.plan(goal, avoid_grass=avoid_grass, adjacent=adjacent)
            if path is None:
                raise BlockedPath(f"no path to {goal} on {self.map_name()} from {self.pos()}")
            if not path:
                return True
            r = self._follow(path, key0)
            if r is None:
                return False
            if r and tuple(self.pos()) == goal:
                self.emu.run(4)
                return True
        raise BlockedPath(f"gave up walking to {goal} on {self.map_name()} at {self.pos()}")

    def cross(self, d, avoid_grass=True):
        """Walk off the map edge in direction d (a connection) and settle on the neighbour."""
        mw, mh = self.map_size()
        m = self.snapshot_map()
        key0 = self.loc()
        if d == "UP":
            goals = [(x, -1) for x in range(mw) if self.cell(m, x, -1)[0]]
        elif d == "DOWN":
            goals = [(x, mh) for x in range(mw) if self.cell(m, x, mh)[0]]
        elif d == "LEFT":
            goals = [(-1, y) for y in range(mh) if self.cell(m, -1, y)[0]]
        else:
            goals = [(mw, y) for y in range(mh) if self.cell(m, mw, y)[0]]
        if not goals:
            raise BlockedPath(f"no edge cells {d} on {self.map_name()}")
        self.goto_any(goals, avoid_grass=avoid_grass)
        if self.loc() == key0:
            self.step(d)
        self.settle_after_warp()
        return self.loc() != key0

    def goto_any(self, goals, avoid_grass=True, max_replans=12):
        """Like goto, but to the cheapest of several goals."""
        key0 = self.loc()
        self.blocked.pop(key0, None)
        for attempt in range(max_replans):
            path = self.plan(goals, avoid_grass=avoid_grass)
            if path is None:
                raise BlockedPath(f"no path to any of {len(goals)} goals on {self.map_name()} from {self.pos()}")
            if not path:
                return True
            ok = self._follow(path, key0)
            if ok is None:
                return False
            if ok and tuple(self.pos()) in goals:
                return True
        raise BlockedPath(f"gave up walking on {self.map_name()} at {self.pos()}")

    def _follow(self, path, key0):
        """Walk a planned path. Returns True when finished, False when interrupted (replan), None when the map changed."""
        for (d, nx, ny) in path:
            if self.loc() != key0:
                return None
            if not self.ready():
                self.settle()
                if self.loc() != key0:
                    return None
                return False
            before = tuple(self.pos())
            self.step(d)
            if self.loc() != key0:
                return None
            if not self.ready():
                self.settle()
                if self.loc() != key0:
                    return None
                return False
            now = tuple(self.pos())
            if now != (nx, ny):
                if now == before:
                    if any((o["x"], o["y"]) == (nx, ny) for o in self.objects()) or not self.ready():
                        self.emu.run(20)          # somebody walked into the way: wait and plan again
                    else:
                        dx, dy = DIRV[d]
                        self.blocked.setdefault(key0, set()).add((before[0], before[1], dx, dy))
                        self.say(f"  blocked at {before} going {d} on {self.map_name()}")
                return False
        return True

    def walk_dirs(self, dirs):
        """Raw walk: list of direction names, no planning (for scripted beats)."""
        for d in dirs:
            if not self.ready():
                self.settle()
            self.step(d)
        self.emu.run(6)

    def warp_dir(self, x, y):
        """Direction that fires the warp on cell (x, y): the arrow of an arrow warp, else UP (doors)."""
        m = self.snapshot_map()
        name = MB_NAME.get(self.cell(m, x, y)[1], "")
        for k, d in (("EAST_ARROW", "RIGHT"), ("WEST_ARROW", "LEFT"), ("NORTH_ARROW", "UP"), ("SOUTH_ARROW", "DOWN")):
            if k in name:
                return d
        mw, mh = self.map_size()
        if y >= mh - 1:
            return "DOWN"
        if x >= mw - 1:
            return "RIGHT"
        if x <= 0:
            return "LEFT"
        return "UP"

    def exit_through(self, x, y, d=None, wait=240):
        """Walk to (x, y) then keep pressing d until the map changes (a warp or a connection)."""
        key0 = self.loc()
        if d is None:
            d = self.warp_dir(x, y)
        if tuple(self.pos()) != (x, y):
            self.goto(x, y)
        if self.loc() != key0:
            self.settle_after_warp()
            return True
        for _ in range(8):
            self.emu.hold(d, 16)
            if self.loc() != key0:
                break
            if not self.ready():
                self.settle()
                if self.loc() != key0:
                    break
        self.settle_after_warp()
        return self.loc() != key0

    FIRING = ("DOOR", "ARROW_WARP", "STAIRS", "LADDER", "ESCALATOR", "WARP_PAD", "CAVE_ENTRANCE", "WARP")

    def warp_options(self, dest=None):
        """warp_events of the current map (from map.json) whose cell actually fires, optionally to `dest`
        (a map dir name). Returns [(x, y, dest_dir, dest_warp_id, behaviour_name)]."""
        mj = mapjson(self.map_name())
        m = self.snapshot_map()
        out = []
        for w in mj.get("warp_events", []):
            dd = None
            dm = w["dest_map"]
            if dm.startswith("MAP_"):
                dd = MAP_CONST.get(dm)
            if dest and dd != dest:
                continue
            ok, b = self.cell(m, w["x"], w["y"])
            nm = MB_NAME.get(b, "")
            if any(k in nm for k in self.FIRING) and "NOT" not in nm or (dd is None):
                out.append((w["x"], w["y"], dd, int(w["dest_warp_id"]), nm))
        return out

    def take_exit(self, dest, which=0):
        """Leave the current map through a warp that goes to `dest` (a map dir name)."""
        opts = self.warp_options(dest)
        if not opts:
            raise BlockedPath(f"no firing warp to {dest} on {self.map_name()}")
        # nearest by planned path length
        best = None
        for (x, y, dd, wid, nm) in opts:
            pl = self.plan((x, y))
            if pl is None:
                continue
            if best is None or len(pl) < best[0]:
                best = (len(pl), x, y)
        if best is None:
            raise BlockedPath(f"no path to any warp to {dest} on {self.map_name()}")
        return self.exit_through(best[1], best[2])

    def settle_after_warp(self):
        for _ in range(60):
            if self.ready():
                self.emu.run(10)
                if self.ready():
                    return
            self.emu.run(6)
            if not self.ready() and not self.emu.in_battle():
                pass
        self.settle()

    def talk(self, x, y, mash_limit=200, answer=None):
        """Walk next to the object at (x, y), face it and press A, then mash text until free.
        Returns the texts seen."""
        n0 = len(self.seen)
        m = self.snapshot_map()
        # objects far off-screen are not spawned yet, so also treat the map's authored object cells as taken
        occ = {(o["x"], o["y"]) for o in self.objects()} | {(o["x"], o["y"]) for o in mapjson(self.map_name()).get("object_events", [])}
        occ.discard((x, y)) if False else None
        cands = [(x + dx, y + dy) for dx, dy in DIRV.values() if self.cell(m, x + dx, y + dy)[0] and (x + dx, y + dy) not in occ]
        far = []          # across a counter: stand two cells away with an impassable cell between
        for dx, dy in DIRV.values():
            mid = (x + dx, y + dy)
            if not self.cell(m, *mid)[0] and "COUNTER" in MB_NAME.get(self.cell(m, *mid)[1], ""):
                if self.cell(m, x + 2 * dx, y + 2 * dy)[0] and (x + 2 * dx, y + 2 * dy) not in occ:
                    far.append((x + 2 * dx, y + 2 * dy))
        path = self.plan(cands + far, adjacent=True, extra_block=())
        if path is None:
            raise BlockedPath(f"cannot reach a cell next to {(x, y)} on {self.map_name()}")
        if path:
            self.goto(path[-1][1], path[-1][2])
        px, py = self.pos()
        dx, dy = x - px, y - py
        d = VDIR[(dx // max(1, abs(dx)) if dx else 0, dy // max(1, abs(dy)) if dy else 0)]
        me = next((o for o in self.objects(with_player=True) if o["player"]), None)
        if me is None or me["facing"] != {"DOWN": 1, "UP": 2, "LEFT": 3, "RIGHT": 4}[d]:
            self.face(d)           # a tap turns on the spot; pressing the way we already face would walk
        else:
            self.emu.run(10)
        self.emu.cmd(f"write {self.sym['gStringVar4']:#x} ff")       # so an old string is not mistaken for new text
        self.press("A", hold=4, after=16)
        self.note_text()
        self.mash(n=mash_limit)
        return self.seen[n0:]

    # --- battles --------------------------------------------------------------------------
    BM = 84

    def bmon(self, i):
        raw = self.emu.read(self.sym["gBattleMons"] + self.BM * i, self.BM)
        species, atk, dfn, spe, spa, spd = struct.unpack_from("<6H", raw, 0)
        moves = list(struct.unpack_from("<4H", raw, 12))
        types = (raw[33], raw[34])
        hp, = struct.unpack_from("<H", raw, 36)
        level = raw[38]
        maxhp, = struct.unpack_from("<H", raw, 40)
        status1, = struct.unpack_from("<I", raw, 72)
        return dict(species=species, moves=moves, types=types, hp=hp, maxhp=maxhp, level=level, status=status1)

    def battle_tp(self):
        bs = self.emu.u32(self.sym["gBattleStruct"])
        slot = self.emu.u8(self.sym["gBattlerPartyIndexes"])
        return self.emu.u8(bs + 0x1c3 + slot)

    _tc = None

    def type_mult(self, atk_type, def_types):
        if self._tc is None:
            self._tc = self.emu.read(self.sym["gTypeEffectiveness"], 336)
        m = 10.0
        seen = set()
        for dt in def_types:
            if dt in seen:
                continue
            seen.add(dt)
            for i in range(0, 336, 3):
                a, d, v = self._tc[i], self._tc[i + 1], self._tc[i + 2]
                if a == 0xFF:
                    break
                if a == 0xFE:
                    continue
                if a == atk_type and d == dt:
                    m = m * v / 10.0
        return m / 10.0

    def move_info(self, mv):
        raw = self.emu.read(self.sym["gMovesInfo"] + 32 * mv, 32)
        return dict(effect=raw[13], power=raw[14], type=raw[15], acc=raw[16], tp=raw[17])

    def best_move(self, avoid=()):
        """Index of the move with the best expected damage that the mon has TP for; None if none."""
        me, foe = self.bmon(0), self.bmon(1)
        tp = self.battle_tp()
        if self.lose_mode:
            cand = [(self.move_info(mv)["power"], i) for i, mv in enumerate(me["moves"])
                    if mv and i not in avoid and self.move_info(mv)["tp"] <= tp]
            return min(cand)[1] if cand else 0
        best, best_s = None, -1.0
        for i, mv in enumerate(me["moves"]):
            if mv == 0 or i in avoid:
                continue
            mi = self.move_info(mv)
            if mi["tp"] > tp:
                continue
            s = mi["power"] * self.type_mult(mi["type"], foe["types"]) * (1.5 if mi["type"] in me["types"] else 1.0)
            s *= (mi["acc"] or 100) / 100.0
            if s > best_s:
                best, best_s = i, s
        if best is None or best_s <= 0:
            # status move or nothing: any move with TP, else slot 0
            for i, mv in enumerate(me["moves"]):
                if mv and i not in avoid and self.move_info(mv)["tp"] <= tp:
                    return i
            return 0
        return best

    def is_wild(self):
        return not (self.emu.u32(self.sym["gBattleTypeFlags"]) & 8)   # BATTLE_TYPE_TRAINER = 8

    def run_battle(self, policy="auto", max_turns=80):
        """Fight the current battle with real input. policy: 'attack' always attacks, 'run' tries to
        flee a wild battle, 'auto' attacks unless a wild battle goes badly (lead below 35% HP)."""
        e = self.emu
        self.cat = "battle"
        self.stats["battles"] += 1
        if self.seg in self.seg_stats:
            self.seg_stats[self.seg]["battles"] += 1
        turns = 0
        idle = 0
        potions = 0
        wild = self.is_wild()
        if wild and policy == "auto":
            policy = self.wild_policy
        if self.lose_mode:
            policy = "attack"
        self.say(f"  battle start ({'wild' if wild else 'trainer'})")
        for _ in range(8000):
            if not e.in_battle():
                break
            if e.player_choosing_action():
                if turns >= max_turns:
                    raise Fail("battle never ended")
                turns += 1
                self.stats["battle_turns"] += 1
                if self.seg in self.seg_stats:
                    self.seg_stats[self.seg]["battle_turns"] += 1
                me = self.bmon(0)
                if turns == 1:
                    fo = self.bmon(1)
                    self.say(f"    foe species {fo['species']} L{fo['level']} hp {fo['hp']}/{fo['maxhp']}; lead hp {me['hp']}/{me['maxhp']}")
                flee = False
                if wild and (policy == "run" or (policy == "auto" and me["hp"] * 100 < me["maxhp"] * 35)):
                    flee = True
                if (not wild or policy == "attack") and not flee and not self.lose_mode and potions < 4 and me["hp"] * 100 < me["maxhp"] * 45 \
                        and self.bag_count("ITEM_POTION") and me["maxhp"] - me["hp"] >= 15:
                    potions += 1
                    self.say(f"  battle: Potion at {me['hp']}/{me['maxhp']}")
                    self.battle_use_item()
                else:
                    self.battle_turn(flee)
                idle = 0
            elif self.choosing_move():
                # we are somehow in the move menu (cursor left there): pick the best move
                self.choose_move()
            else:
                t = self.last_text().lower()
                key = "B" if any(w in t for w in ("will you change", "switch", "use next")) else "A"
                e.press(key, hold=4, after=10)
                self.note_text()
                idle += 1
                if idle > 600:
                    self.shot("FAIL_battle_stuck")
                    raise Fail("battle stuck: " + t[:60])
        self.cat = "other"
        e.run(20)
        lead = self.mon(0) if self.party_count() else None
        if lead:
            self.say(f"  battle over after {turns} turns; lead L{lead['level']} hp {lead['hp']}/{lead['maxhp']} tp spent {lead['tp']}")
        return turns

    def battle_turn(self, flee=False):
        e = self.emu
        if flee:
            self.menu_cursor_to(3)
            e.press("A", hold=4, after=14)
            e.run(20)
            return
        self.menu_cursor_to(0)
        e.press("A", hold=4, after=14)
        for _ in range(30):
            if self.choosing_move():
                break
            e.run(4)
        self.choose_move()

    def battle_use_item(self):
        """ITEM > first item (a Potion) > USE > lead. Real battle bag menus."""
        e = self.emu
        self.menu_cursor_to(2)
        e.press("A", hold=4, after=60)
        e.press("A", hold=4, after=30)       # select the item
        e.press("A", hold=4, after=40)       # USE
        e.press("A", hold=4, after=30)       # party slot 0
        e.run(40)

    def choose_move(self):
        e = self.emu
        avoid = []
        for _ in range(4):
            idx = self.best_move(avoid)
            self.move_cursor_to(idx)
            e.press("A", hold=4, after=14)
            e.run(16)
            if not self.choosing_move():
                return idx
            avoid.append(idx)       # refused (no TP): try another
        e.press("B", hold=4, after=10)
        return None

    def pick_fight(self, move=0, run_away=False):
        self.battle_turn(run_away)

    def choosing_move(self):
        e = self.emu
        move = self.sym["HandleInputChooseMove"]
        return (e.u32(self.sym["gBattlerControllerFuncs"]) & ~1) == move

    def menu_cursor_to(self, idx):
        cur = self.emu.u8(self.sym["gActionSelectionCursor"])
        # 2x2 grid: 0 FIGHT, 1 BAG, 2 POKEMON, 3 RUN  (vanilla layout: 0 1 / 2 3)
        for _ in range(4):
            cur = self.emu.u8(self.sym["gActionSelectionCursor"])
            if cur == idx:
                return
            cx, cy = cur & 1, cur >> 1
            tx, ty = idx & 1, idx >> 1
            if cx != tx:
                self.emu.press("LEFT" if tx < cx else "RIGHT", hold=3, after=8)
            elif cy != ty:
                self.emu.press("UP" if ty < cy else "DOWN", hold=3, after=8)

    def move_cursor_to(self, idx):
        for _ in range(6):
            cur = self.emu.u8(self.sym["gMoveSelectionCursor"])
            if cur == idx:
                return
            cx, cy = cur & 1, cur >> 1
            tx, ty = idx & 1, idx >> 1
            if cx != tx:
                self.emu.press("LEFT" if tx < cx else "RIGHT", hold=3, after=8)
            elif cy != ty:
                self.emu.press("UP" if ty < cy else "DOWN", hold=3, after=8)

    def save_state(self, path):
        self.emu.cmd(f"savestate {path}")

    def load_state(self, path):
        self.emu.cmd(f"loadstate {path}")
        self.emu.run(2)

    # --- grinding by state injection (always logged) ---------------------------------------
    NATURE_TABLE = None

    def _nature_table(self):
        if self.NATURE_TABLE is None:
            txt = open(ROOT + "/src/pokemon.c").read()
            blk = txt[txt.index("const s8 gNatureStatTable"):]
            blk = blk[:blk.index("};")]
            rows = re.findall(r"\[NATURE_\w+\]\s*=\s*\{([^}]*)\}", blk)
            Bot.NATURE_TABLE = [[int(v) for v in re.findall(r"[+-]?\d+", r)] for r in rows]
        return self.NATURE_TABLE

    def species_info(self, sp):
        raw = self.emu.read(self.sym["gSpeciesInfo"] + 28 * sp, 28)
        return dict(hp=raw[0], atk=raw[1], dfn=raw[2], spe=raw[3], spa=raw[4], spd=raw[5], growth=raw[19])

    def exp_for(self, growth, level):
        return self.emu.u32(self.sym["gExperienceTables"] + (growth * 101 + level) * 4)

    def learnset_moves(self, sp, level):
        ptr = self.emu.u32(self.sym["gLevelUpLearnsets"] + 4 * sp)
        raw = self.emu.read(ptr, 2 * 40)
        moves = []
        for i in range(40):
            v = struct.unpack_from("<H", raw, 2 * i)[0]
            if v == 0xFFFF:
                break
            lv, mv = v >> 9, v & 0x1FF
            if lv > level:
                break
            if mv not in moves:
                moves.append(mv)
        return moves[-4:]

    def set_party_level(self, slot, level, why="", moves=True):
        """INJECTION: raise party mon `slot` to `level` (exp, stats, full HP, the learnset's last four moves).
        This is the plan's allowed 'grinding' shortcut; it is logged with the before/after."""
        import itertools
        base = self.sym["gPlayerParty"] + 100 * slot
        raw = bytearray(self.emu.read(base, 100))
        pers, otid = struct.unpack_from("<II", raw, 0)
        key = pers ^ otid
        words = list(struct.unpack_from("<12I", raw, 32))
        words = [w ^ key for w in words]
        order = list(itertools.permutations("GAEM"))[pers % 24]
        off = {c: 3 * order.index(c) for c in "GAEM"}
        species = words[off["G"]] & 0xFFFF
        si = self.species_info(species)
        old_level = raw[84]
        exp = self.exp_for(si["growth"], level)
        words[off["G"] + 1] = exp
        if moves:
            ms = self.learnset_moves(species, level)
            ms = ms + [0] * (4 - len(ms))
            words[off["A"]] = ms[0] | (ms[1] << 16)
            words[off["A"] + 1] = ms[2] | (ms[3] << 16)
        # IVs and EVs
        ev = struct.pack("<3I", *words[off["E"]:off["E"] + 3])
        hp_ev, atk_ev, def_ev, spe_ev, spa_ev, spd_ev = ev[:6]
        iv_word = words[off["M"] + 1]
        ivs = [(iv_word >> (5 * i)) & 31 for i in range(6)]       # hp, atk, def, spe, spa, spd
        nat = self._nature_table()[pers % 25]
        def stat(basev, iv, evv, idx):
            n = ((2 * basev + iv + evv // 4) * level) // 100 + 5
            m = nat[idx]
            return n * 110 // 100 if m == 1 else (n * 90 // 100 if m == -1 else n)
        maxhp = ((2 * si["hp"] + ivs[0] + hp_ev // 4) * level) // 100 + level + 10
        stats = [stat(si["atk"], ivs[1], atk_ev, 0), stat(si["dfn"], ivs[2], def_ev, 1), stat(si["spe"], ivs[3], spe_ev, 2),
                 stat(si["spa"], ivs[4], spa_ev, 3), stat(si["spd"], ivs[5], spd_ev, 4)]
        # write back: re-encrypt, checksum, party bytes
        chk = sum(w & 0xFFFF for w in words) + sum(w >> 16 for w in words)
        struct.pack_into("<H", raw, 28, chk & 0xFFFF)
        for i, w in enumerate(words):
            struct.pack_into("<I", raw, 32 + 4 * i, w ^ key)
        raw[84] = level
        struct.pack_into("<HH", raw, 86, maxhp, maxhp)           # hp, maxHP
        struct.pack_into("<5H", raw, 90, *stats)
        struct.pack_into("<I", raw, 80, 0)                       # status
        self.emu.cmd(f"write {base:#x} {bytes(raw).hex()}")
        self.inject("level", f"party slot {slot} species {species} L{old_level} -> L{level} {why}")
        m = self.mon(slot)
        self.say(f"    now L{m['level']} hp {m['hp']}/{m['maxhp']} moves {m['moves']}")
        return m

    # --- menus ----------------------------------------------------------------------------
    MENU_ACTIONS = dict(POKEDEX=0, POKEMON=1, BAG=2, POKENAV=3, PLAYER=4, SAVE=5, OPTION=6, EXIT=7)

    def start_menu(self, action):
        """Open the start menu and put the cursor on `action` (leaves the menu open)."""
        e = self.emu
        if not self.ready():
            self.settle()
        e.press("START", hold=4, after=24)
        n = e.u8(self.sym["sNumStartMenuActions"])
        acts = list(e.read(self.sym["sCurrentStartMenuActions"], n))
        want = self.MENU_ACTIONS[action]
        if want not in acts:
            raise Fail(f"start menu has no {action}: {acts}")
        tgt = acts.index(want)
        for _ in range(12):
            cur = e.u8(self.sym["sStartMenuCursorPos"])
            if cur == tgt:
                break
            e.press("DOWN" if tgt > cur else "UP", hold=4, after=8)
        return True

    def close_menus(self, limit=40):
        for _ in range(limit):
            if self.ready():
                return
            self.emu.press("B", hold=4, after=14)
        self.settle()

    def bag_items(self, pocket=0):
        off, n = ((0x560, 30), (0x5d8, 30), (0x650, 16), (0x690, 58))[pocket]
        key = self.emu.u32(self.sb2() + 0x6C) & 0xFFFF
        raw = self.emu.read(self.sb1() + off, 4 * n)
        out = []
        for i in range(n):
            a, q = struct.unpack_from("<HH", raw, 4 * i)
            if a:
                out.append((a, q ^ key))
        return out

    def use_item_on_mon(self, item, slot=0):
        """Field bag, Items pocket: USE `item` on party slot `slot`. Returns True if the item count dropped."""
        e = self.emu
        iid = C(item)
        items = self.bag_items(0)
        idx = next((i for i, (a, q) in enumerate(items) if a == iid), None)
        if idx is None:
            return False
        before = self.bag_count(iid)
        self.start_menu("BAG")
        e.press("A", hold=4, after=60)
        # Items pocket is the first; put the cursor on the item
        base = self.sym["gBagPosition"]
        for _ in range(40):
            if e.u8(base + 5) == 0:
                break
            e.press("LEFT", hold=4, after=10)
        for _ in range(60):
            cur = struct.unpack("<H", e.read(base + 8, 2))[0]
            if cur == idx:
                break
            e.press("DOWN" if idx > cur else "UP", hold=4, after=8)
        e.press("A", hold=4, after=20)           # the item menu
        e.press("A", hold=4, after=40)           # USE -> party
        # party menu: cursor to `slot`
        pm = self.sym["gPartyMenu"]
        for _ in range(10):
            cur = e.u8(pm + 9)
            if cur == slot:
                break
            e.press("DOWN" if slot > cur else "UP", hold=4, after=8)
        e.press("A", hold=4, after=30)
        for _ in range(40):
            e.press("A", hold=4, after=16)
            if self.bag_count(iid) < before and self.ready():
                break
            if self.bag_count(iid) < before and not self.emu.in_overworld():
                continue
        self.close_menus()
        return self.bag_count(iid) < before

    def save_game(self):
        """START > SAVE > YES, as a player does. Returns True when "saved the game" was shown."""
        e = self.emu
        self.start_menu("SAVE")
        saved = False
        for _ in range(60):
            e.press("A", hold=4, after=30)
            t = self.last_text().lower()
            if "saved the game" in t:
                saved = True
                e.run(60)
                break
        self.close_menus()
        return saved

    # --- reports --------------------------------------------------------------------------
    def close(self):
        if self.rec:
            self.rec.close()
        try:
            self.emu.p.stdin.write("quit\n")
        except Exception:
            pass
