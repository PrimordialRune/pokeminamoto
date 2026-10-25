#!/usr/bin/env python3
"""Records a song's audio from the field (REGRESS ROM).

    python3 tools/regress/record_song.py ROM STATE SONG SECONDS OUT.raw

Loads STATE (a field savestate of the same ROM, e.g. from record_av.py
--save-state), plays SONG (number, see include/constants/songs.h) through
gRegress (REGRESS_CMD_MUSIC) and writes SECONDS of raw s16le stereo 32768 Hz
audio to OUT.raw.
"""
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as R  # noqa: E402

CMD_MUSIC = 10
SONG_OFFSET = 40 + 2 * 1024 + 6   # gRegress.song


def main():
    rom, state, song, secs, out = sys.argv[1:6]
    emu = R.Emu(rom, rom.replace(".gba", ".elf"), tempfile.mkdtemp(), False)
    emu.cmd(f"loadstate {os.path.abspath(state)}")
    song = int(song, 0)
    emu.cmd(f"write {emu.rs + SONG_OFFSET:#x} {song & 0xFF:02x}{song >> 8:02x}")
    emu.command(CMD_MUSIC)
    emu.cmd(f"audio {os.path.abspath(out)}")
    emu.run(int(float(secs) * 59.7275))
    print(emu.cmd("audio off"))


if __name__ == "__main__":
    main()
