# porymap-mcp — Tool Reference

This MCP server gives Claude structured read/write access to pokeminamoto's map data,
scripts, and palettes, plus the ability to trigger builds — without manually copying
file contents into the chat.

All file paths are resolved inside the project root and validated against path traversal.

---

## Tool Reference

### `list_maps`

Returns a flat array of every map name defined in `data/maps/map_groups.json`.

**Arguments:** none

**Returns:** JSON array of strings, e.g. `["PetalburgCity", "Route101", ...]`

---

### `read_map`

Reads and returns the full contents of `data/maps/{map_name}/map.json` as parsed JSON.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Exact map directory name, e.g. `Route101` |

**Returns:** Parsed map.json object (connections, object events, warp events, weather, music, etc.)

---

### `write_map`

Writes a new `data/maps/{map_name}/map.json`. The incoming JSON is validated before
any write happens — a malformed string will return an error without touching the file.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Exact map directory name |
| `map_json` | string | yes | Full map.json content as a JSON string |

**Returns:** Confirmation string on success, error message on failure.

---

### `read_script`

Reads the Poryscript source at `data/maps/{map_name}/scripts.pory`. Falls back to
`scripts.inc` if the `.pory` file doesn't exist yet (pre-migration maps).

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Exact map directory name |

**Returns:** Raw text of the scripts.pory (or scripts.inc) file.

---

### `write_script`

> **Legacy.** Writes raw pokescript directly to `data/maps/{map_name}/scripts.inc`,
> bypassing the `.pory` source of truth. Use `write_poryscript` instead.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Exact map directory name |
| `content` | string | yes | Full scripts.inc content |

**Returns:** Confirmation string on success.

---

### `list_tilesets`

Lists all tileset directory names under `data/tilesets/primary/` and
`data/tilesets/secondary/`.

**Arguments:** none

**Returns:** Object with two arrays:
```json
{
  "primary": ["building", "general", "secret_base"],
  "secondary": ["fortree", "mauville", ...]
}
```

---

### `read_palette`

Reads a `.pal` file (JASC-PAL format, 16 RGB entries) from a tileset's palette directory.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `tileset_path` | string | yes | Relative path inside `data/tilesets/`, e.g. `primary/general` |
| `palette_id` | string | yes | Palette filename without extension, e.g. `00`, `01` |

**Returns:** Raw text of the `.pal` file.

---

### `write_palette`

Writes a `.pal` file back to `data/tilesets/{tileset_path}/palettes/{palette_id}.pal`.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `tileset_path` | string | yes | e.g. `primary/general` |
| `palette_id` | string | yes | e.g. `00` |
| `content` | string | yes | Full JASC-PAL file content |

**Returns:** Confirmation string on success.

---

### `get_layout`

Looks up the layout entry for a map in `data/layouts/layouts.json`. Matches by layout
name or layout ID (case-insensitive, ignores underscores).

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Map name or partial layout name, e.g. `Route101` |

**Returns:** Matched layout object, e.g.:
```json
{
  "id": "LAYOUT_ROUTE101",
  "name": "Route101_Layout",
  "width": 20,
  "height": 20,
  "primary_tileset": "gTileset_General",
  "secondary_tileset": "gTileset_Route101",
  "border_filepath": "data/layouts/Route101/border.bin",
  "blockdata_filepath": "data/layouts/Route101/map.bin"
}
```

---

### `build`

Runs `make -j4 [target]` in the project root. Output (stdout + stderr combined) is
capped at 8 KB. Hard timeout of 120 seconds.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `target` | string | no | Make target, e.g. `modern`. Omit to build the default ROM. |

**Returns:** Exit code + build output on success; error with output on non-zero exit.

---

### `delete_map`

Fully removes a map from the project. Touches every file the tutorial video describes:

| File / directory | What happens |
|-----------------|--------------|
| `data/maps/{map_name}/` | Entire folder deleted |
| `data/maps/map_groups.json` | Entry removed from its group array |
| `data/maps/groups.inc` | `.4byte {map_name}` line removed |
| `data/maps/events.inc` | `.include` line for this map removed |
| `data/maps/connections.inc` | `.include` line for this map removed |
| `data/maps/headers.inc` | `.include` line for this map removed |
| `data/event_scripts.s` | `.include` line for this map removed |
| Other maps' `map.json` + `events.inc` | Inbound warp destinations rewritten to `redirect_to` (only if provided) |
| `data/layouts/layouts.json` | Layout entry removed **only** if no other map shares it |
| `data/layouts/{layout_dir}/` | Layout folder deleted **only** if layout was exclusive |

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Directory name of the map to delete, e.g. `OldaleTown_Mart` |
| `redirect_to` | string | no | Map constant to redirect inbound warps to, e.g. `MAP_OLDALE_TOWN_POKEMON_CENTER_1F` |

**Returns:** Newline-separated log of every action taken.

> **Warning:** This is irreversible. Commit or stash your changes before running.

---

### `write_poryscript`

Writes Poryscript source (`.pory` syntax) to `data/maps/{map_name}/scripts.pory` **and**
compiles it to `data/maps/{map_name}/scripts.inc`. The `.pory` file is the source of truth;
the `.inc` is the generated build artifact.

Compilation is validated before any files are written — if the source doesn't compile, both
paths are left unchanged and the error is returned. The binary at
`tools/poryscript/poryscript` (v3.6.0) is invoked automatically.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `map_name` | string | yes | Map directory name, e.g. `Route101` |
| `content` | string | yes | Poryscript source (`.pory` syntax) |

**Returns:** Paths of both files written on success; compiler error output on failure.

**Example Poryscript source:**
```
script MyScript {
  lock
  msgbox("Hello, world!")
  release
  end
}
```

**Compiled output (`scripts.inc`):**
```asm
MyScript::
	lock
	msgbox MyScript_Text_0
	release
	end

MyScript_Text_0:
	.string "Hello, world!$"
```

---

### `read_global_script`

Reads a global Poryscript source file from `data/scripts/{script_name}.pory`. Falls back to
`data/scripts/{script_name}.inc` if the `.pory` doesn't exist yet.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `script_name` | string | yes | Filename without extension, e.g. `day_care`, `movement` |

**Returns:** Raw text of the `.pory` (or `.inc`) file.

---

### `write_global_poryscript`

Writes Poryscript source to `data/scripts/{script_name}.pory` and compiles it to
`data/scripts/{script_name}.inc`. Compilation is validated before any files are written.

**Arguments:**

| Name | Type | Required | Description |
|------|------|----------|-------------|
| `script_name` | string | yes | Filename without extension, e.g. `day_care` |
| `content` | string | yes | Poryscript source (`.pory` syntax) |

**Returns:** Paths of both files written on success; compiler error output on failure.

---

## Porymap (GUI map editor)

