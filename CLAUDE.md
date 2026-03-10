# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is **pokeminamoto** — a Gen 1-only fork of the [pokeemerald](https://github.com/pret/pokeemerald) GBA decompilation. The goal is a fully playable Pokémon Emerald ROM containing only the original 151 Pokémon (Bulbasaur through Mew) with Gen 1-equivalent moves. It runs on WSL2 (Ubuntu) and targets the GBA ARM7TDMI architecture.

## Build Commands

### Prerequisites (WSL2/Ubuntu)

```bash
# System packages (already installed)
sudo apt install build-essential binutils-arm-none-eabi git libpng-dev

# agbcc (legacy GCC 2.95 for GBA) — copy from old project or clone+build:
git clone https://github.com/pret/agbcc ../agbcc
cd ../agbcc && ./build.sh && ./install.sh ../pokeminamoto
```

**Note:** `tools/agbcc/` is not committed to git. It was bootstrapped by copying from `/home/primordial/pokeminamoto/tools/agbcc/`. The `arm-none-eabi-*` binutils from apt AND devkitARM (`/opt/devkitpro/devkitARM`) are both present; the Makefile uses whichever is in PATH (apt binutils take precedence if `DEVKITARM` is unset).

### Build Targets

```bash
make -j$(nproc)          # Build pokeemerald.gba (classic agbcc compiler)
make modern -j$(nproc)   # Build pokeemerald_modern.gba (arm-none-eabi-gcc)
make modern DINFO=1 -j$(nproc)  # Modern build with debug symbols (for GDB)
make compare             # Verify ROM matches original sha1 (only useful pre-modification)
make clean               # Full clean (tools + objects + assets)
make tidy                # Remove ROM/ELF/objects only (keeps tools and assets)
make syms                # Generate symbol file from ELF
```

### Single-file rebuild

```bash
# Delete the object to force recompile, then rebuild:
rm build/emerald/src/<filename>.o && make -j$(nproc)
```

### Build outputs

- `pokeemerald.gba` — main ROM (16 MB, GBA format)
- `pokeemerald.elf` — ELF with symbols
- `pokeemerald.map` — linker map

## Debugging

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

Build with `make modern DINFO=1 -j$(nproc)`, then press F5 in VSCode to debug. The ELF used is `pokeemerald_modern.elf`.

## Architecture

### Compilation Pipeline

C sources go through a 4-stage pipeline (classic build):
1. `cpp` — C preprocessor (expands `#include`, `#define`)
2. `preproc` — custom tool expanding Pokémon-specific string escapes (charmap.txt)
3. `agbcc` — GCC 2.95-based cross compiler → ARM assembly
4. `as` (arm-none-eabi) → object files, then `ld` → ELF → `objcopy` → `.gba`

The `modern` build swaps agbcc for `arm-none-eabi-gcc` (GCC 10.x from devkitARM or apt).

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
| `tools/` | Build tools (C programs built from source, except `agbcc`) |

### Key Data Files (Gen 1 Modified)

These files were bulk-modified from the original pokeemerald to replace Gen 2/3 content:

| File | What it contains |
|------|-----------------|
| `src/data/wild_encounters.json` | Wild encounter tables per route/area |
| `src/data/trainer_parties.h` | Every trainer's party definition |
| `src/data/battle_frontier/*.h` | Battle Frontier pools (mons, moves, apprentice data) |
| `src/data/pokemon/level_up_learnsets.h` | Level-up moves per species |
| `src/data/pokemon/tutor_learnsets.h` | Move tutor compatibility bitfields |
| `src/data/pokemon/egg_moves.h` | Egg move tables |
| `src/data/contest_opponents.h` | Contest NPC data |

### Species Constants

`include/constants/species.h` defines all species. Gen 2/3 entries are still present (commented with `TODO: GEN2/3_REMOVED`) for array index compatibility — removing them would break hundreds of array accesses. The Gen 1 build simply never instantiates them in gameplay data.

Helper macro in `species.h`:
```c
#define IS_SPECIES_GEN1(species) ((species) >= SPECIES_BULBASAUR && (species) <= SPECIES_MEW)
#define SPECIES_GEN1_MAX SPECIES_MEW
```

### Move Tutor System

Tutor slots are **index-based** (not name-based). `TUTOR(MOVE_ROLLOUT)` means "bit for tutor slot TUTOR_MOVE_ROLLOUT", not the move Rollout. The actual move taught by each slot is defined in `gTutorMoves[]` in `src/data/pokemon/tutor_learnsets.h`. The slot→move mapping was updated so Gen 2/3 slots now teach Gen 1 equivalents, but the slot identifiers in `sTutorLearnsets[]` bitfields must remain as the original `TUTOR_MOVE_xxx` names.

### Scripting

Map scripts are written in pokescript assembly (`.inc` files in `data/scripts/`). The `preproc` tool handles custom string encoding and script macros. Script constants live in `include/constants/`. Poryscript (higher-level scripting language) is not yet integrated but can be added — see `tools/poryscript/` and https://github.com/huderlem/poryscript.

### Memory Layout

Defined by `ld_script.ld` (classic) / `ld_script_modern.ld` (modern):
- ROM: 32 MB limit (~44% used)
- EWRAM: 256 KB (~95% used — **nearly full**, be careful adding data)
- IWRAM: 32 KB (~94% used)

### Task System

The game uses a cooperative multitasking system (`include/task.h`). Nearly all gameplay logic runs as `Task` callbacks registered with `CreateTask()`. The main loop in `src/main.c` calls `RunTasks()` each frame.

## Gen 1-Only Build Notes

- **Species arrays** (stats, graphics, evolution, learnsets) retain all entries for array-index compatibility; Gen 2/3 data simply goes unreferenced in gameplay.
- **Designated initializer arrays** (like `sValidApprenticeMoves` in `src/data/battle_frontier/apprentice.h`) cannot have duplicate keys under agbcc — if adding move references, check for pre-existing entries.
- **Tutor bitfields** in `sTutorLearnsets[]` use `TUTOR_MOVE_xxx` slot names (defined in `include/constants/party_menu.h`), never Gen 1 move names directly.
- The starters are `SPECIES_BULBASAUR`, `SPECIES_CHARMANDER`, `SPECIES_SQUIRTLE` (set in `src/starter_choose.c`).
- See `GEN1_ONLY_CHANGES.md` for the full replacement mapping reference.
