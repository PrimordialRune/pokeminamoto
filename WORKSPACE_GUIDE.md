# pokeminamoto — Workspace Guide

A quick reference for working on this project in VS Code. Start here if you're new to VS Code.

---

## The Most Important Shortcut

| Shortcut | What it does |
|----------|-------------|
| `Ctrl+Shift+P` | **Command Palette** — search and run *any* VS Code command by name. When in doubt, open this and type what you want. |

---

## Panels & Sidebar

| Shortcut | Panel |
|----------|-------|
| `Ctrl+Shift+E` | **Explorer** — browse files and folders |
| `Ctrl+Shift+F` | **Search** — search text across all files in the project |
| `Ctrl+Shift+G` | **Source Control (Git)** — stage, commit, view diffs |
| `Ctrl+Shift+D` | **Run & Debug** — start/manage debug sessions |
| `Ctrl+Shift+X` | **Extensions** — install and manage extensions |
| `Ctrl+\`` | **Terminal** — open an integrated terminal (WSL bash) |

---

## Tasks (Build, Clean, Launch)

Tasks are defined in `.vscode/tasks.json`. To run one:

- **`Terminal` menu → `Run Task...`**
- or `Ctrl+Shift+P` → type `Tasks: Run Task`

> **Default build task** (`Ctrl+Shift+B`): runs `Build debug ROM` directly.

### Build tasks

| Task | Command | When to use |
|------|---------|-------------|
| `Build debug ROM` | `make modern DINFO=1 O_LEVEL=g` | **Default.** GDB-friendly build — variables visible, breakpoints reliable |
| `Build and ensure firewall` | parallel: build + firewall check | Use before a debug session after a reboot |
| `Build classic ROM (agbcc)` | `make` | Test with the legacy agbcc compiler → `pokeemerald.gba` |
| `Build modern ROM (release)` | `make modern` | `-O2` optimized, no debug symbols → `pokeemerald_modern.gba` |
| `Build modern ROM (keep temps)` | `make modern … KEEP_TEMPS=1` | Keeps intermediate `.i` (preprocessed C) and `.s` (assembly) files in `build/modern/src/` for inspection |
| `Rebuild single file (debug)` | `rm <file>.o && make modern …` | Prompts for a filename stem (e.g. `battle_main`) — deletes that object and rebuilds only it |
| `Rebuild (tidy modern + debug build)` | sequence: tidy → build | Force a clean slate when you suspect stale objects; faster than full clean (keeps tools/assets) |
| `Generate symbols` | `make modern syms` | Produces `pokeemerald_modern.sym` — human-readable symbol table for GDB or objdump |
| `Regenerate scripts and maps` | `make generated` | Re-runs poryscript (`.pory` → `.inc`) and mapjson (JSON → `.s`) after editing map scripts |

### Clean tasks

| Task | What it removes | Keeps |
|------|----------------|-------|
| `Tidy modern build` | `pokeemerald_modern.*` + `build/modern/` | Tools, assets, classic build |
| `Tidy classic build` | `pokeemerald.*` + `build/emerald/` | Tools, assets, modern build |
| `Tidy both builds` | Both sets of ROMs + object dirs | Tools, assets |
| `Clean generated files` | Poryscript/mapjson `.inc` outputs | Everything else |
| `Clean assets` | Graphics/audio binaries (`.4bpp`, `.gbapal`, `.lz`, etc.) | Tools, objects — **slow to regenerate** |
| `Full clean (tools + objects + assets)` | Everything | Nothing — next build recompiles tools too |

### Other tasks

| Task | What it does |
|------|-------------|
| `Ensure WSL firewall rule` | Checks WSL inbound firewall rule; runs automatically on F5 |
| `Compare ROM sha1` | Builds classic ROM and verifies SHA1 — only meaningful on unmodified pokeemerald code |
| `Launch mGBA (play-test, no debug)` | Opens `pokeemerald_modern.gba` in mGBA without GDB — silent, no terminal clutter |

---

## Debugging (GDB + mGBA)

