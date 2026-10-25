#!/usr/bin/env python3
"""Converts a song from pret/pokered (Red/Green/Blue) to an m4a song on the
GBA's PSG channels.

    python3 tools/gb2agb/pokered2agb.py POKERED_DIR Music_PalletTown mus_gb_pallet \
        [-o sound/songs/mus_gb_pallet.s]

The output is a self-contained .s (song header, tracks, voicegroup and wave
samples) that the Makefile's sound/songs/*.s rule assembles; commit it like
any other generated song. Add the song to sound/song_table.inc and
include/constants/songs.h to use it.

How it works. The GBA's PSG is the Game Boy's sound hardware, and m4a's CGB
code uses the same pitch table as Red's engine (gCgbFreqTable is Red's
Pitches table), so most of a song carries over exactly:

  * Timing: the converter runs Red's engine logic (note length x note speed
    x tempo / 256 with the fractional carry, per channel, every frame) and
    writes the song at TEMPO 150 = one m4a tick per frame, so every note
    starts on the frame it did on the Game Boy.
  * Pitch: Red's "octave o, note n" is m4a key 36 + 12 * (o - 1) + n, which
    gives the same frequency register value. toggle_perfect_pitch (+1 to the
    register) becomes TUNE; vibrato becomes MOD/LFOS/LFODL at the same depth
    and rate (m4a's LFO is a triangle, Red's a square wave).
  * Volume envelope: note_type's volume and fade are the NRx2 envelope.
    m4a writes NRx2 itself at note start with the voice's decay as the
    hardware envelope period, so the voice is attack 0, decay = fade,
    sustain 0 (15 when fade is 0), release 0, and the velocity is chosen so
    m4a's envelope goal equals Red's volume.
  * Duty and wave: one voice per duty (square) or wave instrument (wave
    channel, Red's 32-nibble samples are the same format as GBA wave RAM).
  * Drums: each drum_note plays Red's noise instrument (the SFX_Noise_
    Instrument*_1 sequences), step by step, on a noise voice; NR43 values
    are matched to m4a's noise key table by frequency.
  * Loops: sound_call/sound_ret and counted sound_loop are unrolled; the
    final infinite sound_loop becomes a GOTO to where the channel first
    reached that label.
  * Reverb: the header sets m4a's reverb to 0 (reverb_set+0). The song has
    no PCM voices, so this only removes the mixer's reverb pass, which is
    most of m4a's CPU time when no PCM sound plays.
"""
import argparse
import math
import os
import re
import sys
from fractions import Fraction

NOTE_NAMES = ["C_", "C#", "D_", "D#", "E_", "F_", "F#", "G_", "G#", "A_", "A#", "B_"]
PITCHES = [-2004, -1891, -1785, -1685, -1591, -1501, -1417, -1337, -1262, -1192, -1125, -1062]
NOISE_TABLE = [0xD7, 0xD6, 0xD5, 0xD4, 0xC7, 0xC6, 0xC5, 0xC4, 0xB7, 0xB6, 0xB5, 0xB4,
               0xA7, 0xA6, 0xA5, 0xA4, 0x97, 0x96, 0x95, 0x94, 0x87, 0x86, 0x85, 0x84,
               0x77, 0x76, 0x75, 0x74, 0x67, 0x66, 0x65, 0x64, 0x57, 0x56, 0x55, 0x54,
               0x47, 0x46, 0x45, 0x44, 0x37, 0x36, 0x35, 0x34, 0x27, 0x26, 0x25, 0x24,
               0x17, 0x16, 0x15, 0x14, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00]
WAIT_LENGTHS = list(range(0, 25)) + [28, 30, 32, 36, 40, 42, 44, 48, 52, 54, 56, 60, 64, 66,
                                     68, 72, 76, 78, 80, 84, 88, 90, 92, 96]
# Red's wave channel volume (note_type's 2nd argument) -> m4a envelope level:
# gCgb3Vol maps 14-15 to 100%, 6-9 to 50% and 2-5 to 25%.
WAVE_LEVEL = {0: 0, 1: 15, 2: 8, 3: 4}
MAX_FRAMES = 60 * 60 * 20


class ConvertError(Exception):
    pass


