#!/usr/bin/env python3
"""Oracle: the host build (libminasim.so) against the real ARM code.

The ROM's own ELF is executed in Unicorn (an ARM CPU emulator; `pip install unicorn`), one function
at a time, and its results are compared bit for bit with the same function compiled for the host:

  rng      Random() after SeedRng(seed), 50 draws          (src/random.c)
  createmon  the 100-byte struct CreateMon() builds         (src/pokemon.c: stats, IVs, personality, moves)
  damage   CalculateBaseDamage() for random attacker/defender pairs, moves, held items, stat stages,
           burn and screens                                  (src/pokemon.c)

Not compared: whole battles. The ROM stirs the RNG every VBlank (unless a TEST build's
gTestFreezeRngStir is set) and its animations draw RNG the host skips, so a seeded host battle and a
ROM battle diverge by construction. The pieces that decide a battle (RNG, mon creation, damage) are
compared exactly instead; the battle scripts themselves are the same bytes, assembled for the host.

Usage (from the repo root, after `make balance-lib` and a ROM build such as `make release TEST=1`):

  python3 tools/balance_sim/host/oracle.py                 # rng + createmon + damage, a few hundred cases each
  python3 tools/balance_sim/host/oracle.py --n 1000 --elf pokeemerald_modern_release_test.elf
Exit status 0 only when every compared case is identical. It does not run in `make check` (it needs a
ROM build and unicorn); run it after touching src/random.c, src/pokemon.c, hb_platform.c or the host
build rules, and whenever the stub allow-list changes.
"""
import argparse
import ctypes as C
import glob
import os
import random
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "balance_sim"))

ELF_CANDIDATES = ["pokeemerald_modern_release_test.elf", "pokeemerald_modern_test.elf", "pokeemerald_modern.elf",
                  "pokeemerald_modern_release.elf"]
HOST_SOURCES = ["random", "pokemon", "battle_util", "battle_main", "battle_script_commands", "special_move"]


def find_elf(arg):
    if arg:
        return arg if os.path.isabs(arg) else os.path.join(ROOT, arg)
    for c in ELF_CANDIDATES:
        p = os.path.join(ROOT, c)
        if os.path.exists(p):
            return p
    sys.exit("oracle: no ROM ELF found; build one (`make release TEST=1`) or pass --elf")


def check_fresh(elf):
    newest = max(os.path.getmtime(os.path.join(ROOT, "src", s + ".c")) for s in HOST_SOURCES)
    if os.path.getmtime(elf) < newest:
        print("oracle: WARNING %s is older than the battle sources; rebuild the ROM for a meaningful comparison" % os.path.basename(elf))


