#!/usr/bin/env python3
"""Run the in-ROM rule tests (src/test/, include/test_runner.h) in the headless emulator.

    make release TEST=1 && python3 tools/test/run_tests.py pokeemerald_modern_release_test.gba

The test ROM boots straight into the test runner, runs every test and leaves the outcome in
gTestResults (EWRAM). This script advances the emulator until the runner reports it is done,
reads the results and prints one line per test. Exit status 0 only if all tests passed.
Option --json FILE also writes the results as JSON.
"""
import argparse
import json
import os
import struct
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EMU = os.path.join(ROOT, "tools", "gba-emu", "gbaemu")
MAGIC = 0x54534554
STATE_DONE = 3
HEADER = struct.Struct("<5I")      # magic, state, count, passed, failed
ENTRY = struct.Struct("<IBBHii")   # name ptr, status, kind, line, got, want
MAX_ENTRIES = 96
STATUS = {0: "NOTRUN", 1: "PASS", 2: "FAIL"}


class Emu:
    def __init__(self):
        env = dict(os.environ, GBAEMU_RTC=os.environ.get("GBAEMU_RTC", "1767268800"))
        self.p = subprocess.Popen([EMU], env=env, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                  text=True, bufsize=1, cwd=ROOT)

    def cmd(self, line):
        self.p.stdin.write(line + "\n")
        reply = json.loads(self.p.stdout.readline())
        if not reply.get("ok"):
            raise RuntimeError(f"emu: {line}: {reply}")
        return reply

    def read(self, addr, n):
        reply = self.cmd(f"read 0x{addr:08x} {n}")
        return bytes.fromhex(reply["hex"] if "hex" in reply else reply["data"])

    def close(self):
        try:
            self.p.stdin.write("quit\n")
            self.p.stdin.flush()
        except Exception:
            pass
        self.p.wait(timeout=10)


def symbol(elf, name):
    out = subprocess.run(["arm-none-eabi-nm", elf], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == name:
            return int(parts[0], 16)
    raise SystemExit(f"symbol {name} not found in {elf}: is this a TEST=1 build?")


def cstring(emu, addr):
    if addr == 0:
        return "?"
    data = emu.read(addr, 64)
    return data.split(b"\0")[0].decode("ascii", "replace")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rom", help="the TEST=1 ROM (its .elf must be next to it)")
    ap.add_argument("--max-frames", type=int, default=60000)
    ap.add_argument("--json", help="write the results to this file")
    ap.add_argument("-q", "--quiet", action="store_true", help="list only failures")
    args = ap.parse_args()

    rom = os.path.abspath(args.rom)
    elf = os.path.splitext(rom)[0] + ".elf"
    results_addr = symbol(elf, "gTestResults")
    emu = Emu()
    t0 = time.time()
    try:
        emu.cmd("load " + rom)
        frames = 0
        state = 0
        while frames < args.max_frames:
            emu.cmd("run 60")
            frames += 60
            head = HEADER.unpack(emu.read(results_addr, HEADER.size))
            if head[0] == MAGIC and head[1] == STATE_DONE:
                state = STATE_DONE
                break
        raw = emu.read(results_addr, HEADER.size + MAX_ENTRIES * ENTRY.size)
        magic, state_now, count, passed, failed = HEADER.unpack_from(raw, 0)
        tests = []
        for i in range(min(count, MAX_ENTRIES)):
            ptr, status, kind, line, got, want = ENTRY.unpack_from(raw, HEADER.size + i * ENTRY.size)
            tests.append({"name": cstring(emu, ptr), "status": STATUS.get(status, "?"),
                          "kind": "battle" if kind else "pure", "line": line, "got": got, "want": want})
        elapsed = time.time() - t0
    finally:
        emu.close()

    if state != STATE_DONE:
        print(f"FAIL: the runner did not finish within {args.max_frames} frames "
              f"(state {state_now}, {count} tests started)")
        for t in tests[-3:]:
            print("  last:", t)
        return 2

    for t in tests:
        if t["status"] == "PASS":
            if not args.quiet:
                print(f"  ok    {t['kind']:6s} {t['name']}")
        else:
            print(f"  FAIL  {t['kind']:6s} {t['name']}  line {t['line']} got {t['got']} (0x{t['got'] & 0xFFFFFFFF:x}) want {t['want']}")
    print(f"{passed} passed, {failed} failed of {count} tests, {frames} frames, {elapsed:.1f}s")
    if args.json:
        with open(args.json, "w") as f:
            json.dump({"passed": passed, "failed": failed, "frames": frames, "seconds": elapsed, "tests": tests}, f, indent=1)
    return 0 if failed == 0 and passed == count else 1


if __name__ == "__main__":
    sys.exit(main())
