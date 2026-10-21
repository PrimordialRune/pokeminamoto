#!/usr/bin/env python3
"""GBA-style wave animation for every water tile of a map mockup.

Every pixel of the flat water color becomes a loop (16 frames by default) on a
32x32 repeat, plus foam that laps along the banks and around the pier posts.
--style picks the waves: swell (Emerald diagonal swells, adds a trough shade),
soft (thinner swells with a mid-tone crest, adds two shades), gsc (GBC ripple
lines, two colors only) or calm (FRLG-pond swells, adds a mid tone).

Writes:
  OUT_MAP.png           the mockup with frame 0 drawn in (feed it to map2porytiles.py)
  ANIM_DIR/key.png      one strip with every animated 8x8 tile, frame 0
  ANIM_DIR/f0..f7.png   the same strip for each frame
Add ANIM_DIR's parent anim.json entry (see tools/porytiles/README.md) and
compile; Porytiles writes the tileset_anims.c code.

Porytiles links frames to the tileset through the key frame, so two animated
tiles that look the same in frame 0 must animate the same way. The script
checks that, and nudges frame 0 of any tile that breaks it.

Usage:
  water_anim.py MAP.png OUT_MAP.png ANIM_DIR [--style soft] [--frames 16] [--seed 1] [--preview water.gif]
"""

import argparse
import os
import random
from collections import deque

import numpy as np
from PIL import Image

WATER = np.array([0x6B, 0x63, 0xFF])
FOAM = np.array([0xA8, 0xA3, 0xFF])
FRAMES = 8
REPEAT = 32
CALM_BAND = 0

DARK = np.array([0x58, 0x50, 0xE0])   # trough shade; the one color this adds
FLIPS = [lambda t: t, lambda t: t[:, ::-1], lambda t: t[::-1], lambda t: t[::-1, ::-1]]
TAU = 2 * np.pi


def crests(seed):
    """Phases of the cross waves; another seed gives another (equally valid) pattern."""
    r = random.Random(seed)
    return [r.uniform(0, TAU) for _ in range(3)]


MID = np.array([0x88, 0x80, 0xFF])    # crest shoulder for the soft and calm styles


def swells(k, ph):
    y, x = np.mgrid[0:REPEAT, 0:REPEAT]
    t = k / FRAMES
    return (np.sin(TAU * ((-x + 2 * y) / REPEAT - t))
            + 0.35 * np.sin(TAU * (3 * x - y) / REPEAT + ph[0]) * np.cos(TAU * t)
            + 0.30 * np.sin(TAU * ((-x + 3 * y) / REPEAT - t) + ph[1])
            + 0.35 * np.sin(TAU * (2 * x + y) / REPEAT + ph[2]) * np.sin(TAU * t + 0.5))


def wave_swell(k, ph):
    """Emerald-style: diagonal swells (wave vector (-1, 2), one band per loop)
    broken into blobs by cross waves that bob in place. Troughs take the dark
    shade, the lit edge above each trough the foam color."""
    f = swells(k, ph)
    a = np.zeros((REPEAT, REPEAT, 3), np.uint8)
    a[:] = WATER
    dark = f < -0.65
    a[dark] = DARK
    a[~dark & np.roll(dark, -1, 0)] = FOAM
    return a


def wave_soft(k, ph):
    """The same swells with thinner troughs; the crest fades out through a mid tone."""
    f = swells(k, ph)
    a = np.zeros((REPEAT, REPEAT, 3), np.uint8)
    a[:] = WATER
    dark = f < -0.95
    a[dark] = DARK
    rim = ~dark & np.roll(dark, -1, 0)
    a[rim & (f > -1.05)] = FOAM
    a[np.roll(rim, -1, 1) & ~dark & ~rim] = MID
    return a


def wave_gsc(k, ph):
    """GBC-style: four rows of broken horizontal ripple lines that sway, two colors."""
    t = k / FRAMES
    a = np.zeros((REPEAT, REPEAT, 3), np.uint8)
    a[:] = WATER
    for row, y0 in enumerate((3, 11, 19, 27)):
        off = row * 0.37
        for x in range(REPEAT):
            y = y0 + int(np.round(0.9 * np.sin(TAU * (x / REPEAT + off + t))))
            g = np.sin(TAU * (x / 16 + off * 2 - t)) + 0.5 * np.sin(TAU * (x / REPEAT * 3 + row))
            if g > 0.35:
                a[y % REPEAT, x] = FOAM
    return a


def wave_calm(k, ph):
    """FRLG-pond style: low-contrast horizontal swells with broken light crests."""
    y, x = np.mgrid[0:REPEAT, 0:REPEAT]
    t = k / FRAMES
    f = (np.sin(TAU * (2 * y / REPEAT + 0.12 * np.sin(TAU * x / REPEAT + ph[0]) - t))
         + 0.30 * np.sin(TAU * (x + 2 * y) / REPEAT + ph[1]) * np.cos(TAU * t)
         + 0.25 * np.sin(TAU * (-2 * x + 2 * y) / REPEAT + ph[2]) * np.sin(TAU * t))
    e = (np.sin(TAU * ((2 * x + y) / REPEAT - t) + ph[0])
         + 0.6 * np.sin(TAU * (-x + y) / REPEAT + ph[1]) * np.cos(TAU * t))
    a = np.zeros((REPEAT, REPEAT, 3), np.uint8)
    a[:] = WATER
    crest = (f >= np.roll(f, 1, 0)) & (f > np.roll(f, -1, 0))
    hi = crest & (f > 0.8) & (e > 0.1)
    a[hi] = FOAM
    a[np.roll(hi, 1, 0)] = MID
    a[crest & (f > 0.5) & ~hi & (e > -0.6)] = MID
    return a


