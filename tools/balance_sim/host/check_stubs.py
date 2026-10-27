#!/usr/bin/env python3
"""Fails when the set of auto-stubbed symbols of the host build changes.

gen.py link stubs every symbol the battle sources reference but the host layer does not define
(graphics, sound, overworld, link: F = function returning 0, D = zeroed 64 KB data). A new name
here means battle code started calling something new, which silently does nothing on the host.
Look at it: if it is inert (draws, plays a sound ...) add it with --update; if it changes game
state (a flag, the party, the RNG, a stat), implement it in hb_platform.c instead.

  check_stubs.py            compare build/host/stubs.txt with stubs_allowlist.txt (make check)
  check_stubs.py --update   rewrite stubs_allowlist.txt from the current build
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
CUR = os.path.join(ROOT, "build", "host", "stubs.txt")
ALLOW = os.path.join(HERE, "stubs_allowlist.txt")


def load(p):
    with open(p) as f:
        return {tuple(l.split()[:2]) for l in f if l.strip() and not l.startswith("#")}


def main():
    if not os.path.exists(CUR):
        sys.exit("check_stubs: %s missing; run `make balance-lib`" % CUR)
    cur = load(CUR)
    if "--update" in sys.argv:
        with open(ALLOW, "w") as f:
            f.write("# Auto-stubbed host symbols (F function returning 0, D zeroed data). See check_stubs.py.\n")
            for k, n in sorted(cur, key=lambda x: x[1]):
                f.write("%s %s\n" % (k, n))
        print("wrote %d symbols to %s" % (len(cur), ALLOW))
        return 0
    allow = load(ALLOW) if os.path.exists(ALLOW) else set()
    added, removed = sorted(cur - allow, key=lambda x: x[1]), sorted(allow - cur, key=lambda x: x[1])
    if not added and not removed:
        print("balance-check: %d auto-stubbed symbols, unchanged" % len(cur))
        return 0
    for k, n in added:
        print("NEW STUB   %s %s" % (k, n))
    for k, n in removed:
        print("GONE STUB  %s %s" % (k, n))
    print("\nbalance-check: the host build's stub set changed (see tools/balance_sim/host/check_stubs.py).\n"
          "If every new stub is inert for battle logic, run: python3 tools/balance_sim/host/check_stubs.py --update")
    return 1


if __name__ == "__main__":
    sys.exit(main())