| Shortcut | Action |
|----------|--------|
| `F5` | Start debugging — launches mGBA, attaches GDB, stops at entry |
| `Shift+F5` | Stop debugging |
| `Ctrl+Shift+F5` | Restart the debug session |
| `F9` | Toggle breakpoint on the current line |
| `F10` | Step over (run to next line, don't enter functions) |
| `F11` | Step into (enter the function on this line) |
| `Shift+F11` | Step out (finish current function and return to caller) |
| `F6` | Pause execution |

**How to set a breakpoint:** open a `.c` file, click in the gutter (left of the line number), then press `F5`.

**Debug panel tabs** (visible while debugging):
- **Variables** — live values of local and global variables
- **Watch** — expressions you pin to monitor (e.g. `gPlayerParty[0].species`)
- **Call Stack** — the chain of function calls that got here
- **Breakpoints** — list of all active breakpoints

**First-time debug setup checklist:**
1. Build the debug ROM: `Ctrl+Shift+B`
2. Press `F5` — firewall rule is handled automatically (UAC prompt on first use after reboot)

---

## Debugging Tips — Variable Visibility

### Why variables show `<optimized out>`

`-Og` prevents variable *elimination*, but on ARM parameters are passed in registers `r0`–`r3`. The moment any function is called inside that function, those registers are clobbered and GDB loses the values. `-Og` does **not** force parameters onto the stack — only `-O0` does.

Two common cases that will still show `<optimized out>` with `-Og`:
- **Function parameters** in any function that immediately calls another function (e.g. `CreateMon` calls `ZeroMonData` before you can inspect `species`)
- **Simple loop counters** (`i`, `j`) — always register-only on ARM

### What to watch instead

**Global variables** — always visible, always have a fixed memory address:
```
gBattleMons[0].hp
gBattleMons[0].species
gPlayerParty[0].species
gBattlerAttacker
gCurrentMove
gBattleOutcome
```
Pin these in the **Watch** panel (`+` button) so they update automatically as you step.

**Function parameters** — visible at the start of a function before they get reused:

| File | Function | Good parameters to inspect |
|------|----------|---------------------------|
| `src/pokemon.c` | `CreateMon` | `species`, `level` |
| `src/wild_encounter.c` | `WildEncounterStart` | `area`, `type` |
| `src/battle_setup.c` | `StartWildBattle` | `species`, `level`, `personality` |
| `src/battle_main.c` | `BattleSetup_StartWildBattle` | struct pointer fields |

**Struct fields through a pointer** — dereference works well in Watch:
```
gPlayerParty[0].hp
gBattleMons[gBattlerAttacker].moves[0]
gTrainerBattleOpponent_A
```

### When `-Og` is not enough — use `-O0` on a single function

**Do NOT compile a whole file with `-O0`** — GBA palette loading is timing-sensitive. With `-O0` an entire file runs much slower, shifting DMA palette transfers out of the VBlank window and causing corrupted/wrong Pokémon colors.

Instead, add `__attribute__((optimize("O0")))` to only the one function you want to fully inspect:

```c
// Temporary — remove when done debugging
__attribute__((optimize("O0")))
void CreateMon(struct Pokemon *mon, u16 species, u8 level, ...)
{
    ...
}
```

Then run **`Rebuild single file (debug)`** (`Terminal → Run Task`, enter the filename stem e.g. `pokemon`). Only that function loses optimization; all timing-sensitive code elsewhere is unaffected.

Remove the attribute when you're done.

### Good breakpoint locations to test GDB is working

| File | Line / function | What you can inspect |
|------|----------------|----------------------|
| `src/wild_encounter.c` | `WildEncounterStart` | `area`, encounter type |
| `src/pokemon.c` | `CreateMon` | `species`, `level` params — set here when a wild mon is generated |
| `src/battle_main.c` | `BattleSetup_StartWildBattle` | party/battle struct data |
| `src/battle_script_commands.c` | `Cmd_attackanimation` | move data mid-battle |

### Using the Watch panel effectively

1. Open **Run & Debug** (`Ctrl+Shift+D`) → expand **Watch**
2. Click `+` and type an expression: `gPlayerParty[0].species`
3. The value updates every time execution pauses at a breakpoint
4. You can type any valid C expression: `gBattleMons[0].hp > 50`, `(int)gBattlerAttacker`

---

## Poryscript (.pory files)

The `karathan.poryscript` extension provides IDE support for map scripts.

| Feature | How to use |
|---------|-----------|
| **Syntax highlighting** | Automatic for `.pory` files |
| **Autocomplete** | Type a symbol prefix (e.g. `FLAG_`, `VAR_`, `ITEM_`) and press `Ctrl+Space` |
| **Go to definition** | `F12` or `Ctrl+Click` on a symbol name |
| **Find all references** | `Shift+F12` on a symbol |
| **Hover info** | Hover the mouse over a symbol |
| **Error diagnostics** | Red underlines appear live; see the **Problems** panel (`Ctrl+Shift+M`) |

Symbols available for autocomplete: `FLAG_*`, `VAR_*`, `ITEM_*`, `SPECIES_*`, `MOVE_*`, `MAP_*`, `TRAINER_*`, `OBJ_EVENT_GFX_*`, `ABILITY_*`.

---

## Useful Editor Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+P` | Quick open file by name |
| `Ctrl+G` | Go to line number |
| `Ctrl+/` | Toggle line comment |
| `Alt+Up/Down` | Move current line up/down |
| `Ctrl+D` | Select next occurrence of selected word |
| `Ctrl+Shift+L` | Select all occurrences of selected word |
| `Ctrl+Z` / `Ctrl+Y` | Undo / Redo |
| `Ctrl+\` | Split editor (view two files side by side) |
| `Ctrl+W` | Close current tab |
| `Ctrl+Tab` | Switch between open tabs |
| `Ctrl+Shift+\`` | New terminal |

---

## Git (Source Control)

Use the **Source Control panel** (`Ctrl+Shift+G`) or the integrated terminal.

| Action | How |
|--------|-----|
| See what changed | Source Control panel → lists modified files; click any file to see a diff |
| Stage a file | Click the `+` next to it in the panel |
| Commit | Type a message in the box at the top, press `Ctrl+Enter` |
| View history | `Ctrl+Shift+P` → `Git: View History` (or use `git log` in terminal) |
| Switch branch | Bottom-left corner shows current branch — click it to switch |

Current branch: `copilot/refactor-gen-1-build`

---

## Build Reference (Terminal)

Run these in the integrated terminal (`Ctrl+\``):

```bash
make -j$(nproc)                              # Classic ROM (agbcc) → pokeemerald.gba
make modern -j$(nproc)                       # Modern ROM (release) → pokeemerald_modern.gba
make modern DINFO=1 O_LEVEL=g -j$(nproc)    # Debug ROM for GDB (what Ctrl+Shift+B does)
make tidy                                    # Remove build outputs, keep tools/assets
make tidymodern                              # Remove only modern build outputs
make clean                                   # Full clean including tools and assets
make modern syms                             # Generate pokeemerald_modern.sym symbol table
make generated                               # Regenerate poryscript/mapjson outputs
```

To force-recompile a single file (modern/debug):
```bash
rm build/modern/src/<filename>.o && make modern DINFO=1 O_LEVEL=g -j$(nproc)
```

> **Why `O_LEVEL=g`?** The default is `-O2`, which optimizes variables into registers — GDB shows them as `<optimized out>`. `-Og` is GCC's debug-friendly optimization: no variable elimination, no inlining, but still faster than `-O0`. Always use it for GDB builds.

---

## Recommended Extensions

Defined in `.vscode/extensions.json` — VS Code will prompt you to install these when you open the workspace:

| Extension | Purpose |
|-----------|---------|
| `karathan.poryscript` | Syntax, linting, autocomplete for `.pory` map scripts |
| `ms-vscode.cpptools-extension-pack` | C/C++ IntelliSense, GDB debugger UI |
