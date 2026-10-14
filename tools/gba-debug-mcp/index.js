import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import { CallToolRequestSchema, ListToolsRequestSchema } from "@modelcontextprotocol/sdk/types.js";
import { spawn, execSync } from "child_process";
import path from "path";
import fs from "fs";
import { fileURLToPath } from "url";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PROJECT_ROOT = path.resolve(__dirname, "../../");

const GDB_BIN = "/opt/devkitpro/devkitARM/bin/arm-none-eabi-gdb";
const OBJDUMP = "arm-none-eabi-objdump";
const NM = "arm-none-eabi-nm";
const ADDR2LINE = "arm-none-eabi-addr2line";
const SIZE = "arm-none-eabi-size";
const NPROC = Number(execSync("nproc").toString().trim());

// --- GDB session state ---
let gdbProc = null;
let gdbBuf = "";
let gdbWaiter = null; // { resolve, reject, timer }

function ok(text) {
  return { content: [{ type: "text", text: String(text) }] };
}
function err(msg) {
  return { isError: true, content: [{ type: "text", text: String(msg) }] };
}

// Run a process, return { stdout, stderr, code }
function runProc(cmd, args, opts = {}) {
  return new Promise((resolve, reject) => {
    const proc = spawn(cmd, args, { cwd: PROJECT_ROOT, ...opts });
    let stdout = "", stderr = "";
    proc.stdout?.on("data", d => (stdout += d));
    proc.stderr?.on("data", d => (stderr += d));
    proc.on("close", code => resolve({ stdout, stderr, code }));
    proc.on("error", e => reject(new Error(`spawn ${cmd}: ${e.message}`)));
  });
}

