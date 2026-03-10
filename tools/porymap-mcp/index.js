import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";
import fs from "fs";
import os from "os";
import path from "path";
import { spawn, spawnSync } from "child_process";

const PORYSCRIPT_BIN = path.resolve(import.meta.dirname, "../../tools/poryscript/poryscript");
const PORYSCRIPT_DIR = path.dirname(PORYSCRIPT_BIN);

const PROJECT_ROOT = path.resolve(import.meta.dirname, "../../");

function safePath(relative) {
  const resolved = path.resolve(PROJECT_ROOT, relative);
  if (!resolved.startsWith(PROJECT_ROOT + path.sep) && resolved !== PROJECT_ROOT) {
    throw new Error(`Path traversal detected: ${relative}`);
  }
  return resolved;
}

function ok(content) {
  return { content: [{ type: "text", text: typeof content === "string" ? content : JSON.stringify(content, null, 2) }] };
}

function err(msg) {
  return { isError: true, content: [{ type: "text", text: String(msg) }] };
}

const server = new Server(
  { name: "porymap-mcp", version: "1.0.0" },
  { capabilities: { tools: {} } }
);

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: [
    {
      name: "list_maps",
      description: "List all map names from data/maps/map_groups.json",
      inputSchema: { type: "object", properties: {} },
    },
    {
      name: "read_map",
      description: "Read data/maps/{map_name}/map.json",
      inputSchema: {
        type: "object",
        properties: { map_name: { type: "string" } },
        required: ["map_name"],
      },
    },
    {
      name: "write_map",
      description: "Write data/maps/{map_name}/map.json (validates JSON before writing)",
      inputSchema: {
        type: "object",
        properties: {
          map_name: { type: "string" },
          map_json: { type: "string", description: "JSON string to write" },
        },
        required: ["map_name", "map_json"],
      },
    },
    {
      name: "read_script",
      description: "Read data/maps/{map_name}/scripts.pory (Poryscript source). Falls back to scripts.inc if the .pory doesn't exist yet.",
      inputSchema: {
        type: "object",
        properties: { map_name: { type: "string" } },
        required: ["map_name"],
      },
    },
    {
      name: "write_script",
      description: "Write raw pokescript directly to data/maps/{map_name}/scripts.inc, bypassing the .pory source. Legacy — prefer write_poryscript instead.",
      inputSchema: {
        type: "object",
        properties: {
          map_name: { type: "string" },
          content: { type: "string" },
        },
        required: ["map_name", "content"],
      },
    },
    {
      name: "list_tilesets",
      description: "List all tileset names under data/tilesets/primary/ and secondary/",
      inputSchema: { type: "object", properties: {} },
    },
    {
      name: "read_palette",
      description: "Read data/tilesets/{tileset_path}/palettes/{palette_id}.pal",
      inputSchema: {
        type: "object",
        properties: {
          tileset_path: { type: "string", description: "e.g. primary/General" },
          palette_id: { type: "string", description: "e.g. 00" },
        },
        required: ["tileset_path", "palette_id"],
      },
    },
    {
      name: "write_palette",
      description: "Write data/tilesets/{tileset_path}/palettes/{palette_id}.pal",
      inputSchema: {
        type: "object",
        properties: {
          tileset_path: { type: "string" },
          palette_id: { type: "string" },
          content: { type: "string" },
        },
        required: ["tileset_path", "palette_id", "content"],
      },
    },
    {
      name: "get_layout",
      description: "Get the layout entry from data/layouts/layouts.json for a given map name",
      inputSchema: {
        type: "object",
        properties: { map_name: { type: "string" } },
        required: ["map_name"],
      },
    },
    {
      name: "build",
      description: "Run make in project root. Returns stdout+stderr (capped 8KB).",
      inputSchema: {
        type: "object",
        properties: {
          target: { type: "string", description: "Make target, e.g. 'modern'. Omit for default." },
        },
      },
    },
    {
      name: "delete_map",
      description: [
        "Fully delete a map from the project. Removes the map folder under data/maps/, its",
        "entries in map_groups.json, groups.inc, events.inc, connections.inc, headers.inc,",
        "and event_scripts.s. If the layout is exclusive to this map it is also removed from",
        "layouts.json and data/layouts/. Optionally redirects all inbound warps (in other",
        "maps' map.json + events.inc) to a replacement map constant.",
      ].join(" "),
      inputSchema: {
        type: "object",
        properties: {
          map_name: { type: "string", description: "Directory name of the map, e.g. OldaleTown_Mart" },
          redirect_to: {
            type: "string",
            description: "Optional map constant to redirect inbound warps to, e.g. MAP_OLDALE_TOWN_POKEMON_CENTER_1F",
          },
        },
        required: ["map_name"],
      },
    },
    {
      name: "write_poryscript",
      description: [
        "Write Poryscript source (.pory) to data/maps/{map_name}/scripts.pory and compile it",
        "to data/maps/{map_name}/scripts.inc. Both files are written; the .pory is the source",
        "of truth. Use this instead of write_script when authoring new scripts.",
      ].join(" "),
      inputSchema: {
        type: "object",
        properties: {
          map_name: { type: "string", description: "Map directory name, e.g. Route101" },
          content: { type: "string", description: "Poryscript source (.pory syntax)" },
        },
        required: ["map_name", "content"],
      },
    },
    {
      name: "read_global_script",
      description: "Read a global Poryscript source file from data/scripts/{script_name}.pory, or fall back to .inc if .pory doesn't exist yet.",
      inputSchema: {
        type: "object",
        properties: {
          script_name: { type: "string", description: "Filename without extension, e.g. day_care, movement" },
        },
        required: ["script_name"],
      },
    },
    {
      name: "write_global_poryscript",
      description: "Write Poryscript source to data/scripts/{script_name}.pory and compile it to data/scripts/{script_name}.inc.",
      inputSchema: {
        type: "object",
        properties: {
          script_name: { type: "string", description: "Filename without extension, e.g. day_care" },
          content: { type: "string", description: "Poryscript source (.pory syntax)" },
        },
        required: ["script_name", "content"],
      },
    },
  ],
}));