def warn(msg):
    print("pokered2agb: warning: " + msg, file=sys.stderr)


# ---------------------------------------------------------------- parsing

def parse_asm(text, filename):
    """Returns (labels, commands): labels map a full label name to an index
    into commands; commands are (name, [args]) tuples."""
    labels = {}
    commands = []
    scope = ""
    for lineno, raw in enumerate(text.splitlines(), 1):
        line = raw.split(";", 1)[0].strip()
        if not line:
            continue
        m = re.match(r"^([A-Za-z_.][\w.]*)(::?)\s*(.*)$", line)
        if m and not line.startswith(("note ", "rest ")):
            name = m.group(1)
            if name.startswith("."):
                name = scope + name
            else:
                scope = name
            labels[name] = len(commands)
            line = m.group(3).strip()
            if not line:
                continue
        parts = line.split(None, 1)
        args = [a.strip() for a in parts[1].split(",")] if len(parts) > 1 else []
        args = [scope + a if a.startswith(".") else a for a in args]
        commands.append((parts[0], args, f"{filename}:{lineno}"))
    return labels, commands


def num(s):
    s = s.strip()
    if s.startswith("$"):
        return int(s[1:], 16)
    if s.startswith("%"):
        return int(s[1:], 2)
    return int(s, 0)


class Source:
    def __init__(self, root):
        self.root = root
        self.labels = {}
        self.commands = []
        for sub in ("audio/music", "audio/sfx"):
            d = os.path.join(root, sub)
            for f in sorted(os.listdir(d)):
                if f.endswith(".asm"):
                    with open(os.path.join(d, f)) as fh:
                        labels, cmds = parse_asm(fh.read(), f"{sub}/{f}")
                    base = len(self.commands)
                    for k, v in labels.items():
                        self.labels[k] = base + v
                    self.commands.extend(cmds)
        self.headers = {}
        hdir = os.path.join(root, "audio/headers")
        for f in sorted(os.listdir(hdir)):
            with open(os.path.join(hdir, f)) as fh:
                cur = None
                for raw in fh:
                    line = raw.split(";", 1)[0].strip()
                    m = re.match(r"^(\w+)::$", line)
                    if m:
                        cur = m.group(1)
                        self.headers[cur] = {}
                        continue
                    m = re.match(r"^channel\s+(\d+),\s*(\w+)$", line)
                    if m and cur:
                        self.headers[cur][int(m.group(1))] = m.group(2)
        self.waves = []
        with open(os.path.join(root, "audio/wave_samples.asm")) as fh:
            for line in fh:
                line = line.split(";", 1)[0].strip()
                if line.startswith("dn "):
                    self.waves.append([num(x) for x in line[3:].split(",")])

    def label(self, name):
        if name not in self.labels:
            raise ConvertError(f"label {name} not found")
        return self.labels[name]


# ---------------------------------------------------------------- engine

class Channel:
    def __init__(self, src, num, start):
        self.src = src
        self.num = num              # 1..4
        self.pc = start
        self.call_ret = None
        self.loop_count = 1
        self.delay = 1              # frames until the next command is read
        self.frac = 0
        self.speed = 1
        self.volume = 0
        self.fade = 0
        self.wave = 0
        self.octave = 4
        self.duty = 0
        self.vib_delay = 0
        self.vib_extent = 0
        self.vib_rate = 0
        self.perfect = False
        self.done = False
        self.events = []            # (frame, event)
        self.label_hits = {}        # command index -> first (event index, frame)
        self.loop_target = None     # (event index, frame) the GOTO goes to
        self.loop_frame = None

    def label_names(self, index):
        return [k for k, v in self.src.labels.items() if v == index]