// Resolve source file to project-relative path like "src/battle_main.c"
function resolveSourceFile(input) {
  // Strip leading "./" or "/"
  let rel = input.replace(/^\.\//, "").replace(/^\//, "");
  // If no directory component, assume src/
  if (!rel.includes("/")) rel = `src/${rel}`;
  // Add .c if missing
  if (!rel.endsWith(".c")) rel += ".c";
  const abs = path.join(PROJECT_ROOT, rel);
  if (!fs.existsSync(abs)) throw new Error(`Source file not found: ${rel}`);
  return rel; // e.g. "src/battle_main.c"
}

// Map a source-relative path to its build object path
function objPath(srcRel, modern) {
  // srcRel: "src/battle_main.c" -> "build/modern/src/battle_main.o"
  const base = srcRel.replace(/\.c$/, ".o");
  return path.join(PROJECT_ROOT, "build", modern ? "modern" : "emerald", base);
}

function defaultElf(modern = true) {
  return path.join(PROJECT_ROOT, modern ? "pokeemerald_modern.elf" : "pokeemerald.elf");
}

function defaultMap(modern = true) {
  return path.join(PROJECT_ROOT, modern ? "pokeemerald_modern.map" : "pokeemerald.map");
}

// ===== GDB helpers =====

function gdbHandleData(data) {
  gdbBuf += data.toString();
  // GDB CLI prompt appears as "(gdb) " (with trailing space, no newline after it)
  if (gdbBuf.endsWith("(gdb) ") && gdbWaiter) {
    const waiter = gdbWaiter;
    gdbWaiter = null;
    clearTimeout(waiter.timer);
    // Strip the trailing prompt
    const output = gdbBuf.replace(/\(gdb\) $/, "").trim();
    gdbBuf = "";
    waiter.resolve(output);
  }
}

function gdbSend(cmd, timeoutMs = 20000) {
  return new Promise((resolve, reject) => {
    if (!gdbProc) return reject(new Error("No active GDB session. Call gdb_connect first."));
    gdbBuf = "";
    const timer = setTimeout(() => {
      gdbWaiter = null;
      reject(new Error(`GDB timed out waiting for prompt after: ${cmd}`));
    }, timeoutMs);
    gdbWaiter = { resolve, reject, timer };
    gdbProc.stdin.write(cmd + "\n");
  });
}

function gdbWaitForPrompt(timeoutMs = 10000) {
  return new Promise((resolve, reject) => {
    if (!gdbProc) return reject(new Error("No GDB process"));
    gdbBuf = "";
    const timer = setTimeout(() => {
      gdbWaiter = null;
      reject(new Error("Timed out waiting for initial GDB prompt"));
    }, timeoutMs);
    gdbWaiter = { resolve, reject, timer };
  });
}

// ===== Tool handlers =====

async function handleRebuildFileDebug({ source_file, modern = true }) {
  let srcRel;
  try {
    srcRel = resolveSourceFile(source_file);
  } catch (e) {
    return err(e.message);
  }

  const obj = objPath(srcRel, modern);
  // Delete the .o to force recompile
  if (fs.existsSync(obj)) {
    fs.unlinkSync(obj);
  }

  const makeTarget = modern ? "modern" : "all";
  const makeArgs = [makeTarget, `O_LEVEL=0`, `DINFO=1`, `-j${NPROC}`];
  const { stdout, stderr, code } = await runProc("make", makeArgs);

  const combined = [stdout, stderr].filter(Boolean).join("\n").trim();
  if (code !== 0) {
    return err(`Build failed (exit ${code}):\n${combined}`);
  }
  return ok(
    `Rebuilt ${srcRel} with O_LEVEL=0 + debug symbols.\n\nBuild output:\n${combined || "(no output)"}`
  );
}

async function handleDisassemble({ function_name, elf_file, source_interleave = true }) {
  const elf = elf_file ? path.resolve(PROJECT_ROOT, elf_file) : defaultElf(true);
  if (!fs.existsSync(elf)) return err(`ELF not found: ${elf}`);

  const args = [
    `--disassemble=${function_name}`,
    "--no-show-raw-insn",
    source_interleave ? "-S" : "",
    "-l",
    elf,
  ].filter(Boolean);

  const { stdout, stderr, code } = await runProc(OBJDUMP, args);
  if (code !== 0 || !stdout.trim()) {
    // Fallback: look up address range via nm then disassemble that range
    const nmRes = await runProc(NM, ["-n", "--demangle", elf]);
    const lines = nmRes.stdout.split("\n");
    const match = lines.find(l => {
      const parts = l.trim().split(/\s+/);
      return parts[2] === function_name;
    });
    if (!match) {
      return err(
        `Symbol "${function_name}" not found in ${path.basename(elf)}.\n` +
          `stderr: ${stderr}`
      );
    }
    const [addrHex, , , ...rest] = match.trim().split(/\s+/);
    const startAddr = "0x" + addrHex;
    // Find the next symbol to determine end address
    const idx = lines.indexOf(match);
    const nextLine = lines.slice(idx + 1).find(l => l.trim().split(/\s+/)[1] === "T" || l.trim().split(/\s+/)[1] === "t");
    const endAddr = nextLine ? "0x" + nextLine.trim().split(/\s+/)[0] : undefined;
    const rangeArgs = [
      "--disassemble",
      "--no-show-raw-insn",
      source_interleave ? "-S" : "",
      "-l",
      `--start-address=${startAddr}`,
      endAddr ? `--stop-address=${endAddr}` : "",
      elf,
    ].filter(Boolean);
    const rangeRes = await runProc(OBJDUMP, rangeArgs);
    return ok(rangeRes.stdout || `(no disassembly output)\nstderr: ${rangeRes.stderr}`);
  }
  return ok(stdout);
}

async function handleListSymbols({ pattern, elf_file, type = "all" }) {
  const elf = elf_file ? path.resolve(PROJECT_ROOT, elf_file) : defaultElf(true);
  if (!fs.existsSync(elf)) return err(`ELF not found: ${elf}`);

  const args = ["--demangle", "-n"];
  if (type === "function") args.push("--defined-only");
  args.push(elf);

  const { stdout, code, stderr } = await runProc(NM, args);
  if (code !== 0) return err(`nm failed: ${stderr}`);

  let lines = stdout.split("\n").filter(Boolean);
  if (type === "function") lines = lines.filter(l => / [Tt] /.test(l));
  else if (type === "object") lines = lines.filter(l => / [BbDdRr] /.test(l));

  if (pattern) {
    const re = new RegExp(pattern, "i");
    lines = lines.filter(l => re.test(l));
  }

  if (lines.length === 0) return ok("No symbols matched.");
  // Cap output to avoid flooding context
  const capped = lines.length > 200;
  const display = lines.slice(0, 200).join("\n");
  return ok(display + (capped ? `\n\n... (${lines.length - 200} more results — narrow your pattern)` : ""));
}

async function handleAddr2line({ address, elf_file }) {
  const elf = elf_file ? path.resolve(PROJECT_ROOT, elf_file) : defaultElf(true);
  if (!fs.existsSync(elf)) return err(`ELF not found: ${elf}`);

  const addr = address.startsWith("0x") || address.startsWith("0X") ? address : "0x" + address;
  const { stdout, stderr, code } = await runProc(ADDR2LINE, ["-e", elf, "-f", "-p", "-C", addr]);
  if (code !== 0) return err(`addr2line failed: ${stderr}`);
  return ok(stdout.trim() || `No debug info for ${addr}`);
}

async function handleMapLookup({ pattern, map_file }) {
  const mapPath = map_file ? path.resolve(PROJECT_ROOT, map_file) : defaultMap(true);
  if (!fs.existsSync(mapPath)) return err(`Map file not found: ${mapPath}`);

  const text = fs.readFileSync(mapPath, "utf8");
  const re = new RegExp(pattern, "i");
  const lines = text.split("\n").filter(l => re.test(l));

  if (lines.length === 0) return ok("No matches in map file.");
  const capped = lines.length > 100;
  return ok(lines.slice(0, 100).join("\n") + (capped ? `\n... (${lines.length - 100} more)` : ""));
}

async function handleElfSections({ elf_file }) {
  const elf = elf_file ? path.resolve(PROJECT_ROOT, elf_file) : defaultElf(true);
  if (!fs.existsSync(elf)) return err(`ELF not found: ${elf}`);

  const { stdout, stderr, code } = await runProc(SIZE, ["-A", elf]);
  if (code !== 0) return err(`size failed: ${stderr}`);
  return ok(stdout);
}

async function handleGdbConnect({ port = 2345, elf_file }) {
  if (gdbProc) {
    return err("A GDB session is already active. Call gdb_disconnect first.");
  }

  const elf = elf_file ? path.resolve(PROJECT_ROOT, elf_file) : defaultElf(true);
  if (!fs.existsSync(elf)) return err(`ELF not found: ${elf}`);
  if (!fs.existsSync(GDB_BIN)) return err(`GDB not found at ${GDB_BIN}`);

  gdbProc = spawn(GDB_BIN, ["--quiet", elf], {
    cwd: PROJECT_ROOT,
    stdio: ["pipe", "pipe", "pipe"],
  });

  gdbProc.stdout.on("data", gdbHandleData);
  gdbProc.stderr.on("data", d => { /* swallow GDB stderr */ });
  gdbProc.on("close", () => {
    gdbProc = null;
    gdbBuf = "";
    if (gdbWaiter) {
      const w = gdbWaiter;
      gdbWaiter = null;
      clearTimeout(w.timer);
      w.reject(new Error("GDB process exited unexpectedly"));
    }
  });

  let startupOutput;
  try {
    startupOutput = await gdbWaitForPrompt(15000);
  } catch (e) {
    gdbProc?.kill();
    gdbProc = null;
    return err(`GDB failed to start: ${e.message}`);
  }

  // Connect to mGBA stub
  let connectOutput;
  try {
    connectOutput = await gdbSend(`target remote localhost:${port}`, 15000);
  } catch (e) {
    gdbProc?.kill();
    gdbProc = null;
    return err(`Failed to connect to mGBA on port ${port}: ${e.message}\nMake sure mGBA is running with -g flag.`);
  }

  return ok(`GDB started with ${path.basename(elf)}\n\nStartup:\n${startupOutput}\n\nConnection:\n${connectOutput}`);
}

async function handleGdbExec({ command, timeout_ms = 20000 }) {
  if (!gdbProc) return err("No active GDB session. Call gdb_connect first.");
  try {
    const output = await gdbSend(command, timeout_ms);
    return ok(output || "(no output)");
  } catch (e) {
    return err(e.message);
  }
}

async function handleGdbDisconnect() {
  if (!gdbProc) return ok("No active GDB session.");
  try {
    await Promise.race([
      gdbSend("quit", 3000),
      new Promise(r => setTimeout(r, 3000)),
    ]);
  } catch (_) {
    // ignore
  }
  gdbProc?.kill();
  gdbProc = null;
  gdbBuf = "";
  gdbWaiter = null;
  return ok("GDB session terminated.");
}

// ===== MCP Server setup =====

const server = new Server(
  { name: "gba-debug-mcp", version: "1.0.0" },
  { capabilities: { tools: {} } }
);

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: [
    {
      name: "rebuild_file_debug",
      description:
        "Recompile a single .c source file with O_LEVEL=0 (no optimization) and full debug symbols (DINFO=1), then relink the ROM. Use this before disassembling to get readable ARM output. Only the specified file's .o is deleted and recompiled; other objects are reused.",
      inputSchema: {
        type: "object",
        properties: {
          source_file: {
            type: "string",
            description:
              'Project-relative source path, e.g. "src/battle_main.c" or just "battle_main". Assumes src/ if no directory given.',
          },
          modern: {
            type: "boolean",
            description: "true = modern build (arm-none-eabi-gcc), false = classic (agbcc). Default: true.",
          },
        },
        required: ["source_file"],
      },
    },
    {
      name: "disassemble",
      description:
        "Disassemble a specific function from the ELF using arm-none-eabi-objdump. With source_interleave=true (default), C source lines are interleaved with ARM instructions. Rebuild with rebuild_file_debug first to get O0 output.",
      inputSchema: {
        type: "object",
        properties: {
          function_name: { type: "string", description: "Exact C function name, e.g. BattleMainCB2" },
          elf_file: {
            type: "string",
            description: "Path to ELF file relative to project root. Default: pokeemerald_modern.elf",
          },
          source_interleave: {
            type: "boolean",
            description: "Interleave C source lines with disassembly (-S flag). Default: true. Requires DINFO=1 build.",
          },
        },
        required: ["function_name"],
      },
    },
    {
      name: "list_symbols",
      description:
        "List symbols in the ELF via arm-none-eabi-nm, sorted by address. Use pattern to filter by name (regex). Useful for finding function addresses, checking what got linked, or exploring the symbol table.",
      inputSchema: {
        type: "object",
        properties: {
          pattern: { type: "string", description: "Regex filter on symbol name (case-insensitive). Omit to list all." },
          elf_file: { type: "string", description: "Path to ELF relative to project root. Default: pokeemerald_modern.elf" },
          type: {
            type: "string",
            enum: ["all", "function", "object"],
            description: '"function" = text symbols only, "object" = data symbols only, "all" = everything. Default: "all"',
          },
        },
        required: [],
      },
    },
    {
      name: "addr2line",
      description:
        "Translate a hex address (e.g. from a crash or GDB) to a source file + line number using arm-none-eabi-addr2line. Requires a DINFO=1 build.",
      inputSchema: {
        type: "object",
        properties: {
          address: { type: "string", description: 'Hex address, with or without 0x prefix, e.g. "0x0803a1bc" or "803a1bc"' },
          elf_file: { type: "string", description: "Path to ELF relative to project root. Default: pokeemerald_modern.elf" },
        },
        required: ["address"],
      },
    },
    {
      name: "map_lookup",
      description:
        "Search the linker .map file for a symbol, section, or file name. Returns matching lines showing link addresses and input object files. Useful for confirming where something was placed in ROM/RAM.",
      inputSchema: {
        type: "object",
        properties: {
          pattern: { type: "string", description: "Regex search pattern (case-insensitive)." },
          map_file: { type: "string", description: "Path to .map file relative to project root. Default: pokeemerald_modern.map" },
        },
        required: ["pattern"],
      },
    },
    {
      name: "elf_sections",
      description:
        "Show section sizes from the ELF using arm-none-eabi-size -A. Useful for checking ROM, EWRAM, and IWRAM usage.",
      inputSchema: {
        type: "object",
        properties: {
          elf_file: { type: "string", description: "Path to ELF relative to project root. Default: pokeemerald_modern.elf" },
        },
        required: [],
      },
    },
    {
      name: "gdb_connect",
      description:
        "Spawn arm-none-eabi-gdb with the project ELF and connect to a running mGBA GDB stub via TCP. mGBA must be started with the -g flag (e.g. mGBA.exe pokeemerald_modern.gba -g). Only one session can be active at a time.",
      inputSchema: {
        type: "object",
        properties: {
          port: { type: "number", description: "mGBA GDB stub port. Default: 2345" },
          elf_file: { type: "string", description: "Path to ELF relative to project root. Default: pokeemerald_modern.elf" },
        },
        required: [],
      },
    },
    {
      name: "gdb_exec",
      description:
        "Send a GDB command to the active session and return the output. Examples: 'break BattleMainCB2', 'continue', 'bt', 'info locals', 'print gBattleTypeFlags', 'x/16xw 0x2000000', 'info registers'. Requires gdb_connect to have been called first.",
      inputSchema: {
        type: "object",
        properties: {
          command: { type: "string", description: "GDB CLI command string." },
          timeout_ms: {
            type: "number",
            description: "Milliseconds to wait for (gdb) prompt. Default 20000. Increase for 'continue' that runs longer.",
          },
        },
        required: ["command"],
      },
    },
    {
      name: "gdb_disconnect",
      description: "Send 'quit' and kill the active GDB session. Call this when done debugging.",
      inputSchema: {
        type: "object",
        properties: {},
        required: [],
      },
    },
  ],
}));

server.setRequestHandler(CallToolRequestSchema, async (req) => {
  const { name, arguments: args } = req.params;
  try {
    switch (name) {
      case "rebuild_file_debug": return await handleRebuildFileDebug(args);
      case "disassemble":        return await handleDisassemble(args);
      case "list_symbols":       return await handleListSymbols(args);
      case "addr2line":          return await handleAddr2line(args);
      case "map_lookup":         return await handleMapLookup(args);
      case "elf_sections":       return await handleElfSections(args);
      case "gdb_connect":        return await handleGdbConnect(args);
      case "gdb_exec":           return await handleGdbExec(args);
      case "gdb_disconnect":     return await handleGdbDisconnect();
      default: return err(`Unknown tool: ${name}`);
    }
  } catch (e) {
    return err(`Unexpected error in ${name}: ${e.message}\n${e.stack}`);
  }
});

const transport = new StdioServerTransport();
await server.connect(transport);