# ------------------------------------------------------------------ the ARM side
class Arm:
    def __init__(self, elf):
        try:
            from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_HOOK_INTR
            from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC
        except ImportError:
            sys.exit("oracle: needs unicorn (pip install unicorn)")
        self.U = dict(R=[UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3], SP=UC_ARM_REG_SP, LR=UC_ARM_REG_LR, PC=UC_ARM_REG_PC)
        self.syms = {}
        self.objs = set()  # data symbols: their address is not a Thumb-tagged one
        out = subprocess.check_output(["arm-none-eabi-readelf", "-sW", elf], text=True)
        for l in out.splitlines():
            p = l.split()
            if len(p) >= 8 and p[3] in ("FUNC", "OBJECT") and p[6] != "UND":
                self.syms[p[7]] = int(p[1], 16)
                if p[3] == "OBJECT":
                    self.objs.add(p[7])
        self.mu = mu = Uc(UC_ARCH_ARM, UC_MODE_THUMB)
        for base, size in [(0x02000000, 0x40000), (0x03000000, 0x8000), (0x04000000, 0x10000), (0x05000000, 0x1000),
                           (0x06000000, 0x20000), (0x07000000, 0x1000), (0x08000000, 0x2000000)]:
            mu.mem_map(base, size)
        data = open(elf, "rb").read()
        phoff, = struct.unpack_from("<I", data, 28)
        phentsize, phnum = struct.unpack_from("<HH", data, 42)
        for i in range(phnum):
            t, off, va, pa, fsz, msz, fl, al = struct.unpack_from("<8I", data, phoff + i * phentsize)
            if t == 1 and fsz:
                mu.mem_write(pa, data[off:off + fsz])
                if va != pa:
                    mu.mem_write(va, data[off:off + fsz])  # .iwram_code runs from IWRAM
        self.sp0 = 0x03007E00
        mu.hook_add(UC_HOOK_INTR, self._swi)
        # the two save block pointers the game keeps in EWRAM
        for ptr, blk in (("gSaveBlock2Ptr", "gSaveblock2"), ("gSaveBlock1Ptr", "gSaveblock1")):
            if ptr in self.syms and blk in self.syms:
                self.w32(self.syms[ptr], self.syms[blk])

    def _swi(self, uc, intno, ud):
        from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_PC
        pc = uc.reg_read(UC_ARM_REG_PC)
        # thumb SWI: the immediate is the low byte of the instruction before pc
        try:
            num = uc.mem_read(pc - 2, 1)[0]
        except Exception:
            num = 0
        r0, r1, r2 = (uc.reg_read(r) for r in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2))
        s32 = lambda v: v - (1 << 32) if v & 0x80000000 else v
        if num in (6, 7):  # Div / DivArm
            n, d = (s32(r0), s32(r1)) if num == 6 else (s32(r1), s32(r0))
            q = int(abs(n) / abs(d)) * (1 if (n < 0) == (d < 0) else -1) if d else 0
            q = abs(n) // abs(d) * (1 if (n < 0) == (d < 0) else -1) if d else 0
            uc.reg_write(UC_ARM_REG_R0, q & 0xFFFFFFFF)
            uc.reg_write(UC_ARM_REG_R1, (n - q * d) & 0xFFFFFFFF)
            uc.reg_write(UC_ARM_REG_R3, abs(q) & 0xFFFFFFFF)
        elif num in (0x0B, 0x0C):  # CpuSet / CpuFastSet
            n, fill = r2 & 0x1FFFFF, bool(r2 & (1 << 24))
            unit = 4 if (num == 0x0C or r2 & (1 << 26)) else 2
            if fill:
                v = bytes(uc.mem_read(r0, unit))
                uc.mem_write(r1, v * n)
            else:
                uc.mem_write(r1, bytes(uc.mem_read(r0, n * unit)))
        else:
            raise RuntimeError("oracle: unhandled SWI 0x%02x at 0x%08x" % (num, pc))

    def w32(self, addr, v): self.mu.mem_write(addr, struct.pack("<I", v & 0xFFFFFFFF))
    def w16(self, addr, v): self.mu.mem_write(addr, struct.pack("<H", v & 0xFFFF))
    def read(self, addr, n): return bytes(self.mu.mem_read(addr, n))
    def r32(self, addr): return struct.unpack("<I", self.read(addr, 4))[0]
    def sym(self, name): return self.syms[name] if name in self.objs else self.syms[name] & ~1

    def call(self, name, *args, maxins=20_000_000):
        mu, U = self.mu, self.U
        for r, a in zip(U["R"], args[:4]):
            mu.reg_write(r, a & 0xFFFFFFFF)
        sp = self.sp0
        extra = args[4:]
        if extra:
            sp = (sp - 4 * len(extra)) & ~7
            for i, a in enumerate(extra):
                mu.mem_write(sp + 4 * i, struct.pack("<I", a & 0xFFFFFFFF))
        mu.reg_write(U["SP"], sp)
        ret = 0x08000001
        mu.reg_write(U["LR"], ret)
        fn = self.syms[name]
        try:
            mu.emu_start(fn | 1 if not (fn & 1) and self._is_thumb(name) else fn, ret & ~1, count=maxins)
        except Exception as e:
            pc = mu.reg_read(U["PC"])
            near = max((a, n) for n, a in self.syms.items() if a <= pc)[1]
            raise RuntimeError("ARM call %s faulted (%s) at pc=0x%08x in %s" % (name, e, pc, near))
        return mu.reg_read(U["R"][0])

    def _is_thumb(self, name):
        return True  # every battle/pokemon function in the modern build is thumb; ARM ones keep bit 0 clear but are IWRAM_CODE