class Engine:
    def __init__(self, src, header):
        self.src = src
        chans = src.headers.get(header)
        if not chans:
            raise ConvertError(f"no music header {header}")
        self.channels = [Channel(src, n, src.label(lbl)) for n, lbl in sorted(chans.items())]
        self.tempo = 0x100
        self.frame = 0
        self.label_set = set(src.labels.values())

    def note_frames(self, ch, length):
        total = length * ch.speed * self.tempo + ch.frac
        ch.frac = total & 0xFF
        return total >> 8

    def run(self):
        while not all(c.done for c in self.channels):
            for ch in self.channels:
                if ch.done:
                    continue
                ch.delay -= 1
                if ch.delay == 0:
                    self.next_note(ch)
            self.frame += 1
            if self.frame > MAX_FRAMES:
                raise ConvertError("song never loops")

    def next_note(self, ch):
        cmds = self.src.commands
        while True:
            if ch.pc in self.label_set and ch.pc not in ch.label_hits:
                ch.label_hits[ch.pc] = (len(ch.events), self.frame)
            name, args, where = cmds[ch.pc]
            ch.pc += 1
            try:
                if self.command(ch, name, args):
                    return
            except (ValueError, IndexError) as e:
                raise ConvertError(f"{where}: {name} {args}: {e}")
            if ch.done:
                return

    def command(self, ch, name, a):
        """Runs one command; returns True when it took time (a note or rest)."""
        if name == "tempo":
            self.tempo = num(a[0])
        elif name == "volume":
            if (num(a[0]), num(a[1])) != (7, 7):
                warn(f"master volume {a} ignored (m4a keeps NR50 at 7, 7)")
        elif name == "duty_cycle":
            ch.duty = num(a[0])
        elif name == "duty_cycle_pattern":
            ch.duty = num(a[0])
            warn("duty_cycle_pattern: using its first duty only")
        elif name == "note_type":
            ch.speed = num(a[0])
            if ch.num == 3:
                ch.volume = num(a[1]) & 3
                ch.wave = num(a[2]) & 0xF
            else:
                ch.volume = num(a[1])
                ch.fade = num(a[2])
        elif name == "drum_speed":
            ch.speed = num(a[0])
        elif name == "octave":
            ch.octave = num(a[0])
        elif name == "vibrato":
            ch.vib_delay = num(a[0])
            ch.vib_extent = num(a[1])
            ch.vib_rate = num(a[2])
        elif name == "toggle_perfect_pitch":
            ch.perfect = not ch.perfect
        elif name == "stereo_panning":
            warn("stereo_panning ignored")
        elif name == "sound_call":
            if ch.call_ret is not None:
                raise ValueError("nested sound_call")
            ch.call_ret = ch.pc
            ch.pc = self.src.label(a[0])
        elif name == "sound_ret":
            if ch.call_ret is None:
                ch.done = True
                self.end_channel(ch, None)
            else:
                ch.pc = ch.call_ret
                ch.call_ret = None
        elif name == "sound_loop":
            count = num(a[0])
            target = self.src.label(a[1])
            if count == 0:
                if target not in ch.label_hits:
                    raise ValueError("loop target never reached")
                ch.done = True
                self.end_channel(ch, target)
            elif ch.loop_count == count:
                ch.loop_count = 1
            else:
                ch.loop_count += 1
                ch.pc = target
        elif name == "note":
            pitch = NOTE_NAMES.index(a[0])
            frames = self.note_frames(ch, num(a[1]))
            ch.events.append((self.frame, ("note", self.note_state(ch, pitch))))
            ch.delay = frames
            return True
        elif name == "rest":
            ch.delay = self.note_frames(ch, num(a[0]))
            ch.events.append((self.frame, ("rest",)))
            return True
        elif name == "drum_note":
            inst = num(a[0])
            frames = self.note_frames(ch, num(a[1]))
            ch.events.append((self.frame, ("drum", inst)))
            ch.delay = frames
            return True
        else:
            raise ValueError(f"unsupported command {name}")
        return False

    def note_state(self, ch, pitch):
        return dict(key=36 + 12 * (ch.octave - 1) + pitch, octave=ch.octave, pitch=pitch,
                    volume=ch.volume, fade=ch.fade, duty=ch.duty, wave=ch.wave,
                    vib=(ch.vib_delay, ch.vib_extent, ch.vib_rate), perfect=ch.perfect)

    def end_channel(self, ch, target):
        ch.loop_frame = self.frame
        if target is not None:
            ch.loop_target = ch.label_hits[target]


def gb_register(octave, pitch):
    return 2048 + (PITCHES[pitch] >> (octave - 1))


def noise_freq(nr43):
    s, r = nr43 >> 4, nr43 & 7
    return 524288 / (r if r else 0.5) / (2 ** (s + 1))