STYLES = {'swell': wave_swell, 'soft': wave_soft, 'gsc': wave_gsc, 'calm': wave_calm}
wave = wave_calm


def shore_distance(water):
    H, W = water.shape
    d = np.where(water, 99, 0)
    q = deque(zip(*np.nonzero(~water)))
    while q:
        y, x = q.popleft()
        for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            yy, xx = y + dy, x + dx
            if 0 <= yy < H and 0 <= xx < W and d[yy, xx] > d[y, x] + 1:
                d[yy, xx] = d[y, x] + 1
                q.append((yy, xx))
    return d


def build(m, water, dist, cs):
    ys, xs = np.nonzero(water)
    dd = dist[ys, xs]
    out = []
    for k in range(FRAMES):
        f = m.copy()
        f[ys, xs] = wave(k, cs)[ys % REPEAT, xs % REPEAT]
        if CALM_BAND:
            # Still water along the bank: shore tiles then depend only on the
            # bank shape and the foam, not on where the swells fall.
            near = dd <= CALM_BAND
            f[ys[near], xs[near]] = WATER
        # Foam periods divide the 32-pixel repeat, so tiles stay periodic.
        s1 = (dd == 1) & (((xs + ys) // 2 + k) % 4 != 0)
        s2 = (dd == 2) & (((xs - ys) // 2 + k) % 4 == 0) & (k % 4 < 2)
        s3 = (dd == 3) & (((xs + ys) // 2 + k) % 8 == 0) & (k % 4 == 1)
        sel = s1 | s2 | s3
        f[ys[sel], xs[sel]] = FOAM
        out.append(f)
    return np.stack(out)


def animated_tiles(F, water, fix):
    """Unique animated tiles (frame sequences). With fix, rewrite frame 0 of tiles
    whose key frame clashes with a different sequence; returns the clash count.
    Copies of one sequence get the same rewrite, so they still share a tile."""
    _, H, W, _ = F.shape
    seqs, tiles, clashes, nudged = {}, [], 0, {}

    def keys(seq):
        kb, i = min((FLIPS[i](seq[0]).tobytes(), i) for i in range(4))
        return kb, np.stack([FLIPS[i](s) for s in seq]).tobytes()

    for ty in range(0, H, 8):
        for tx in range(0, W, 8):
            if not water[ty:ty + 8, tx:tx + 8].any():
                continue
            seq = F[:, ty:ty + 8, tx:tx + 8]
            kb, cs = keys(seq)
            if kb in seqs and seqs[kb] != cs:
                clashes += 1
                if not fix:
                    continue
                canon = min(np.stack([FLIPS[i](s) for s in seq]).tobytes() for i in range(4))
                if canon not in nudged:
                    # first later frame whose image is free to serve as this key
                    nudged[canon] = next((j for j in range(1, FRAMES)
                                          if keys(np.concatenate([seq[j:j + 1], seq[1:]]))[0] not in seqs), 1)
                seq[0] = seq[nudged[canon]]
                kb, cs = keys(seq)
            if kb in seqs:
                continue
            seqs[kb] = cs
            tiles.append(seq.copy())
    return tiles, clashes


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('map')
    ap.add_argument('out_map')
    ap.add_argument('anim_dir')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--style', choices=sorted(STYLES), default='calm')
    ap.add_argument('--frames', type=int, default=16, help='8 or 16 (the loop must divide 16 steps)')
    ap.add_argument('--repeat', type=int, default=32, help='wave tile size in pixels: 32, or 16 (fewer tiles)')
    ap.add_argument('--calm-band', type=int, default=0, help='no swells within this many pixels of the bank')
    ap.add_argument('--preview')
    a = ap.parse_args()

    global FRAMES, REPEAT, CALM_BAND, wave
    FRAMES, REPEAT, CALM_BAND, wave = a.frames, a.repeat, a.calm_band, STYLES[a.style]
    m = np.array(Image.open(a.map).convert('RGB'))
    water = (m == WATER).all(-1)
    dist = shore_distance(water)
    for seed in range(a.seed, a.seed + 50):
        F = build(m, water, dist, crests(seed))
        for _ in range(10):
            _, clashes = animated_tiles(F, water, fix=True)
            if not clashes:
                break
        tiles, clashes = animated_tiles(F, water, fix=False)
        if not clashes:
            break
    else:
        raise SystemExit(f'animated tiles still clash for seeds {a.seed}-{a.seed + 49}')

    Image.fromarray(F[0]).save(a.out_map)
    os.makedirs(a.anim_dir, exist_ok=True)
    n = len(tiles)
    for k in range(FRAMES + 1):
        img = np.zeros((8, 8 * n, 3), np.uint8)
        for i, seq in enumerate(tiles):
            img[:, i * 8:i * 8 + 8] = seq[0 if k == FRAMES else k]
        Image.fromarray(img).save(os.path.join(a.anim_dir, 'key.png' if k == FRAMES else f'f{k}.png'))
    if a.preview:
        ys, xs = np.nonzero(water)
        y0, y1 = max(ys.min() - 32, 0), ys.max() + 1
        x0, x1 = xs.min(), xs.max() + 1
        imgs = [Image.fromarray(f[y0:y1, x0:x1]) for f in F]
        imgs = [im.resize((im.width * 2, im.height * 2), Image.NEAREST) for im in imgs]
        imgs[0].save(a.preview, save_all=True, append_images=imgs[1:], duration=267, loop=0)
    print(f'{n} animated tiles, {FRAMES} frames, {a.style}, seed {seed}')


if __name__ == '__main__':
    main()