# ------------------------------------------------------------------ the host side
def load_host():
    from minasim.engine import Engine
    eng = Engine()
    return eng


class Report:
    def __init__(self):
        self.rows = []
        self.fail = False

    def add(self, name, ok, total, first=None):
        self.rows.append((name, ok, total, first))
        if ok != total:
            self.fail = True
        print("%-10s %d/%d identical%s" % (name, ok, total, "" if ok == total else "   first difference: %s" % (first,)))


def test_rng(arm, host, n, rep):
    L = host.lib
    L.hb_rand_seq.restype = C.c_uint
    L.hb_rand_seq.argtypes = [C.c_uint, C.c_int]
    ok, first = 0, None
    for seed in random.Random(1).sample(range(1, 2**32), n):
        arm.w32(arm.sym("gRngValue"), seed)
        v = 0
        for _ in range(50):
            v = arm.call("Random")
        h = L.hb_rand_seq(seed, 50)
        if v == h:
            ok += 1
        elif first is None:
            first = {"seed": seed, "arm": v, "host": h}
    rep.add("rng", ok, n, first)


def test_createmon(arm, host, n, rep):
    L = host.lib
    L.hb_createmon_dump.argtypes = [C.c_uint, C.c_int, C.c_int, C.c_int, C.c_char_p]
    rnd = random.Random(2)
    gparty = arm.sym("gPlayerParty")
    species = [host.c.species[k] for k in host.c.species if 1 <= host.c.species[k] <= 151]
    ok, first = 0, None
    buf = C.create_string_buffer(host.pokemon_size)
    for _ in range(n):
        seed, sp, lvl, iv = rnd.randrange(1, 2**32), rnd.choice(species), rnd.randint(2, 100), rnd.choice([32, 31, 0, 15, 20])
        arm.w32(arm.sym("gRngValue"), seed)
        arm.w32(arm.sym("gSaveBlock2Ptr"), arm.sym("gSaveblock2")) if "gSaveblock2" in arm.syms else None
        arm.mu.mem_write(arm.sym("gSaveblock2"), bytes(0xF00)) if "gSaveblock2" in arm.syms else None
        arm.call("ZeroPlayerPartyMons")
        arm.call("CreateMon", gparty, sp, lvl, iv, 0, 0, 0, 0)
        a = arm.read(gparty, host.pokemon_size)
        L.hb_createmon_dump(seed, sp, lvl, iv, buf)
        # bytes after the nickname's 0xFF terminator are residue of whatever string buffer was used
        # last (the ROM has it too); they carry no state, so mask them on both sides
        a, b = bytearray(a), bytearray(buf.raw)
        for x in (a, b):
            t = x.find(0xFF, 8, 18)
            if t >= 0:
                x[t + 1:18] = bytes(17 - t)
        a, b = bytes(a), bytes(b)
        if a == b:
            ok += 1
        elif first is None:
            diff = [i for i in range(len(a)) if a[i] != b[i]]
            first = {"seed": seed, "species": sp, "level": lvl, "iv": iv, "differing bytes": diff[:12]}
    rep.add("createmon", ok, n, first)