def noise_key(nr43):
    want = noise_freq(nr43 & ~8)
    best = min(range(len(NOISE_TABLE)),
               key=lambda i: abs(math.log(noise_freq(NOISE_TABLE[i]) / want)))
    if noise_freq(NOISE_TABLE[best]) != want:
        warn(f"noise NR43 {nr43:#04x} has no exact m4a key; using {NOISE_TABLE[best]:#04x}")
    return best + 21


def velocity_for(level):
    """Smallest velocity whose m4a CGB envelope goal is `level` (track VOL 127,
    centre pan): goal = (leftVolume + rightVolume) / 16, see CgbModVol."""
    if level == 0:
        return 0
    vol_mr = (128 * ((127 * 64) >> 5)) >> 8
    vol_ml = (127 * ((127 * 64) >> 5)) >> 8
    for vel in range(1, 128):
        right = min((vel * 128 * vol_mr) >> 14, 255)
        left = min((vel * 127 * vol_ml) >> 14, 255)
        if (left + right) // 16 >= level:
            return vel
    return 127


# ---------------------------------------------------------------- output

class Writer:
    def __init__(self, song, src):
        self.song = song
        self.src = src
        self.voices = []        # voice keys in order
        self.waves_used = []

    def voice(self, key):
        if key not in self.voices:
            self.voices.append(key)
        return self.voices.index(key)

    def voice_line(self, key):
        kind = key[0]
        if kind in ("sq1", "sq2"):
            _, duty, fade = key
            attack, decay, sustain = envelope(fade)
            if kind == "sq1":
                return f"voice_square_1 60, 0, 0, {duty}, {attack}, {decay}, {sustain}, 0"
            return f"voice_square_2 60, 0, {duty}, {attack}, {decay}, {sustain}, 0"
        if kind == "wave":
            _, wave = key
            if wave not in self.waves_used:
                self.waves_used.append(wave)
            return f"voice_programmable_wave 60, 0, {self.song}_wave{wave}, 0, 0, 15, 0"
        _, width, fade = key
        attack, decay, sustain = envelope(fade)
        return f"voice_noise 60, 0, {width}, {attack}, {decay}, {sustain}, 0"


def envelope(fade):
    if fade > 0:
        return 0, fade & 7, 0
    if fade < 0:
        return (-fade) & 7, 0, 15
    return 0, 0, 15


def wait_bytes(n):
    out = []
    while n > 0:
        w = max(x for x in WAIT_LENGTHS if x <= n)
        out.append(f"W{w:02d}")
        n -= w
    return out


def vibrato_params(note):
    delay, extent, rate = note["vib"]
    if extent == 0:
        return 0, 0, 0
    up = extent // 2 + extent % 2
    x = gb_register(note["octave"], note["pitch"]) + (1 if note["perfect"] else 0)
    period = 2048 - x
    if period - up <= 0:
        return 0, 0, 0
    semis = 12 * math.log2(period / (period - up))
    mod = max(1, round(semis * 16))
    lfos = max(1, round(256 / (2 * (rate + 1))))
    return min(mod, 127), min(lfos, 127), min(delay, 127)


def perfect_tune(note):
    """TUNE value that raises the frequency register by 1, as Red's
    toggle_perfect_pitch does (m4a interpolates between keys in 1/256)."""
    k = note["key"] - 36
    o, p = divmod(k, 12)
    v1 = PITCHES[p] >> o
    o2, p2 = divmod(k + 1, 12)
    v2 = PITCHES[p2] >> o2
    step = v2 - v1
    if step <= 0:
        return 0
    fine = math.ceil(256 / step)
    return min(63, math.ceil(fine / 4))


