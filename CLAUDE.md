# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **pokeminamoto** — a Gen 1-only fork of the [pokeemerald](https://github.com/pret/pokeemerald) GBA decompilation. The goal is a fully playable Pokémon Emerald ROM containing only the original 151 Pokémon (Bulbasaur through Mew) with Gen 1-equivalent moves. It runs on WSL2 (Ubuntu) and targets the GBA ARM7TDMI architecture.

## Where the code lives (read this first)

- **Canonical checkout:** WSL2 Ubuntu, `/home/primordial/pokeminamoto_v2/pokeminamoto`.
- **Windows view:** the symlink `E:\Omar\Professional\GameDesign\Games\Pokemon\PKMN_Minamoto\pokeminamoto` → `\\wsl.localhost\Ubuntu\home\primordial\pokeminamoto_v2\pokeminamoto`. The parent `PKMN_Minamoto` folder is a separate repo of art (`MyRes/`), `MathModels/` and reference assets; link to it, never move files out of it.
- `~/pokeminamoto` and `~/pokeminamoto_old` in WSL are stale 2023 copies. Never read or modify them.
- Remotes: `origin` = `github.com/PrimordialRune/pokeminamoto`, `upstream` = `pret/pokeemerald`. Branches: `develop` is the default working branch (renamed from `copilot/refactor-gen-1-build`); `main` gets cut from it at milestones; `master` is the old upstream sync, kept as a mirror.

### The WSL rule

**Run every git and make command inside WSL, never with Windows git or make.** Windows git has `core.autocrlf=true` system-wide and writes CRLF into the tree; the repo is set to `core.autocrlf=input`. From a Windows shell (including Claude on Windows):

```bash
wsl.exe -d Ubuntu --cd /home/primordial/pokeminamoto_v2/pokeminamoto -- bash -lc 'make modern -j$(nproc)'
```

From Git Bash, prefix with `MSYS_NO_PATHCONV=1` or the `/home/...` path gets rewritten and `--cd` fails with `ERROR_PATH_NOT_FOUND`. Editing files through the `\\wsl.localhost\...` path is fine; just don't run git/make from Windows. `wsl.exe` starts the distro on demand, so WSL does not need to be open beforehand.

In a cloud session or a native Linux shell, run `make`/`git` directly.

## Build Commands

### Prerequisites

Fresh Debian/Ubuntu box (cloud session, CI, new machine): run `tools/setup-cloud.sh`. It installs `build-essential git libpng-dev binutils-arm-none-eabi gcc-arm-none-eabi libnewlib-arm-none-eabi`, downloads the pinned Poryscript binary (`tools/poryscript/fetch.sh`, sha256-checked; `make` runs it too when the binary is missing) and runs `npm ci` for the MCP servers when node is present. It is idempotent. `LEAN=1` installs only the build toolchain (CI uses it): no emulator, MCP deps, Porytiles or pip.

On the home WSL box everything is already installed: apt `arm-none-eabi` binutils + gcc 10.3, devkitARM at `/opt/devkitpro` (the modern build picks up its gcc 12.2 and `arm-none-eabi-gdb`).

### Build Targets

```bash
make -j$(nproc)          # Main build: pokeemerald_modern.gba (arm-none-eabi-gcc)
make modern -j$(nproc)   # Same as above (kept as an alias)
make DINFO=1 -j$(nproc)  # Modern build with debug symbols (for GDB)
make clean               # Full clean (tools + objects + assets)
make tidy                # Remove ROM/ELF/objects only (keeps tools and assets)
make syms                # Generate symbol file from ELF
make release -j$(nproc)  # Shipping ROM without debug menus: pokeemerald_modern_release.gba
make check               # Data checks (tools/check_data.py) + the in-ROM test runner (TEST=1) in gbaemu
make release REGRESS=1 DINFO=1 -j$(nproc)  # ROM with the hooks tools/regress drives
```

The classic agbcc build and the `MODERN` switch are gone; gcc is the only compiler. Build-time options: `REGRESS=1` (regression hooks, `include/regress.h`), `TEST=1` (in-ROM tests, `src/test/`), `DINFO=1` (debug symbols), `STRICT_WARNINGS` (`-Wundef`, `-Werror=array-bounds`).

