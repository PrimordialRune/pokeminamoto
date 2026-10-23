#!/usr/bin/env python3
"""Compare two tools/regress runs and the RAM/ROM use of their builds.

    python3 tools/regress/compare.py BASE_DIR NEW_DIR [--base-elf X.elf --new-elf Y.elf] [--md out.md]

Checks, in order of how much they matter:
  1. The game-state dumps (party, PC, bag, money, Pokedex, flags) at every
     checkpoint are identical. Any difference is printed field by field.
  2. Saving, resetting and continuing gives back exactly the saved state.
  3. No scene ran out of heap or overflowed the DMA queue.
  4. Per-scene heap peak, DMA peak, lag frames and CPU busy lines, side by side.
  5. Checkpoint screenshots, pixel by pixel (differences are saved as
     side-by-side PNGs in NEW_DIR/diff/).
  6. EWRAM / IWRAM / ROM use of the two ELFs.
Exit code 0 when 1-3 pass.
"""
import argparse
import json
import os
import subprocess
import sys

from PIL import Image, ImageChops


def load(d):
    with open(os.path.join(d, "report.json")) as f:
        return json.load(f)


def diff_obj(a, b, path=""):
    out = []
    if isinstance(a, dict) and isinstance(b, dict):
        for k in sorted(set(a) | set(b)):
            out += diff_obj(a.get(k), b.get(k), f"{path}.{k}")
    elif isinstance(a, list) and isinstance(b, list) and len(a) == len(b):
        for i, (x, y) in enumerate(zip(a, b)):
            out += diff_obj(x, y, f"{path}[{i}]")
    elif a != b:
        out.append(f"{path}: {a} -> {b}")
    return out


def mem_use(elf):
    if not elf or not os.path.exists(elf):
        return None
    use = {"ewram": 0, "iwram": 0, "rom": 0}
    out = subprocess.run(["arm-none-eabi-size", "-A", elf], capture_output=True, text=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) != 3 or not parts[1].isdigit():
            continue
        size, addr = int(parts[1]), int(parts[2])
        if 0x02000000 <= addr < 0x02040000:
            use["ewram"] += size
        elif 0x03000000 <= addr < 0x03008000:
            use["iwram"] += size
        elif 0x08000000 <= addr < 0x0A000000:
            use["rom"] += size
    return use


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("base")
    ap.add_argument("new")
    ap.add_argument("--base-elf")
    ap.add_argument("--new-elf")
    ap.add_argument("--md")
    ap.add_argument("--allow", action="append", default=[],
                    help="dump field prefix allowed to differ, e.g. .party[0].otId")
    args = ap.parse_args()
    a, b = load(args.base), load(args.new)
    lines = []
    ok = True

    lines.append("## Game state at each checkpoint")
    for name in a["dumps"]:
        d = diff_obj(a["dumps"][name], b["dumps"].get(name))
        bad = [x for x in d if not any(x.startswith(p) for p in args.allow)]
        allowed = len(d) - len(bad)
        if bad:
            ok = False
            lines.append(f"- **{name}: {len(bad)} difference(s)**")
            lines += [f"  - `{x}`" for x in bad[:40]]
        else:
            lines.append(f"- {name}: identical" + (f" ({allowed} allowed differences)" if allowed else ""))

    rt_a, rt_b = a["checks"]["saveRoundTrip"], b["checks"]["saveRoundTrip"]
    lines.append("")
    lines.append(f"## Save, reset, continue\n- base: {'exact' if rt_a else 'MISMATCH'}; new: {'exact' if rt_b else 'MISMATCH'}")
    ok &= bool(rt_b)

    lines.append("")
    lines.append("## Scenes (heap peak in bytes, DMA queue peak, lag frames, CPU busy scanlines avg/max of 228)")
    lines.append("| scene | heap base | heap new | alloc fails | DMA base/new | lag base/new | busy base | busy new |")
    lines.append("|---|---:|---:|---:|---|---|---|---|")
    for name, sa in a["scenes"].items():
        sb = b["scenes"].get(name)
        if sb is None:
            lines.append(f"| {name} | {sa['heapPeak']} | MISSING | | | | | |")
            ok = False
            continue
        if sb["heapFails"] or sb["dmaFull"]:
            ok = False
        lines.append(f"| {name} | {sa['heapPeak']} | {sb['heapPeak']} | {sb['heapFails']} | "
                     f"{sa['dmaPeak']}/{sb['dmaPeak']} | {sa['lagFrames']}/{sb['lagFrames']} | "
                     f"{sa['busyAvg']}/{sa['busyMax']} | {sb['busyAvg']}/{sb['busyMax']} |")
    lines.append(f"\nHeap peak over the run: base {max(s['heapPeak'] for s in a['scenes'].values())}, "
                 f"new {max(s['heapPeak'] for s in b['scenes'].values())}")

    lines.append("")
    lines.append("## Checkpoint screenshots")
    os.makedirs(os.path.join(args.new, "diff"), exist_ok=True)
    shots_a = {**{k: v["shot"] for k, v in a["scenes"].items()}, **a.get("snaps", {})}
    shots_b = {**{k: v["shot"] for k, v in b["scenes"].items()}, **b.get("snaps", {})}
    same = 0
    for name, pa in shots_a.items():
        pb = shots_b.get(name)
        if not pb:
            lines.append(f"- {name}: MISSING in new run")
            continue
        ia = Image.open(os.path.join(args.base, pa)).convert("RGB")
        ib = Image.open(os.path.join(args.new, pb)).convert("RGB")
        bbox = ImageChops.difference(ia, ib).getbbox()
        if bbox is None:
            same += 1
        else:
            px = sum(1 for p in ImageChops.difference(ia, ib).getdata() if p != (0, 0, 0))
            side = Image.new("RGB", (ia.width * 2 + 4, ia.height), (255, 0, 255))
            side.paste(ia, (0, 0))
            side.paste(ib, (ia.width + 4, 0))
            out = os.path.join(args.new, "diff", f"{name}.png")
            side.resize((side.width * 2, side.height * 2), Image.NEAREST).save(out)
            lines.append(f"- {name}: {px} pixels differ in {bbox} (see diff/{name}.png)")

    lines.append(f"- {same} of {len(shots_a)} screenshots pixel-identical")

    ma, mb = mem_use(args.base_elf), mem_use(args.new_elf)
    if ma and mb:
        lines.append("")
        lines.append("## Memory (release build)")
        lines.append("| | base | new | change |")
        lines.append("|---|---:|---:|---:|")
        for k, cap in (("ewram", 262144), ("iwram", 32768), ("rom", 33554432)):
            lines.append(f"| {k.upper()} | {ma[k]:,} ({ma[k] * 100 / cap:.1f}%) | {mb[k]:,} ({mb[k] * 100 / cap:.1f}%) | {mb[k] - ma[k]:+,} |")

    lines.append("")
    lines.append("**Result: PASS**" if ok else "**Result: FAIL**")
    text = "\n".join(lines)
    print(text)
    if args.md:
        with open(args.md, "w") as f:
            f.write(text + "\n")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
