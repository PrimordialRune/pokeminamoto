#!/usr/bin/env python3
"""No-RTC check of the slice's timed content (ROM from make release REGRESS=1 DINFO=1).

    python3 tools/regress/slice_clock.py --out DIR [--rom ROM] [--elf ELF]

For every hour 0-23 and every slice map that shows or hides things by the clock (the hour windows of
showduringhours/getgamehour, the time-of-day phases of showduringtime/checktimeofday):

  reference  boot with the cartridge clock set to that hour (GBAEMU_RTC, no override), warp into each map,
             and read the whole flag block (the hide flags the ON_TRANSITION scripts set) and the day/night phase.
  no RTC     boot a ROM whose SiiRtc read routines are stubbed (slice_lag.Rig no_rtc), force the play time to
             the value the fallback clock maps to that hour (src/day_night.c: hour = (play minutes / 5 + 10) % 24),
             warp into the same maps and read the same.

Both must match. Also checked without an RTC: the main menu comes up (no 'battery dry' message), the fallback
clock advances with play time, and the phase follows it. Writes DIR/slice_clock.json; exit 1 on a mismatch.
"""
import argparse
import json
import os
import shutil
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402
import slice_lag as S  # noqa: E402

MAPS = ["MasaraTown", "PalletTown_PlayersHouse_1F", "Route1", "ViridianCity", "ViridianCity_Mart", "ViridianCity_School",
        "ViridianCity_House", "ViridianCity_PokemonCenter_1F", "ViridianCity_PokemonCenter_2F", "PewterCity",
        "PewterCity_CVS", "PewterCity_Museum_1F", "PewterCity_PokemonCenter_1F", "PewterCity_PokemonCenter_2F", "Route3"]
FLAGS_OFF = 3904   # offsetof(struct SaveBlock1, flags), measured with gdb on the ELF
SB2_PLAYTIME = 14  # playTimeHours (u16), playTimeMinutes (u8), playTimeSeconds (u8), playTimeVBlanks (u8)
MINUTES_PER_HOUR = 5
IGNORE = set()
HOUR_BEGIN = 10


def flag_block(rig):
    e = rig.emu
    sb = e.u32(e.sym["gSaveBlock1Ptr"])
    return e.read(sb + FLAGS_OFF, 300)