server.setRequestHandler(CallToolRequestSchema, async (request) => {
  const { name, arguments: args } = request.params;

  try {
    switch (name) {
      case "list_maps": {
        const mapGroupsPath = safePath("data/maps/map_groups.json");
        const data = JSON.parse(fs.readFileSync(mapGroupsPath, "utf8"));
        const maps = [];
        for (const group of data.group_order) {
          const entries = data[group];
          if (Array.isArray(entries)) maps.push(...entries);
        }
        return ok(maps);
      }

      case "read_map": {
        const p = safePath(`data/maps/${args.map_name}/map.json`);
        return ok(JSON.parse(fs.readFileSync(p, "utf8")));
      }

      case "write_map": {
        let parsed;
        try {
          parsed = JSON.parse(args.map_json);
        } catch (e) {
          return err(`Invalid JSON: ${e.message}`);
        }
        const p = safePath(`data/maps/${args.map_name}/map.json`);
        fs.writeFileSync(p, JSON.stringify(parsed, null, 2) + "\n", "utf8");
        return ok(`Written: data/maps/${args.map_name}/map.json`);
      }

      case "read_script": {
        const pory = safePath(`data/maps/${args.map_name}/scripts.pory`);
        const inc = safePath(`data/maps/${args.map_name}/scripts.inc`);
        if (fs.existsSync(pory)) return ok(fs.readFileSync(pory, "utf8"));
        return ok(fs.readFileSync(inc, "utf8"));
      }

      case "write_script": {
        const p = safePath(`data/maps/${args.map_name}/scripts.inc`);
        fs.writeFileSync(p, args.content, "utf8");
        return ok(`Written: data/maps/${args.map_name}/scripts.inc`);
      }

      case "list_tilesets": {
        const result = { primary: [], secondary: [] };
        for (const type of ["primary", "secondary"]) {
          const dir = safePath(`data/tilesets/${type}`);
          if (fs.existsSync(dir)) {
            result[type] = fs.readdirSync(dir).filter((f) =>
              fs.statSync(path.join(dir, f)).isDirectory()
            );
          }
        }
        return ok(result);
      }

      case "read_palette": {
        const p = safePath(`data/tilesets/${args.tileset_path}/palettes/${args.palette_id}.pal`);
        return ok(fs.readFileSync(p, "utf8"));
      }

      case "write_palette": {
        const p = safePath(`data/tilesets/${args.tileset_path}/palettes/${args.palette_id}.pal`);
        fs.writeFileSync(p, args.content, "utf8");
        return ok(`Written: data/tilesets/${args.tileset_path}/palettes/${args.palette_id}.pal`);
      }

      case "get_layout": {
        const layoutsPath = safePath("data/layouts/layouts.json");
        const data = JSON.parse(fs.readFileSync(layoutsPath, "utf8"));
        const needle = args.map_name.replace(/_/g, "").toLowerCase();
        const match = data.layouts.find((l) =>
          l.name.replace(/_/g, "").toLowerCase().includes(needle) ||
          (l.id || "").replace(/_/g, "").toLowerCase().includes(needle)
        );
        if (!match) return err(`No layout found for map: ${args.map_name}`);
        return ok(match);
      }

      case "build": {
        const target = args?.target?.trim() ?? "";
        const makeArgs = ["-j4"];
        if (target) makeArgs.push(target);

        return await new Promise((resolve) => {
          let output = "";
          const MAX = 8 * 1024;
          const proc = spawn("make", makeArgs, {
            cwd: PROJECT_ROOT,
            env: { ...process.env },
          });

          const append = (chunk) => {
            if (output.length < MAX) output += chunk;
          };

          proc.stdout.on("data", (d) => append(d.toString()));
          proc.stderr.on("data", (d) => append(d.toString()));

          const timer = setTimeout(() => {
            proc.kill("SIGTERM");
            resolve(err("Build timed out after 120s"));
          }, 120_000);

          proc.on("close", (code) => {
            clearTimeout(timer);
            const trimmed = output.length >= MAX ? output.slice(0, MAX) + "\n...[truncated]" : output;
            const result = `exit code: ${code}\n\n${trimmed}`;
            resolve(code === 0 ? ok(result) : err(result));
          });
        });
      }

      case "delete_map": {
        const { map_name, redirect_to } = args;
        const report = [];

        // Validate map exists
        const mapJsonPath = safePath(`data/maps/${map_name}/map.json`);
        if (!fs.existsSync(mapJsonPath)) {
          return err(`Map not found: data/maps/${map_name}/map.json`);
        }
        const mapData = JSON.parse(fs.readFileSync(mapJsonPath, "utf8"));
        const mapConstant = mapData.id;   // e.g. "MAP_OLDALE_TOWN_MART"
        const layoutId = mapData.layout;  // e.g. "LAYOUT_MART"

        // Helper: remove all lines containing a substring from a file
        const removeLine = (relPath, substring) => {
          const p = safePath(relPath);
          if (!fs.existsSync(p)) return false;
          const original = fs.readFileSync(p, "utf8");
          const filtered = original.split("\n").filter((l) => !l.includes(substring)).join("\n");
          if (filtered !== original) {
            fs.writeFileSync(p, filtered, "utf8");
            return true;
          }
          return false;
        };

        // 1. Remove from map_groups.json
        const mapGroupsPath = safePath("data/maps/map_groups.json");
        const mapGroups = JSON.parse(fs.readFileSync(mapGroupsPath, "utf8"));
        let foundGroup = null;
        for (const group of mapGroups.group_order) {
          const arr = mapGroups[group];
          if (Array.isArray(arr)) {
            const idx = arr.indexOf(map_name);
            if (idx !== -1) {
              arr.splice(idx, 1);
              foundGroup = group;
              break;
            }
          }
        }
        if (foundGroup) {
          fs.writeFileSync(mapGroupsPath, JSON.stringify(mapGroups, null, 2) + "\n", "utf8");
          report.push(`Removed from map_groups.json (${foundGroup})`);
        } else {
          report.push(`WARNING: ${map_name} not found in map_groups.json`);
        }

        // 2. Remove from generated .inc aggregators
        if (removeLine("data/maps/groups.inc", `\t.4byte ${map_name}`))
          report.push("Removed from data/maps/groups.inc");

        if (removeLine("data/maps/events.inc", `data/maps/${map_name}/events.inc`))
          report.push("Removed from data/maps/events.inc");

        if (removeLine("data/maps/connections.inc", `data/maps/${map_name}/connections.inc`))
          report.push("Removed from data/maps/connections.inc");

        if (removeLine("data/maps/headers.inc", `data/maps/${map_name}/header.inc`))
          report.push("Removed from data/maps/headers.inc");

        if (removeLine("data/event_scripts.s", `data/maps/${map_name}/scripts.inc`))
          report.push("Removed from data/event_scripts.s");

        // 3. Redirect inbound warps in all other maps
        if (redirect_to) {
          const mapsDir = safePath("data/maps");
          const siblings = fs.readdirSync(mapsDir).filter((d) => {
            return d !== map_name && fs.existsSync(path.join(mapsDir, d, "map.json"));
          });

          for (const sibling of siblings) {
            const sibMapJsonPath = path.join(mapsDir, sibling, "map.json");
            const sibData = JSON.parse(fs.readFileSync(sibMapJsonPath, "utf8"));
            let changed = false;

            if (Array.isArray(sibData.warp_events)) {
              for (const warp of sibData.warp_events) {
                if (warp.dest_map === mapConstant) {
                  warp.dest_map = redirect_to;
                  changed = true;
                }
              }
            }

            if (changed) {
              fs.writeFileSync(sibMapJsonPath, JSON.stringify(sibData, null, 2) + "\n", "utf8");
              report.push(`Redirected warp in data/maps/${sibling}/map.json → ${redirect_to}`);

              // Also patch the auto-generated events.inc so the build stays consistent
              const sibEventsPath = path.join(mapsDir, sibling, "events.inc");
              if (fs.existsSync(sibEventsPath)) {
                const original = fs.readFileSync(sibEventsPath, "utf8");
                const patched = original.replaceAll(mapConstant, redirect_to);
                if (patched !== original) {
                  fs.writeFileSync(sibEventsPath, patched, "utf8");
                  report.push(`Redirected warp in data/maps/${sibling}/events.inc`);
                }
              }
            }
          }
        }

        // 4. Handle layout — only delete if exclusive to this map
        const layoutsPath = safePath("data/layouts/layouts.json");
        const layoutsData = JSON.parse(fs.readFileSync(layoutsPath, "utf8"));
        const mapsDir2 = safePath("data/maps");
        const sharedUsers = fs
          .readdirSync(mapsDir2)
          .filter((d) => d !== map_name && fs.existsSync(path.join(mapsDir2, d, "map.json")))
          .filter((d) => {
            const mj = JSON.parse(fs.readFileSync(path.join(mapsDir2, d, "map.json"), "utf8"));
            return mj.layout === layoutId;
          });

        if (sharedUsers.length === 0) {
          const layoutIdx = layoutsData.layouts.findIndex((l) => l.id === layoutId);
          if (layoutIdx !== -1) {
            const layoutEntry = layoutsData.layouts[layoutIdx];
            layoutsData.layouts.splice(layoutIdx, 1);
            fs.writeFileSync(layoutsPath, JSON.stringify(layoutsData, null, 2) + "\n", "utf8");
            report.push(`Removed layout ${layoutId} from data/layouts/layouts.json`);

            // Delete layout directory (named after the map, not the layout id)
            const layoutDirName = (layoutEntry.name ?? "").replace(/_Layout$/, "") || map_name;
            const layoutDir = safePath(`data/layouts/${layoutDirName}`);
            if (fs.existsSync(layoutDir)) {
              fs.rmSync(layoutDir, { recursive: true });
              report.push(`Deleted data/layouts/${layoutDirName}/`);
            }
          }
        } else {
          report.push(`Layout ${layoutId} is shared with ${sharedUsers.length} other map(s) — not deleted`);
        }

        // 5. Delete the map directory last
        fs.rmSync(safePath(`data/maps/${map_name}`), { recursive: true });
        report.push(`Deleted data/maps/${map_name}/`);

        return ok(report.join("\n"));
      }

      case "write_poryscript": {
        if (!fs.existsSync(PORYSCRIPT_BIN)) {
          return err(`poryscript binary not found at ${PORYSCRIPT_BIN}`);
        }

        const poryDest = safePath(`data/maps/${args.map_name}/scripts.pory`);
        const tmpOut = path.join(os.tmpdir(), `poryscript_out_${Date.now()}.inc`);
        try {
          // Compile from content first (validate before touching the .pory source)
          const tmpIn = path.join(os.tmpdir(), `poryscript_in_${Date.now()}.pory`);
          fs.writeFileSync(tmpIn, args.content, "utf8");
          try {
            const result = spawnSync(
              PORYSCRIPT_BIN,
              ["-i", tmpIn, "-o", tmpOut, "-lm=false",
               "-fc", path.join(PORYSCRIPT_DIR, "font_config.json"),
               "-cc", path.join(PORYSCRIPT_DIR, "command_config.json")],
              { cwd: PROJECT_ROOT, encoding: "utf8", timeout: 15_000 }
            );
            if (result.status !== 0) {
              return err(`poryscript compilation failed:\n${result.stderr || result.stdout || "(no output)"}`);
            }
          } finally {
            if (fs.existsSync(tmpIn)) fs.unlinkSync(tmpIn);
          }

          const compiled = fs.readFileSync(tmpOut, "utf8");
          // Write the .pory source (source of truth)
          fs.writeFileSync(poryDest, args.content, "utf8");
          // Write the compiled .inc
          const incDest = safePath(`data/maps/${args.map_name}/scripts.inc`);
          fs.writeFileSync(incDest, compiled, "utf8");
          return ok(
            `Written: data/maps/${args.map_name}/scripts.pory\n` +
            `Compiled: data/maps/${args.map_name}/scripts.inc (${compiled.length} bytes)`
          );
        } finally {
          if (fs.existsSync(tmpOut)) fs.unlinkSync(tmpOut);
        }
      }

      case "read_global_script": {
        const pory = safePath(`data/scripts/${args.script_name}.pory`);
        const inc = safePath(`data/scripts/${args.script_name}.inc`);
        if (fs.existsSync(pory)) return ok(fs.readFileSync(pory, "utf8"));
        return ok(fs.readFileSync(inc, "utf8"));
      }

      case "write_global_poryscript": {
        if (!fs.existsSync(PORYSCRIPT_BIN)) {
          return err(`poryscript binary not found at ${PORYSCRIPT_BIN}`);
        }

        const poryDest = safePath(`data/scripts/${args.script_name}.pory`);
        const tmpOut = path.join(os.tmpdir(), `poryscript_out_${Date.now()}.inc`);
        try {
          const tmpIn = path.join(os.tmpdir(), `poryscript_in_${Date.now()}.pory`);
          fs.writeFileSync(tmpIn, args.content, "utf8");
          try {
            const result = spawnSync(
              PORYSCRIPT_BIN,
              ["-i", tmpIn, "-o", tmpOut, "-lm=false",
               "-fc", path.join(PORYSCRIPT_DIR, "font_config.json"),
               "-cc", path.join(PORYSCRIPT_DIR, "command_config.json")],
              { cwd: PROJECT_ROOT, encoding: "utf8", timeout: 15_000 }
            );
            if (result.status !== 0) {
              return err(`poryscript compilation failed:\n${result.stderr || result.stdout || "(no output)"}`);
            }
          } finally {
            if (fs.existsSync(tmpIn)) fs.unlinkSync(tmpIn);
          }

          const compiled = fs.readFileSync(tmpOut, "utf8");
          fs.writeFileSync(poryDest, args.content, "utf8");
          const incDest = safePath(`data/scripts/${args.script_name}.inc`);
          fs.writeFileSync(incDest, compiled, "utf8");
          return ok(
            `Written: data/scripts/${args.script_name}.pory\n` +
            `Compiled: data/scripts/${args.script_name}.inc (${compiled.length} bytes)`
          );
        } finally {
          if (fs.existsSync(tmpOut)) fs.unlinkSync(tmpOut);
        }
      }

      default:
        return err(`Unknown tool: ${name}`);
    }
  } catch (e) {
    return err(e.message);
  }
});

const transport = new StdioServerTransport();
await server.connect(transport);
