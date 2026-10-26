#!/usr/bin/env python3
"""Compare the ROM data of two builds symbol by symbol.

    python3 tools/speciesgen/symcmp.py OLD.elf NEW.elf [--only REGEX] [--ignore REGEX]

For every read-only data symbol (nm types r/R) of OLD it compares the bytes
with the symbol of the same name in NEW, read from the .gba next to each ELF.
Words that are pointers in both builds are compared by what they point at
(symbol name + offset), so moving data around does not count as a change;
everything else must be byte-identical. Prints the symbols that differ, and
the ones only one build has, and exits 1 if anything differs.

Used to show that generated tables (speciesgen, trainerproc) are the same data
as the hand-written ones they replaced.
"""
import bisect
import re
import subprocess
import sys

RANGES = ((0x02000000, 0x02040000), (0x03000000, 0x03008000), (0x08000000, 0x0A000000))


def is_ptr(v):
    return any(a <= v < b for a, b in RANGES)


class Build:
    def __init__(self, elf):
        self.rom = open(re.sub(r'\.elf$', '.gba', elf), 'rb').read()
        self.syms = {}
        allsyms = []
        labels = []
        out = subprocess.run(['arm-none-eabi-nm', '-S', '--defined-only', elf],
                             capture_output=True, text=True, check=True).stdout
        for line in out.splitlines():
            p = line.split()
            if len(p) == 3:            # a label without a size
                labels.append((int(p[0], 16), p[2]))
                continue
            if len(p) != 4:
                continue
            addr, size, typ, name = int(p[0], 16), int(p[1], 16), p[2], p[3]
            labels.append((addr, name))
            if size == 0:
                continue
            allsyms.append((addr, size, name))
            if typ in 'rR' and 0x08000000 <= addr < 0x0A000000:
                self.syms.setdefault(name, []).append((addr, size))
        labels.sort()
        self.starts = [a for a, n in labels]
        self.labels = labels

    def bytes(self, addr, size):
        return self.rom[addr - 0x08000000: addr - 0x08000000 + size]

    def where(self, v):
        """symbol+offset for a pointer value: the nearest symbol at or below it."""
        t = v & ~1
        i = bisect.bisect_right(self.starts, t) - 1
        if i < 0:
            return ('?%08x' % v, 0, 0)
        a = self.starts[i]
        # several names at one address: take the first, alphabetically
        names = sorted(n for x, n in self.labels[bisect.bisect_left(self.starts, a):i + 1])
        return (names[0], t - a, v & 1)


def same(old, new, a, b, size):
    x, y = old.bytes(a, size), new.bytes(b, size)
    if x == y:
        return None
    if a % 4 != b % 4:
        return 'bytes differ'
    for i in range(size):
        if x[i] == y[i]:
            continue
        w = (i - (a + i) % 4)          # start of the aligned word holding byte i
        if w < 0 or w + 4 > size:
            return 'byte %d differs' % i
        vo = int.from_bytes(x[w:w + 4], 'little')
        vn = int.from_bytes(y[w:w + 4], 'little')
        if not (is_ptr(vo) and is_ptr(vn) and old.where(vo) == new.where(vn)):
            return 'word at +%d: %08x -> %08x (%s -> %s)' % (w, vo, vn, old.where(vo)[0] if is_ptr(vo) else '-',
                                                            new.where(vn)[0] if is_ptr(vn) else '-')
    return None


def main():
    args = sys.argv[1:]
    only = ignore = None
    if '--only' in args:
        i = args.index('--only'); only = re.compile(args[i + 1]); del args[i:i + 2]
    if '--ignore' in args:
        i = args.index('--ignore'); ignore = re.compile(args[i + 1]); del args[i:i + 2]
    old, new = Build(args[0]), Build(args[1])
    bad = 0
    names = sorted(set(old.syms) | set(new.syms))
    checked = 0
    for n in names:
        if only and not only.search(n):
            continue
        if ignore and ignore.search(n):
            continue
        o, w = old.syms.get(n, []), new.syms.get(n, [])
        if not w:
            print('only in old: %s (%d bytes)' % (n, sum(s for a, s in o))); bad += 1
            continue
        if not o:
            print('only in new: %s (%d bytes)' % (n, sum(s for a, s in w))); bad += 1
            continue
        if len(o) != len(w):
            print('%s: %d copies in old, %d in new' % (n, len(o), len(w))); bad += 1
            continue
        for (a, sa), (b, sb) in zip(o, w):
            checked += 1
            if sa != sb:
                print('%s: size %d -> %d' % (n, sa, sb)); bad += 1
                continue
            why = same(old, new, a, b, sa)
            if why:
                print('%s: %s' % (n, why)); bad += 1
    print('%d symbols compared, %d differences' % (checked, bad))
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