def set_playtime_for_hour(rig, hour):
    e = rig.emu
    sb2 = e.u32(e.sym["gSaveBlock2Ptr"])
    total = ((hour - HOUR_BEGIN) % 24) * MINUTES_PER_HOUR + 2    # mid-slot of the 5 minutes
    e.cmd(f"write {sb2 + SB2_PLAYTIME:#x} " + struct.pack("<HBBB", total // 60, total % 60, 0, 0).hex())


def read_phase(rig):
    return rig.emu.u8(rig.emu.sym["sTimeOfDay"])


def sweep(rig, hour, no_rtc, starts):
    out = {}
    if no_rtc:
        set_playtime_for_hour(rig, hour)
        rig.emu.run(130)            # the fallback hour is re-read every 60 frames
    for m in MAPS:
        if no_rtc:
            set_playtime_for_hour(rig, hour)
        rig.warp(m, *starts[m])
        out[m] = dict(flags=flag_block(rig).hex(), phase=read_phase(rig))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--rom", default=os.path.join(R.ROOT, "pokeemerald_modern_release_regress.gba"))
    ap.add_argument("--elf", default=None)
    ap.add_argument("--out", required=True)
    ap.add_argument("--hours", default="")
    args = ap.parse_args()
    elf = args.elf or args.rom.replace(".gba", ".elf")
    if os.path.exists(args.out):
        shutil.rmtree(args.out)
    os.makedirs(args.out)
    hours = [int(h) for h in args.hours.split(",")] if args.hours else list(range(24))
    starts = {m: S.start_tile(m)[0] for m in MAPS}
    S.CONSTS.update(S.consts(["VAR_TIME_OF_DAY_OVERRIDE", "FLAG_HIDE_OLDALE_NIGHT_NPC", "FLAG_HIDE_OLDALE_LATE_NPC"]))
    IGNORE.update([S.CONSTS["FLAG_HIDE_OLDALE_NIGHT_NPC"], S.CONSTS["FLAG_HIDE_OLDALE_LATE_NPC"]])

    report = dict(hours={}, mismatches=[])
    # --- no RTC: one boot, every hour
    d = os.path.join(args.out, "noRTC")
    os.makedirs(d)
    rig = S.Rig(args.rom, elf, d, no_rtc=True)
    S.boot(rig, True)
    st = S.rtc_state(rig)
    report["noRtcBoot"] = st
    print("no-RTC boot:", st, flush=True)
    # the main menu must not have shown the 'battery dry' error: boot() got through it
    base_ss = os.path.join(d, "boot.ss")
    rig.emu.cmd(f"savestate {base_ss}")     # every hour starts from the same clean new game
    nort = {}
    for h in hours:
        rig.emu.cmd(f"loadstate {base_ss}")
        rig.emu.run(2)
        nort[h] = sweep(rig, h, True, starts)
        phase = nort[h][MAPS[0]]["phase"]
        hr = rig.emu.u8(rig.emu.sym["sHourPlusOne"]) - 1
        print(f"  no RTC hour {h:2d}: fallback clock says {hr:2d}, phase {phase}", flush=True)
        if hr != h:
            report["mismatches"].append(f"hour {h}: fallback clock reads {hr}")
    # the clock advances with play time: let 12 minutes of play pass (one game hour)
    rig.emu.cmd(f"loadstate {base_ss}")
    set_playtime_for_hour(rig, 12)
    rig.emu.run(130)
    h0 = rig.emu.u8(rig.emu.sym["sHourPlusOne"]) - 1
    sb2 = rig.emu.u32(rig.emu.sym["gSaveBlock2Ptr"])
    minutes = rig.emu.u8(sb2 + SB2_PLAYTIME + 2)
    rig.emu.cmd(f"write {sb2 + SB2_PLAYTIME + 2:#x} {(minutes + MINUTES_PER_HOUR) % 60:02x}")   # +5 minutes of play
    rig.emu.run(130)
    h1 = rig.emu.u8(rig.emu.sym["sHourPlusOne"]) - 1
    report["advance"] = dict(before=h0, after=h1)
    print(f"  five play minutes: hour {h0} -> {h1}", flush=True)
    if h1 != (h0 + 1) % 24:
        report["mismatches"].append(f"play time did not advance the clock ({h0} -> {h1})")
    rig.emu.cmd("quit")

    # --- reference: the real clock at each hour
    for h in hours:
        d = os.path.join(args.out, f"rtc{h:02d}")
        os.makedirs(d)
        os.environ["GBAEMU_RTC"] = str(1767268800 + (h - 12) * 3600)    # 2026-01-01 12:00 UTC + (h - 12) hours
        r = S.Rig(args.rom, elf, d)
        S.boot(r, False)
        ref = sweep(r, h, False, starts)
        got = S.rtc_state(r)
        r.emu.cmd("quit")
        bad = []
        for m in MAPS:
            if ref[m]["phase"] != nort[h][m]["phase"]:
                bad.append(f"{m}: phase {ref[m]['phase']} (RTC) vs {nort[h][m]['phase']} (no RTC)")
            if ref[m]["flags"] != nort[h][m]["flags"]:
                a, b = bytes.fromhex(ref[m]["flags"]), bytes.fromhex(nort[h][m]["flags"])
                diff = [(i * 8 + k) for i in range(300) for k in range(8) if (a[i] ^ b[i]) >> k & 1]
                diff = [x for x in diff if x not in IGNORE]    # Oldale is where REGRESS setup leaves the player: set at boot time
                if not diff:
                    continue
                bad.append(f"{m}: {len(diff)} flags differ (ids {[hex(x) for x in diff[:8]]})")
        report["hours"][h] = dict(rtcHourSeen=got["hourPlusOne"] - 1, differences=bad)
        print(f"  hour {h:2d}: RTC says {got['hourPlusOne'] - 1:2d}, {'OK' if not bad else 'DIFF ' + '; '.join(bad)}", flush=True)
        report["mismatches"] += [f"hour {h} {b}" for b in bad]
    with open(os.path.join(args.out, "slice_clock.json"), "w") as f:
        json.dump(report, f, indent=1)
    print("no-RTC check:", "PASS" if not report["mismatches"] else f"FAIL ({len(report['mismatches'])} mismatches)")
    return 1 if report["mismatches"] else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except R.Fail as e:
        print("FAIL:", e)
        sys.exit(2)
