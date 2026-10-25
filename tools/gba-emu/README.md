# gba-emu: headless emulator for agents

`gbaemu` is a small host around libmgba (the mGBA core) with no window, no
audio and no real-time clock driving it. The game only advances when a command
says so, so the same commands give the same result every time. The
`gba-debug` MCP server (`tools/gba-debug-mcp`) drives it through its `emu_*`
tools; you rarely need to talk to it directly.

## Install

`tools/setup-cloud.sh` does all of this. By hand (Ubuntu 24.04 or newer, which
ships mGBA 0.10):

```bash
sudo apt install libmgba-dev gdb-multiarch
make -C tools/gba-emu
(cd tools/gba-debug-mcp && npm ci)
```

Older Ubuntu releases ship mGBA 0.9, whose debugger API differs; build mGBA
0.10 from source there or upgrade the distro.

Build the ROM with debug info so gdb has types and line tables:

```bash
make modern DINFO=1 -j$(nproc)
```

## Agent workflow (MCP tools)

| Tool | What it does |
|------|--------------|
| `emu_start` | Load `pokeemerald_modern.gba` + `.elf` (or `rom`, `sav`, `state`, `build: true`) |
| `emu_run` | Run N frames; stops on breakpoint/watchpoint; `until: "Func"` runs to a function; `screenshot: true` |
| `emu_press` / `emu_hold` | Buttons: `A B SELECT START UP DOWN LEFT RIGHT L R`, combine with `+` |
| `emu_screenshot` | Returns the frame as an image, saved under `build/emu/` |
| `emu_break` / `emu_watch` / `emu_delete` / `emu_list` | Breakpoints on `Func`, `0xADDR`, `src/file.c:line`; watchpoints on variables (`w`, `r`, `rw`, `c` = changed) |
| `emu_read` / `emu_write` | Memory by symbol or address, as hex or u8/u16/u32/s8/s16/s32 |
| `emu_regs` / `emu_step` | Registers (pc/lr symbolized, pc source line); single instructions |
| `emu_gdb` | Any gdb command with DWARF types: `print gPlayerParty[0]`, `ptype struct Pokemon`, `bt`, `info locals`, `next`, `finish`, `continue` |
| `emu_savestate` / `emu_loadstate` | Snapshot and restore (same ROM build only) |
| `emu_log` | `DebugPrintf` output (turn off `NDEBUG` in `include/config.h`) and emulator errors |

A typical session:

1. `emu_start` (boot reaches the title screen around frame 3200).
2. `emu_break location="BattleMainCB2"` or `emu_watch location="gBattleMons+0x28"`.
3. `emu_run frames=600` until it reports `"stop":"breakpoint"`.
4. `emu_gdb command="bt"`, `emu_gdb command="print gBattleMons[0]"`, `emu_gdb command="next"`.
5. `emu_savestate file="build/emu/before-bug.ss"` so the next attempt starts there.

Notes:

- With any breakpoint or watchpoint set the core steps one instruction at a
  time, about 300-400 frames per second. With none it runs thousands.
- gdb and the `emu_*` tools share one set of breakpoints. A gdb `continue`
  stops by itself after 3600 frames if nothing hits.
- Savestates and screenshots under `build/emu/` are ignored by git.
- Emerald's intro takes ~3200 frames; load a `.sav` (`emu_start sav=...`,
  read-only) or a savestate to start in the overworld.

## Raw protocol

One command per line on stdin, one JSON object per line on stdout
(`{"ok":true,...}` or `{"ok":false,"error":"..."}`). Send `help` for the list:

```
load <rom> | reset | loadsav <file> | dumpsav <file> | run [frames] | step [n] | regs |
read <addr> <len> | write <addr> <hex> | keys <mask> | break <addr> |
watch <addr> [w|r|rw|c] | delete <id> | list | screenshot <png> |
savestate <file> | loadstate <file> | gdb [port] | gdbframes <n> |
scale <1-4> | log | info | quit
```

Key mask bits: A=1, B=2, SELECT=4, START=8, RIGHT=16, LEFT=32, UP=64,
DOWN=128, R=256, L=512. `gdb <port>` opens mGBA's GDB stub on localhost, for
`gdb-multiarch pokeemerald_modern.elf -ex 'target remote :<port>'`.