[Porymap](https://github.com/huderlem/porymap) is the visual map editor for pokeemerald.

**Current status:** Porymap does not ship a Linux binary. The options for WSL2 are:

### Option A — Use the Windows version (recommended)

1. Download `porymap-windows.zip` from the [latest release](https://github.com/huderlem/porymap/releases/latest) and extract it somewhere on Windows.
2. Open Porymap on Windows and point it at your project via the WSL2 path:
   ```
   \\wsl$\Ubuntu\home\primordial\pokeminamoto_v2\pokeminamoto
   ```
   (Replace `Ubuntu` with your WSL distro name if different.)
3. Porymap reads and writes files directly through the WSL2 filesystem. All edits are
   immediately visible in WSL2 — no sync needed.
4. After editing in Porymap, run `make -j$(nproc)` in WSL2 to rebuild the ROM.

### Option B — Build from source in WSL2 (advanced)

Requires Qt 5.15+ and cmake. Neither is currently installed. You would need sudo access to
run `apt install qt5-default cmake` first.

---

## Tutorial

### 1. Explore available maps

Ask Claude to list all maps, then narrow down by name:

> "List all maps, then show me the map.json for LittlerootTown."

Claude will call `list_maps` → `read_map` automatically and display the result.

---

### 2. Write a script with Poryscript

> "Write a script for OldaleTown that greets the player with a message on their first visit,
> using Poryscript syntax."

Claude authors Poryscript source and calls `write_poryscript`. Example source:

```
script OldaleTown_FirstVisit {
  lock
  msgbox("Welcome to Oldale Town!")
  setflag(FLAG_VISITED_OLDALE)
  release
  end
}
```

Poryscript compiles this to valid pokescript `.inc` assembly automatically.

---

### 3. Read and edit an existing script

> "Read the script for Route101, then add a line that gives the player a Potion when
> they step on tile (5, 3)."

Claude will call `read_script` to see the existing pokescript, draft the new content,
then call `write_script` to save it. Follow up with:

> "Now build the ROM and show me any errors."

Claude calls `build` and reports the assembler output.

---

### 3. Change map weather

> "Read the map.json for FortreeCity and change the weather to WEATHER_RAIN."

Claude calls `read_map`, edits the `"weather"` field in memory, then calls `write_map`
with the updated JSON.

---

### 4. Inspect a tileset's palette

> "Show me palette 00 for the general primary tileset."

Claude calls `read_palette` with `tileset_path: "primary/general"` and `palette_id: "00"`.

The JASC-PAL format looks like this:

```
JASC-PAL
0100
16
24 41 82
255 255 255
...
```

Each line after the header is one `R G B` color (0–255). To edit colors:

> "Change color 1 in palette 00 of primary/general from white (255 255 255) to light
> yellow (255 255 200), then write it back."

Claude modifies the text and calls `write_palette`.

---

### 5. Look up a layout

> "What are the dimensions and tilesets for Route110?"

Claude calls `get_layout` with `map_name: "Route110"` and returns the entry from
`layouts.json` — width, height, primary/secondary tileset names, and file paths.

---

### 6. Trigger a build and verify

> "Build the ROM."          → `build` (no target) → `pokeemerald.gba`
> "Build the modern ROM."   → `build` with `target: "modern"` → `pokeemerald_modern.gba`

If the build fails, Claude reads the error output and can suggest or apply fixes directly.

---

### 7. Delete a map and redirect its warp

> "Delete OldaleTown_Mart and redirect its entrance warp to MAP_OLDALE_TOWN_POKEMON_CENTER_1F."

Claude calls `delete_map` with `map_name: "OldaleTown_Mart"` and
`redirect_to: "MAP_OLDALE_TOWN_POKEMON_CENTER_1F"`.

The tool will:
1. Remove the `data/maps/OldaleTown_Mart/` folder
2. Strip its entries from `map_groups.json`, `groups.inc`, `events.inc`, `connections.inc`,
   `headers.inc`, and `event_scripts.s`
3. Scan all other maps for warps pointing to `MAP_OLDALE_TOWN_MART` and rewrite them to
   `MAP_OLDALE_TOWN_POKEMON_CENTER_1F` (both `map.json` and `events.inc`)
4. Leave `LAYOUT_MART` alone because it is shared with other Mart maps

The response is a line-by-line log of every file touched.

**Important notes:**
- Littleroot Town has no Pokemart in pokeemerald — the nearest equivalent is OldaleTown.
- `LAYOUT_MART` (and `LAYOUT_POKEMON_CENTER`) are shared layouts; `delete_map` detects this
  automatically and skips layout deletion.
- Maps that use a unique layout (one not shared with any other map) will have their
  `data/layouts/` folder removed as well.
- After deleting, the Mart warp tile on OldaleTown's overworld map will now send the player
  to the Pokémon Center. Update the `bg_events` sign text in `OldaleTown/events.inc` if you
  want the sign above that door to reflect the change.

---

## Tips

- Map names are **case-sensitive** and match the directory names under `data/maps/`.
  Use `list_maps` if you are unsure of the exact name.
- Tileset paths use **lowercase** directory names as they appear on disk
  (`primary/general`, not `primary/General`).
- `write_map` and `write_script` overwrite immediately — no undo. Commit or stash
  first if you are experimenting.
- The `build` tool runs `make -j4` (4 parallel jobs). Large rebuilds can take 30–60 s.
  For a quick check after changing a single script, a full rebuild is still required
  because the `.inc` files are included via assembly directives.