def track_lines(w, ch, tnum):
    """m4a commands for one channel, as (frame, [command strings]) chunks."""
    song = w.song
    lines = [f"{song}_{tnum}:", f"\t.byte\tKEYSH , {song}_key+0"]
    if tnum == 1:
        lines.append(f"\t.byte\tTEMPO , 150*{song}_tbs/2")
    lines += [f"\t.byte\tVOL   , 127*{song}_mvl/mxv", "\t.byte\tPAN   , c_v+0",
              "\t.byte\tMOD   , 0", "\t.byte\tTUNE  , c_v+0"]
    kind = {1: "sq1", 2: "sq2", 3: "wave", 4: "noise"}[ch.num]

    # Expand drums into the instrument's noise steps
    events = []
    for i, (frame, ev) in enumerate(ch.events):
        if ev[0] == "drum":
            # A new drum or a rest cuts the instrument short, as on the Game Boy
            limit = ch.events[i + 1][0] if i + 1 < len(ch.events) else ch.loop_frame
            t = frame
            for (length, vol, fade, nr43) in drum_steps(w.src, ev[1]):
                if limit is not None and t >= limit:
                    break
                events.append((t, ("noise", vol, fade, nr43), i))
                t += length
        else:
            events.append((frame, ev, i))
    events.sort(key=lambda e: e[0])
    loop_index = ch.loop_target[0] if ch.loop_target else None
    end_frame = ch.loop_frame

    state = {"voice": None, "mod": None, "lfos": None, "lfodl": None, "tune": 0}
    cur_frame = events[0][0] if events else 0
    if cur_frame:
        lines += ["\t.byte\t" + w_ for w_ in wait_bytes(cur_frame)]
    playing = None   # key of the TIE in progress
    looped = False
    for n, (frame, ev, src_index) in enumerate(events):
        if frame > cur_frame:
            lines += ["\t.byte\t" + w_ for w_ in wait_bytes(frame - cur_frame)]
            cur_frame = frame
        if playing is not None:
            lines.append(f"\t.byte\tEOT   , {playing}")
            playing = None
        if loop_index is not None and src_index == loop_index and not looped \
                and (n == 0 or events[n - 1][2] != src_index):
            lines.append(f"{song}_{tnum}_LOOP:")
            state = {k: None for k in state}
            looped = True
        if ev[0] == "rest":
            continue
        if ev[0] == "note":
            note = ev[1]
            if kind == "wave":
                vkey = ("wave", note["wave"])
                level = WAVE_LEVEL[note["volume"]]
            else:
                vkey = (kind, note["duty"], note["fade"])
                level = note["volume"]
            key = note["key"]
            mod, lfos, lfodl = vibrato_params(note)
            tune = perfect_tune(note) if note["perfect"] else 0
        else:
            _, vol, fade, nr43 = ev
            vkey = ("noise", (nr43 >> 3) & 1, fade)
            level = vol
            key = noise_key(nr43)
            mod = lfos = lfodl = 0
            tune = 0
        if level == 0:
            continue
        v = w.voice(vkey)
        if state["voice"] != v:
            lines.append(f"\t.byte\t\tVOICE , {v}")
            state["voice"] = v
        if mod and state["lfos"] != lfos:
            lines.append(f"\t.byte\t\tLFOS  , {lfos}")
            state["lfos"] = lfos
        if state["lfodl"] != lfodl and mod:
            lines.append(f"\t.byte\t\tLFODL , {lfodl}")
            state["lfodl"] = lfodl
        if state["mod"] != mod:
            lines.append(f"\t.byte\t\tMOD   , {mod}")
            state["mod"] = mod
        if state["tune"] != tune:
            lines.append(f"\t.byte\t\tTUNE  , c_v+{tune}")
            state["tune"] = tune
        lines.append(f"\t.byte\t\tTIE   , {key}, v{velocity_for(level):03d}")
        playing = key
    if end_frame is not None and end_frame > cur_frame:
        lines += ["\t.byte\t" + w_ for w_ in wait_bytes(end_frame - cur_frame)]
    if playing is not None:
        lines.append(f"\t.byte\tEOT   , {playing}")
    if ch.loop_target:
        lines += ["\t.byte\tGOTO", f"\t .word\t{song}_{tnum}_LOOP"]
    lines.append("\t.byte\tFINE")
    return lines


_drum_cache = {}