def test_damage(arm, host, n, rep):
    L = host.lib
    sz = (C.c_int * 16)()
    L.hb_abi(sz)
    bsize, flags_off = sz[8], 4  # flags is the 2nd pointer of BattleResources: offset 4 on the ARM (sz[9] is the x86-64 offset)
    L.hb_battlemon.argtypes = [C.c_uint, C.c_int, C.c_int, C.c_int, C.c_void_p, C.c_int, C.c_void_p, C.c_uint, C.c_void_p]
    L.hb_damage.argtypes = [C.c_void_p, C.c_void_p, C.c_int, C.c_int]
    rnd = random.Random(3)
    c = host.c
    species = [v for k, v in c.species.items() if 1 <= v <= 151]
    moves = [v for k, v in c.moves.items() if 1 <= v <= 165]
    boost_items = [c.items[k] for k in ("CHARCOAL", "MYSTIC_WATER", "MIRACLE_SEED", "MAGNET", "BLACK_BELT", "SHARP_BEAK", "POISON_BARB",
                                        "SOFT_SAND", "SILVER_POWDER", "SPELL_TAG", "TWISTED_SPOON", "NEVER_MELT_ICE", "HARD_STONE", "SILK_SCARF",
                                        "CHOICE_BAND", "LIGHT_BALL", "THICK_CLUB", "METAL_POWDER", "POTION") if k in c.items] + [0, 0, 0]
    # a zeroed BattleStruct and BattleResources (with their flags) for the code that dereferences them
    # (in an extra mapped region: EWRAM is nearly all in use by the ROM's own variables)
    arm.mu.mem_map(0x0E000000, 0x10000)
    bs, br, fl = 0x0E000000, 0x0E004000, 0x0E005000
    arm.w32(arm.sym("gBattleStruct"), bs)
    arm.w32(arm.sym("gBattleResources"), br)
    arm.w32(br + flags_off, fl)
    gb = arm.sym("gBattleMons")
    ok, first, tot = 0, None, 0
    for _ in range(n):
        mon = []
        for k in range(2):
            mv = (C.c_ushort * 4)(*[rnd.choice(moves) for _ in range(4)])
            st = (C.c_byte * 8)(*[rnd.randint(3, 9) for _ in range(8)])
            buf = (C.c_ubyte * bsize)()
            L.hb_battlemon(rnd.randrange(1, 2**32), rnd.choice(species), rnd.randint(2, 100), rnd.choice([31, 15]), mv, rnd.choice(boost_items),
                           st, rnd.choice([0, 0, 0x10, 0x40]), buf)
            mon.append(bytes(buf))
        move, side = rnd.choice(moves), rnd.choice([0, 0, 1, 2, 3])
        if os.environ.get("ORACLE_TRACE"): print("damage", host.c.move_name.get(move), side, file=sys.stderr, flush=True)
        h = L.hb_damage(mon[0], mon[1], move, side)
        if h == -1:
            continue  # status move: no damage calc
        arm.mu.mem_write(gb, bytes(bsize * 2))
        arm.mu.mem_write(gb, mon[0] + mon[1])
        arm.mu.mem_write(arm.sym("gBattleTypeFlags"), bytes(4))
        arm.mu.mem_write(arm.sym("gBattleWeather"), bytes(2))
        arm.w16(arm.sym("gBattleMovePower"), 0)
        arm.mu.mem_write(arm.sym("gCritMultiplier"), bytes([1]))
        tot += 1
        a = arm.call("CalculateBaseDamage", gb, gb + bsize, move, side, 0, 0, 0, 1)
        a = a - (1 << 32) if a & 0x80000000 else a
        if a == h:
            ok += 1
        elif first is None:
            first = {"move": host.c.move_name.get(move), "arm": a, "host": h, "side": side}
        if a != h and os.environ.get("ORACLE_TRACE"): print("DIFF", move, side, a, h, mon[0][:80].hex(), file=sys.stderr)
    rep.add("damage", ok, tot, first)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", help="ROM ELF (default: first of %s)" % ", ".join(ELF_CANDIDATES))
    ap.add_argument("--n", type=int, default=300, help="cases per test")
    a = ap.parse_args()
    elf = find_elf(a.elf)
    check_fresh(elf)
    print("oracle: %s vs build/host/libminasim.so" % os.path.relpath(elf, ROOT))
    arm, host, rep = Arm(elf), load_host(), Report()
    test_rng(arm, host, a.n, rep)
    test_createmon(arm, host, a.n, rep)
    test_damage(arm, host, a.n, rep)
    print("oracle:", "FAILED" if rep.fail else "all identical")
    return 1 if rep.fail else 0


if __name__ == "__main__":
    sys.exit(main())