### Regression harness

`tools/regress/` drives a `REGRESS=1` ROM in the headless emulator and compares against baselines: `run.py` (main route: boot, new game, battle, save round trip, heap leak and stack checks), `scene_check.py` (each scene's heap/OAM/palette limits), `cable_check.py` (two-GBA link), `save_check.py` (power cuts, partial saves, incompatible saves), `sleep_check.py`, `transition_check.py`, `town_map_check.py`, `compare.py`, `profile.py` (scanlines per frame), `record_av.py`/`record_song.py` (videos). Their offsets into `struct RegressState` are hard-coded: append new fields at the end only. `gbaemu` is an untracked binary; copy `tools/gba-emu/gbaemu` into each new worktree.

### Single-file rebuild

```bash
# Delete the object to force recompile, then rebuild:
rm build/modern/src/<filename>.o && make -j$(nproc)
```

### Build outputs

- `pokeemerald_modern.gba` — main ROM (modern build)
- `pokeemerald_modern.elf` — ELF with symbols
- `pokeemerald_modern.map` — linker map

## Debugging

### Headless emulator for agents (cloud and WSL)

The `gba-debug` MCP server's `emu_*` tools run the ROM in a headless mGBA core (`tools/gba-emu/gbaemu`, built by `tools/setup-cloud.sh` from apt `libmgba-dev` + `gdb-multiarch`). Agents can boot the game, press buttons, take screenshots, set breakpoints/watchpoints on symbols or `file.c:line`, read and write memory, save/load states, and run any gdb command with full types (`emu_gdb "print gPlayerParty[0]"`, `bt`, `next`). Build with `make modern DINFO=1` first. Details and a typical session: `tools/gba-emu/README.md`.

### printf-style via mGBA

1. Comment out `#define NDEBUG` in `include/config.h`
2. Use `DebugPrintf("val=%d", x)` in C code
3. Open mGBA → Tools → View Logs (enable INFO/WARNING/ERROR)
4. Rebuild and run — output appears in the log viewer

```c
// Example usage:
DebugPrintf("species=%d", gPlayerParty[0].species);
DebugPrintf("%S", gSpeciesNames[SPECIES_PIKACHU]);  // prints game strings
```

**Restore before shipping:** re-enable `#define NDEBUG`.

### Source-level GDB via VSCode + mGBA (WSL2)

Requires: mGBA ≥ 0.10.0 on Windows, VSCode with C/C++ Extension Pack, devkitARM gdb at `/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb`.

**One-time setup:**

```bash
# In ~/.bashrc — run once, update mGBA path to your Windows install:
echo "export MGBA_EXECUTABLE=/mnt/c/Users/<user>/Documents/mGBA/mGBA.exe" >> ~/.bashrc
echo "export TODAYS_IP=\$(ipconfig.exe | grep --binary-files=text 'vEthernet (WSL' -A4 | cut -d':' -f 2 | tail -n1 | sed -e 's/\s*//g')" >> ~/.bashrc
source ~/.bashrc
```

**Firewall** (elevated PowerShell — must redo after each PC restart):

```powershell
New-NetFirewallRule -DisplayName "WSL" -Direction Inbound -InterfaceAlias "vEthernet (WSL)" -Action Allow
```

**`.vscode/launch.json`:**

```json
{
    "version": "0.2.0",
    "configurations": [{
        "name": "Attach to mGBA",
        "type": "cppdbg",
        "request": "launch",
        "program": "${workspaceFolder}/pokeemerald_modern.elf",
        "stopAtEntry": true,
        "cwd": "${workspaceFolder}",
        "miDebuggerServerAddress": "${env:TODAYS_IP}:2345",
        "debugServerArgs": "${workspaceFolder}/pokeemerald_modern.elf",
        "serverStarted": "started-mgba-server",
        "linux": {
            "MIMode": "gdb",
            "miDebuggerPath": "/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb",
            "debugServerPath": "${workspaceFolder}/.vscode/mgba-gdb-wrapper.sh"
        }
    }]
}
```

**`.vscode/mgba-gdb-wrapper.sh`** (make executable with `chmod +x`):

```bash
#!/bin/bash
( sleep 4; echo "started-mgba-server" )&
"$MGBA_EXECUTABLE" $1 -g
```

Build with `make DINFO=1 -j$(nproc)`, then press F5 in VSCode to debug. The ELF used is `pokeemerald_modern.elf`.

## Architecture

### Compilation Pipeline

C sources go through a 4-stage pipeline:
1. `cpp` — C preprocessor (expands `#include`, `#define`)
2. `preproc` — custom tool expanding Pokémon-specific string escapes (charmap.txt)
3. `arm-none-eabi-gcc` → ARM assembly
4. `as` (arm-none-eabi) → object files, then `ld` → ELF → `objcopy` → `.gba`

The modern build is the main one since 2026-10-23 and adds `-Wall` plus `-Werror` for return-type, implicit declarations, int/pointer conversions and parentheses.

### Directory Layout

| Path | Purpose |
|------|---------|
| `src/` | C source files — game logic |
| `src/data/` | C data headers included by src files (species info, learnsets, trainer parties, etc.) |
| `include/` | Public headers |
| `include/constants/` | Numeric constants (`species.h`, `moves.h`, `items.h`, etc.) |
| `asm/` | Hand-written ARM assembly (decompiled but not yet C-ified) |
| `data/` | Script/map assembly (`.s` files), map data (JSON → generated `.s`) |
| `data/scripts/` | Map event scripts (`.inc` files, assembled by `preproc` + `as`) |
| `data/maps/` | Per-map event/header data (JSON-based, generated via `mapjson`) |
| `graphics/` | PNG source graphics (converted to GBA formats at build time) |
| `sound/` | Music and sound effects |
| `tools/` | Build tools (C programs built from source) |

### Key Data Files

| File | What it contains |
|------|-----------------|
| `src/data/species/<name>.json`, `species.txt` | One record per species (stats, learnsets, evolutions, dex, graphics). `tools/speciesgen` generates `include/constants/species.h` and the `src/data/pokemon/` tables on every build; edit the records, never the generated headers. See `tools/speciesgen/README.md`. |
| `src/data/moves_info.h` | `gMovesInfo[]`, the one move table (power, type, TP cost, effect, names, descriptions, contest data). PP is gone: TP only. |
| `src/data/trainers.party` | Every trainer and party, compiled by `tools/trainerproc`. Removed trainers keep their ids through `include/constants/removed_trainers.h`. |
| `src/data/wild_encounters.json` | Wild encounter tables per route/area |

`NUM_SPECIES` is 152 (none + the 151); removed Gen 2/3 names resolve through `include/constants/removed_species.h`. `BoxPokemon` is 80 B and `Pokemon` 100 B, held by static asserts: new per-mon data goes in the freed PP/contest bytes (`docs/engineering/per-mon-data.md`).

### Move Tutor System

Tutor slots are **index-based** (not name-based). `TUTOR(MOVE_ROLLOUT)` means "bit for tutor slot TUTOR_MOVE_ROLLOUT", not the move Rollout. The actual move taught by each slot is defined in `gTutorMoves[]` in `src/data/pokemon/tutor_learnsets.h`. The slot→move mapping was updated so Gen 2/3 slots now teach Gen 1 equivalents, but the slot identifiers in `sTutorLearnsets[]` bitfields must remain as the original `TUTOR_MOVE_xxx` names.

### Tilesets

New Kanto tilesets are built with Porytiles 2.0.0 (installed by `tools/setup-cloud.sh` into `tools/porytiles/bin/`). `tools/porytiles/tileset.sh` turns a map mockup into a primary + secondary pair and packs palettes optimally; see `tools/porytiles/README.md`. Don't hand-edit `porytiles_bin/` or `porytiles/tilesets/*/config.yaml`: they are generated.

### Scripting

Map and shared scripts are written in [Poryscript](https://github.com/huderlem/poryscript): edit `data/maps/*/scripts.pory` and `data/scripts/*.pory`. The generated `scripts.inc` and `data/scripts/*.inc` files are gitignored and rebuilt by `make` (or `make generated`) with the committed Linux binary `tools/poryscript/poryscript`. Dialogue goes in a separate `text.en.pory` per map (translations in `text.<lang>.pory`, one language per ROM via `make LANGUAGE=ITALIAN`): see `docs/engineering/map-text.md`. The `preproc` tool handles custom string encoding and script macros. Script constants live in `include/constants/`.

### Memory Layout

Defined by `ld_script_modern.ld`. Measured 2026-10-25 (release build):
- ROM: 32 MB limit (about 8.6 MB used)
- EWRAM: 256 KB (about 76% used). Still flag anything that adds RAM or save data: the save sectors and the battle heap are the tighter limits now.
- IWRAM: 32 KB (about 72% used). The linker asserts a 6 KB stack reserve, and hot ARM code goes in the `IWRAM_CODE` section (4 KB cap).

Saves are copy-on-write (`SAVE_COPY_ON_WRITE` in `include/config.h`): each sector has two homes and a commit record, so a power cut keeps the last good save. The footer stores a format version and a layout hash; changing any saved struct changes the hash and makes old saves load as incompatible. The frame loop halts in `VBlankIntrWait`, and debug/regress builds show a red screen on heap failure.

### Task System

The game uses a cooperative multitasking system (`include/task.h`). Nearly all gameplay logic runs as `Task` callbacks registered with `CreateTask()`. The main loop in `src/main.c` calls `RunTasks()` each frame.

## Gen 1-Only Build Notes

- **Tilesets:** only the tilesets the remaining layouts use are built in, plus `SecretBase`/`SecretBaseRedCave` (bedroom decorations), `Cave` and `MirageTower` (named in code). The Hoenn tilesets' animation code in `src/tileset_anims.c` is still there (about 50 KB). `FREE_HOENN_SET_PIECES` switches off the Rayquaza cutscene, cable car and Mirage Tower.
- **Designated initializer arrays** (like `sValidApprenticeMoves` in `src/data/battle_frontier/apprentice.h`) cannot have duplicate keys (gcc warns with `-Woverride-init`) — if adding move references, check for pre-existing entries.
- **Tutor bitfields** in `sTutorLearnsets[]` use `TUTOR_MOVE_xxx` slot names (defined in `include/constants/party_menu.h`), never Gen 1 move names directly.
- The starters are `SPECIES_BULBASAUR`, `SPECIES_CHARMANDER`, `SPECIES_SQUIRTLE` (set in `src/starter_choose.c`).
- See `GEN1_ONLY_CHANGES.md` for the full replacement mapping reference.
- **Save-block cleanup:** the `FREE_*` defines in `include/config.h` drop save data for unreachable Hoenn/e-Reader systems (Mystery Event/Gift, Enigma Berry, e-Reader trainer, Frontier hall records, Union Room chat texts, extra dex copies, Apprentice, GameCube link, editable easy chat phrases, Walda, Dewford trends, TV/PokeNews/outbreaks/Gabby & Ty, Mauville old man, Lilycove lady, battle recorder, Trainer Hill, Pokéblocks and Blender records, contest winners, gift ribbons). `FREE_WIRELESS` removes the Wireless Adapter stack (librfu, `link_rfu_*`: Union Room and the wireless minigames can't start; cable link works, its functions are stubs) and the saved names of wirelessly linked trainers; `FREE_FOUR_PLAYER_LINK` sizes the cable receive queue for 2 players, and the Cable Club refuses to start a link with more. Reads go through macros such as `ENIGMA_BERRY`, `EREADER_TRAINER`, `DEWFORD_TRENDS`, `SAVED_APPRENTICES`, `CONTEST_WINNERS`, `GIFT_RIBBONS`, `SAVED_POKEBLOCK()` (empty ROM copies when freed); whole systems that are gone (TV, old man, Lilycove lady, battle recorder, Trainer Hill, Pokéblock case) keep their specials as stubs at the end of their file, and record mixing sends empty data for them. Changing any of them changes the save layout.
- **Debug menus:** every modern build except `make release` has them (`DEBUG_MENUS` in `include/config.h`). Overworld: hold R and press START (`src/debug_menu.c`: heal, warp, give mons/items, flags/vars, battles, sound, no-clip/encounter/trainer-sight toggles, `Debug_Script_1-4` in `data/scripts/debug.pory`). Battle: hold L and press SELECT on the action menu (`src/battle_debug.c`: HP, TP, status, stat stages). Keep debug code inside `#ifdef DEBUG_MENUS`; release builds must add no RAM.
- **Hardware features:** L+R+SELECT in the overworld puts the GBA to sleep (BIOS Stop, `src/sleep_mode.c`), any key wakes it. Options has an LCD profile (GBA/SP/IPS, default IPS, `include/palette_tone.h`). Day/night is in `src/day_night.c` (`VAR_TIME_OF_DAY_OVERRIDE` forces a phase).

## Working remotely

- **Remote Control (home PC):** sessions started in the Claude desktop app on the PC connect to Remote Control automatically, so they can be driven from the phone or claude.ai/code. The PC must be on and not asleep (the app keeps it awake only while Claude is working). Claude on Windows reaches the repo through the symlink and runs git/make via `wsl.exe` (the WSL rule above). mGBA, GDB and the `gba-debug` GDB tools only work here.
- **Cloud sessions (claude.ai/code, from GitHub):** the repo is cloned fresh into an Ubuntu container. Set the environment's setup script to `tools/setup-cloud.sh`. `make` (apt gcc 13) builds there. The headless emulator (`emu_*` tools) runs there too, so agents can boot the ROM, debug and screenshot it; play-feel checks still happen on the PC. Commit and push before the session ends; the container is discarded.
- **MCP servers** (`.mcp.json`): `porymap` (maps, scripts, palettes, builds) and `gba-debug` (headless emulator `emu_*` tools, disassembly, symbols, addr2line, map lookup; its `gdb_*` tools attach to an external mGBA, e.g. the desktop one on the PC). Both run with `node`; `tools/setup-cloud.sh` installs their deps.

## Agents

Project subagents live in `.claude/agents/`. Keep their briefs small and name the files.

| Agent | Model | Use for |
|-------|-------|---------|
| `pokeminamoto-mechanic` | Haiku | builds, `make generated`, tidy, line endings, grep audits |
| `pokeminamoto-engineer` | Sonnet | one scoped code or data change, a bug fix, a read-only audit |
| `pokeminamoto-lead` | Opus | engine or design-judgement work (TP, move ranks, AI states, RAM/save budget), multi-step plans |

Default to Sonnet; use the lead only when the task can't be specified up front.

## Design docs

Game design (story, region, roster, mechanics, art direction, open questions) is summarised in `docs/design/` from the GDD, story doc, arena ecology and city framework. The originals stay the source of truth; see `docs/design/README.md` for where they live.

## Where things stand (2026-10-25)

- Gen-1 data on `develop`; only the opening maps (truck, Littleroot, Route 101, Oldale, Route 103) and the link rooms remain. Old map ids still named by dead code go through `include/constants/parked_maps.h`. See `GEN1_ONLY_CHANGES.md`.
- Hoenn-only systems (Frontier, contests, berries, Secret Bases, TV, e-Reader, wireless and more) are being deleted behind the `FREE_*` switches.
- TP replaces PP, with the Minamoto battle UI. TP persists between battles and only refills at an inn (`RestPlayerParty`); Pokémon Centers are out of the design. Inns, Charisma tiers, clock-windowed NPCs: `docs/engineering/town-life-toolkit.md`. R = Talk (Provoke/Unnerve) sets the first trainer states (Resolute, Panic; `src/battle_talk.c`); SELECT opens the foe info screen (`src/battle_foe_info.c`). Move ranks, signature moves and the other trainer states are not implemented yet.
- `make check` and the `tools/regress` scripts are the regression gate; run them before merging engine changes.