def drum_steps(src, inst):
    """Red's noise instrument `inst` (SFX_Noise_InstrumentNN_1): a list of
    (frames, volume, fade, NR43). SFX run at tempo 256, speed 1."""
    if inst in _drum_cache:
        return _drum_cache[inst]
    label = f"SFX_Noise_Instrument{inst:02d}_1_Ch8"
    pc = src.label(label)
    steps = []
    while True:
        name, a, where = src.commands[pc]
        pc += 1
        if name == "noise_note":
            steps.append((num(a[0]) + 1, num(a[1]), num(a[2]), num(a[3])))
        elif name == "sound_ret":
            break
        else:
            raise ConvertError(f"{where}: unsupported {name} in noise instrument")
    _drum_cache[inst] = steps
    return steps


def convert(root, header, song):
    src = Source(root)
    eng = Engine(src, header)
    eng.run()
    w = Writer(song, src)
    tracks = []
    info = []
    for i, ch in enumerate(eng.channels, 1):
        tracks.append(track_lines(w, ch, i))
        if ch.loop_target:
            info.append(f"channel {ch.num}: intro {ch.loop_target[1]} frames, loop "
                        f"{ch.loop_frame - ch.loop_target[1]} frames")
        else:
            info.append(f"channel {ch.num}: ends at frame {ch.loop_frame}")
    loops = {ch.loop_frame - ch.loop_target[1] for ch in eng.channels if ch.loop_target}
    intros = {ch.loop_target[1] for ch in eng.channels if ch.loop_target}
    if loops and (any(max(loops) % n for n in loops) or len(intros) > 1):
        warn("channels loop with different lengths, they will drift: " + "; ".join(info))

    out = [f"@ Generated by tools/gb2agb/pokered2agb.py from pret/pokered {header}.",
           "@ Do not edit; change the converter or the source and run it again.",
           "@ " + "; ".join(info),
           "",
           "\t.include \"MPlayDef.s\"",
           "\t.include \"asm/macros/music_voice.inc\"",
           "",
           f"\t.equ\t{song}_grp, {song}_voicegroup",
           f"\t.equ\t{song}_pri, 0",
           # reverb_set+0 turns m4a's PCM reverb off while the song plays: the
           # Game Boy had none, and with no PCM voices in the song it saves the
           # mixer's reverb pass (~6000 cycles a frame).
           f"\t.equ\t{song}_rev, reverb_set+0",
           f"\t.equ\t{song}_mvl, 127",
           f"\t.equ\t{song}_key, 0",
           f"\t.equ\t{song}_tbs, 1",
           f"\t.equ\t{song}_exg, 0",
           f"\t.equ\t{song}_cmp, 1",
           "",
           "\t.section .rodata",
           f"\t.global\t{song}",
           "\t.align\t2",
           ""]
    for t in tracks:
        out += [f"@**************** Track {tracks.index(t) + 1} ****************@", ""] + t + [""]
    out += ["\t.align\t2", f"{song}_voicegroup:"]
    for key in w.voices:
        out.append("\t" + w.voice_line(key) + f"  @ {key}")
    out.append("")
    for wave in w.waves_used:
        nib = src.waves[wave]
        out += ["\t.align\t2", f"{song}_wave{wave}:",
                "\t.byte\t" + ", ".join(f"0x{(nib[i] << 4) | nib[i + 1]:02X}" for i in range(0, 32, 2))]
    out += ["", "\t.align\t2", f"{song}:", f"\t.byte\t{len(tracks)}\t@ NumTrks", "\t.byte\t0\t@ NumBlks",
            f"\t.byte\t{song}_pri\t@ Priority", f"\t.byte\t{song}_rev\t@ Reverb.", "",
            f"\t.word\t{song}_grp", ""]
    out += [f"\t.word\t{song}_{i}" for i in range(1, len(tracks) + 1)]
    out += ["", "\t.end", ""]
    return "\n".join(out), info


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("pokered", help="pret/pokered checkout")
    ap.add_argument("header", help="music header label, e.g. Music_PalletTown")
    ap.add_argument("song", help="m4a song label, e.g. mus_gb_pallet")
    ap.add_argument("-o", "--output")
    args = ap.parse_args()
    try:
        text, info = convert(args.pokered, args.header, args.song)
    except ConvertError as e:
        sys.exit(f"pokered2agb: {e}")
    path = args.output or os.path.join("sound", "songs", args.song + ".s")
    with open(path, "w") as fh:
        fh.write(text)
    for line in info:
        print(f"{args.song}: {line}")


if __name__ == "__main__":
    main()
