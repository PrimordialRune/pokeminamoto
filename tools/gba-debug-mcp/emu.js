// Headless emulator tools (emu_*) for gba-debug-mcp.
//
// Drives tools/gba-emu/gbaemu (a small libmgba host, one JSON line per
// command) and, for type-aware inspection, gdb-multiarch attached to the
// host's GDB stub. Works in cloud sessions and in WSL; no display needed.
import { spawn, execFileSync } from "child_process";
import path from "path";
import fs from "fs";
import net from "net";

const KEY_BITS = { A: 0, B: 1, SELECT: 2, START: 3, RIGHT: 4, LEFT: 5, UP: 6, DOWN: 7, R: 8, L: 9 };

let PROJECT_ROOT;
let HOST_BIN;
let OUT_DIR;

function ok(text, extra = []) {
  return { content: [{ type: "text", text: typeof text === "string" ? text : JSON.stringify(text, null, 1) }, ...extra] };
}
function err(msg) {
  return { isError: true, content: [{ type: "text", text: String(msg) }] };
}

// ---------------------------------------------------------------- symbols

class Symbols {
  constructor(elf) {
    this.elf = elf;
    this.byName = new Map();
    this.sorted = [];
    const out = execFileSync("arm-none-eabi-nm", ["-n", "-S", "--defined-only", elf], {
      maxBuffer: 256 * 1024 * 1024,
    }).toString();
    for (const line of out.split("\n")) {
      const p = line.trim().split(/\s+/);
      let addr, size, type, name;
      if (p.length === 4) [addr, size, type, name] = p;
      else if (p.length === 3) [addr, type, name] = p;
      else continue;
      const s = {
        name,
        type,
        addr: parseInt(addr, 16) & (/[Tt]/.test(type) ? ~1 : ~0),
        size: size ? parseInt(size, 16) : 0,
      };
      s.addr >>>= 0;
      if (!this.byName.has(name)) this.byName.set(name, s);
      if (!name.startsWith("$") && !name.startsWith(".")) this.sorted.push(s);
    }
    this.sorted.sort((a, b) => a.addr - b.addr);
  }

  // Nearest symbol at or below addr, as "Name+0x12".
  describe(addr) {
    addr >>>= 0;
    let lo = 0, hi = this.sorted.length - 1, best = null;
    while (lo <= hi) {
      const mid = (lo + hi) >> 1;
      if (this.sorted[mid].addr <= addr) { best = this.sorted[mid]; lo = mid + 1; }
      else hi = mid - 1;
    }
    if (!best) return null;
    const off = addr - best.addr;
    if (best.size && off >= best.size && off > 0x1000) return null;
    return off ? `${best.name}+0x${off.toString(16)}` : best.name;
  }
}

let symCache = null;
function symbols(elf) {
  const mtime = fs.statSync(elf).mtimeMs;
  if (!symCache || symCache.elf !== elf || symCache.mtime !== mtime) {
    symCache = { elf, mtime, syms: new Symbols(elf) };
  }
  return symCache.syms;
}

function addr2line(elf, addr) {
  try {
    const out = execFileSync("arm-none-eabi-addr2line", ["-e", elf, "-f", "-C", "0x" + addr.toString(16)]).toString().split("\n");
    const loc = (out[1] || "").trim();
    if (!loc || loc.startsWith("??")) return null;
    return path.relative(PROJECT_ROOT, path.resolve(PROJECT_ROOT, loc)).replace(/ \(discriminator.*\)/, "");
  } catch {
    return null;
  }
}

// ---------------------------------------------------------------- host

let host = null; // { proc, rom, elf, buf, queue, gdbPort }

function hostSend(cmd, timeoutMs = 120000) {
  return new Promise((resolve, reject) => {
    if (!host) return reject(new Error("Emulator not running. Call emu_start first."));
    const timer = setTimeout(() => {
      host.queue = host.queue.filter(w => w.timer !== timer);
      reject(new Error(`emulator timed out on: ${cmd}`));
    }, timeoutMs);
    host.queue.push({ resolve, reject, timer });
    host.proc.stdin.write(cmd + "\n");
  });
}

function hostOnData(data) {
  host.buf += data.toString();
  let nl;
  while ((nl = host.buf.indexOf("\n")) >= 0) {
    const line = host.buf.slice(0, nl);
    host.buf = host.buf.slice(nl + 1);
    const w = host.queue.shift();
    if (!w) continue;
    clearTimeout(w.timer);
    try {
      const j = JSON.parse(line);
      if (j.ok === false) w.reject(new Error(j.error));
      else w.resolve(j);
    } catch {
      w.reject(new Error(`bad reply from emulator: ${line.slice(0, 300)}`));
    }
  }
}

async function stopHost() {
  await gdbStop();
  if (!host) return;
  const h = host;
  try { await hostSend("quit", 3000); } catch {}
  h.proc.kill();
  host = null;
}

// Resolve "0x2024284", "gPlayerParty", "gPlayerParty+0x64", "src/file.c:123".
async function resolveLocation(loc) {
  loc = String(loc).trim();
  if (/^0x[0-9a-f]+$/i.test(loc) || /^[0-9a-f]{7,8}$/i.test(loc)) {
    return { addr: parseInt(loc, 16) >>> 0, size: 0, label: loc };
  }
  const fl = loc.match(/^(.+\.[ch]):(\d+)$/);
  if (fl) {
    const out = await gdbOffline([`info line ${fl[1]}:${fl[2]}`]);
    const m = out.match(/starts at address (0x[0-9a-f]+)/i) || out.match(/is at address (0x[0-9a-f]+)/i);
    if (!m) throw new Error(`No code for ${loc}: ${out.trim().slice(0, 300)}`);
    return { addr: parseInt(m[1], 16) >>> 0, size: 0, label: loc };
  }
  const sm = loc.match(/^([A-Za-z_][\w.]*)\s*(?:\+\s*(0x[0-9a-f]+|\d+))?$/i);
  if (!sm) throw new Error(`Can't parse location "${loc}" (use 0xADDR, symbol, symbol+off or file.c:line)`);
  const s = symbols(host.elf).byName.get(sm[1]);
  if (!s) throw new Error(`Unknown symbol ${sm[1]} in ${path.basename(host.elf)}`);
  const off = sm[2] ? Number(sm[2]) : 0;
  return { addr: (s.addr + off) >>> 0, size: off ? 0 : s.size, label: loc };
}

function decorate(j) {
  if (!host || !host.elf) return j;
  const syms = symbols(host.elf);
  for (const k of ["pc", "lr", "address"]) {
    if (typeof j[k] === "string" && j[k].startsWith("0x")) {
      const a = parseInt(j[k], 16);
      const d = syms.describe(a);
      if (d) j[`${k}_sym`] = d;
      if (k === "pc") {
        const src = addr2line(host.elf, a);
        if (src) j.pc_src = src;
      }
    }
  }
  return j;
}

async function withLog(j) {
  if (j.log > 0) {
    const l = await hostSend("log");
    j.log = l.lines;
  } else {
    delete j.log;
  }
  return j;
}

async function screenshotContent(name) {
  fs.mkdirSync(OUT_DIR, { recursive: true });
  const file = path.join(OUT_DIR, name || `shot-${Date.now()}.png`);
  const r = await hostSend(`screenshot ${file}`);
  const data = fs.readFileSync(file).toString("base64");
  return { file: path.relative(PROJECT_ROOT, file), frame: r.frame, image: { type: "image", data, mimeType: "image/png" } };
}

async function result(j, wantShot) {
  j = decorate(await withLog(j));
  if (!wantShot) return ok(j);
  const s = await screenshotContent();
  j.screenshot = s.file;
  return ok(j, [s.image]);
}

function parseKeys(spec) {
  let mask = 0;
  for (const k of String(spec || "").toUpperCase().split(/[\s+,]+/).filter(Boolean)) {
    if (!(k in KEY_BITS)) throw new Error(`Unknown button ${k}; use ${Object.keys(KEY_BITS).join(" ")}`);
    mask |= 1 << KEY_BITS[k];
  }
  return mask;
}

// ---------------------------------------------------------------- gdb

let gdb = null; // { proc, buf, waiter }

function gdbProcess(args) {
  return spawn("gdb-multiarch", ["-q", "-nx", ...args], { cwd: PROJECT_ROOT, stdio: ["pipe", "pipe", "pipe"] });
}

// One-shot gdb without a target, for file:line lookups.
function gdbOffline(cmds) {
  return new Promise((resolve, reject) => {
    const p = gdbProcess(["-batch", "-ex", "set architecture armv4t", ...cmds.flatMap(c => ["-ex", c]), host.elf]);
    let out = "";
    p.stdout.on("data", d => (out += d));
    p.stderr.on("data", d => (out += d));
    p.on("close", () => resolve(out));
    p.on("error", e => reject(new Error(`gdb-multiarch: ${e.message}`)));
  });
}

function gdbSendRaw(cmd, timeoutMs) {
  return new Promise((resolve, reject) => {
    gdb.buf = "";
    const timer = setTimeout(() => {
      gdb.waiter = null;
      reject(new Error(`gdb timed out after ${timeoutMs} ms on: ${cmd}`));
    }, timeoutMs);
    gdb.waiter = { resolve, timer };
    if (cmd !== null) gdb.proc.stdin.write(cmd + "\n");
  });
}

function gdbOnData(d) {
  gdb.buf += d.toString();
  if (gdb.buf.endsWith("(gdb) ") && gdb.waiter) {
    const w = gdb.waiter;
    gdb.waiter = null;
    clearTimeout(w.timer);
    w.resolve(gdb.buf.replace(/\(gdb\) $/, "").trim());
  }
}

function freePort() {
  return new Promise((resolve, reject) => {
    const s = net.createServer();
    s.listen(0, "127.0.0.1", () => {
      const { port } = s.address();
      s.close(() => resolve(port));
    });
    s.on("error", reject);
  });
}

async function gdbStart() {
  if (gdb) return;
  if (!host.gdbPort) {
    const port = await freePort();
    await hostSend(`gdb ${port}`);
    host.gdbPort = port;
  }
  const proc = gdbProcess([host.elf]);
  gdb = { proc, buf: "", waiter: null };
  proc.stdout.on("data", gdbOnData);
  proc.stderr.on("data", gdbOnData);
  proc.on("close", () => {
    if (gdb && gdb.proc === proc) gdb = null;
  });
  proc.on("error", () => {
    if (gdb && gdb.proc === proc) gdb = null;
  });
  await gdbSendRaw(null, 60000); // initial prompt after loading the ELF
  for (const c of ["set pagination off", "set confirm off", "set width 0", "set architecture armv4t",
                   "set stack-cache off", "set code-cache off"]) {
    await gdbSendRaw(c, 10000);
  }
  const out = await gdbSendRaw(`target remote 127.0.0.1:${host.gdbPort}`, 30000);
  if (/Connection refused|timed out|Remote communication error/i.test(out)) {
    gdbStop();
    throw new Error(`gdb could not attach: ${out}`);
  }
}

async function gdbStop() {
  if (!gdb) return;
  const g = gdb;
  gdb = null;
  g.proc.kill("SIGKILL");
}

// ---------------------------------------------------------------- handlers

const handlers = {
  async emu_start({ rom, elf, sav, state, build = false, scale = 2 }) {
    if (host) await stopHost();
    if (!fs.existsSync(HOST_BIN)) {
      return err(`${path.relative(PROJECT_ROOT, HOST_BIN)} is not built. Run tools/setup-cloud.sh (or: sudo apt install libmgba-dev gdb-multiarch && make -C tools/gba-emu).`);
    }
    if (build) {
      const r = await new Promise(res => {
        const p = spawn("make", ["modern", "DINFO=1", `-j${Math.max(1, (await_nproc()))}`], { cwd: PROJECT_ROOT });
        let o = "";
        p.stdout.on("data", d => (o += d));
        p.stderr.on("data", d => (o += d));
        p.on("close", code => res({ code, o }));
      });
      if (r.code !== 0) return err(`make modern DINFO=1 failed:\n${r.o.slice(-4000)}`);
    }
    const romPath = path.resolve(PROJECT_ROOT, rom || "pokeemerald_modern.gba");
    const elfPath = path.resolve(PROJECT_ROOT, elf || romPath.replace(/\.gba$/, ".elf"));
    if (!fs.existsSync(romPath)) return err(`ROM not found: ${romPath}. Build with: make modern DINFO=1 -j$(nproc), or pass build: true.`);
    const proc = spawn(HOST_BIN, [], { cwd: PROJECT_ROOT, stdio: ["pipe", "pipe", "pipe"] });
    host = { proc, rom: romPath, elf: fs.existsSync(elfPath) ? elfPath : null, buf: "", queue: [], gdbPort: 0 };
    proc.stdout.on("data", hostOnData);
    proc.stderr.on("data", () => {});
    proc.on("close", () => {
      if (host && host.proc === proc) {
        for (const w of host.queue) { clearTimeout(w.timer); w.reject(new Error("emulator exited")); }
        host = null;
      }
    });
    const info = await hostSend(`load ${romPath}`);
    await hostSend(`scale ${scale}`);
    if (sav) Object.assign(info, { sav: await hostSend(`loadsav ${path.resolve(PROJECT_ROOT, sav)}`) });
    if (state) Object.assign(info, { state: await hostSend(`loadstate ${path.resolve(PROJECT_ROOT, state)}`) });
    info.rom = path.relative(PROJECT_ROOT, romPath);
    info.elf = host.elf ? path.relative(PROJECT_ROOT, host.elf) : "(none: no symbols)";
    if (host.elf) symbols(host.elf);
    return ok(info);
  },

  async emu_stop() {
    if (!host) return ok("Emulator not running.");
    await stopHost();
    return ok("Emulator stopped.");
  },

  async emu_run({ frames = 60, until, screenshot = false }) {
    let tmp = null;
    if (until) {
      const loc = await resolveLocation(until);
      tmp = (await hostSend(`break 0x${loc.addr.toString(16)}`)).id;
    }
    try {
      const j = await hostSend(`run ${frames}`, 600000);
      return await result(j, screenshot);
    } finally {
      if (tmp !== null && host) await hostSend(`delete ${tmp}`).catch(() => {});
    }
  },

  async emu_step({ count = 1 }) {
    return result(await hostSend(`step ${count}`), false);
  },

  async emu_press({ buttons, hold_frames = 4, after_frames = 20, times = 1, screenshot = false }) {
    const mask = parseKeys(buttons);
    let j;
    for (let i = 0; i < times; i++) {
      await hostSend(`keys ${mask}`);
      j = await hostSend(`run ${hold_frames}`);
      await hostSend("keys 0");
      if (j.stop !== "frames") break;
      j = await hostSend(`run ${after_frames}`);
      if (j.stop !== "frames") break;
    }
    await hostSend("keys 0");
    return result(j, screenshot);
  },

  async emu_hold({ buttons }) {
    return ok(await hostSend(`keys ${parseKeys(buttons)}`));
  },

  async emu_screenshot({ file }) {
    const s = await screenshotContent(file);
    return ok({ file: s.file, frame: s.frame }, [s.image]);
  },

  async emu_regs() {
    return result(await hostSend("regs"), false);
  },

  async emu_read({ location, length, format = "hex" }) {
    const loc = await resolveLocation(location);
    const len = Math.min(length || loc.size || 16, 0x10000);
    const r = await hostSend(`read 0x${loc.addr.toString(16)} ${len}`);
    const buf = Buffer.from(r.hex, "hex");
    const out = { address: r.address, symbol: host.elf ? symbols(host.elf).describe(loc.addr) : null, length: len };
    const m = format.match(/^([us])(8|16|32)$/);
    if (m) {
      const w = Number(m[2]) / 8;
      const vals = [];
      for (let i = 0; i + w <= buf.length; i += w) {
        const fn = `read${m[1] === "s" ? "Int" : "UInt"}${m[2]}${w > 1 ? "LE" : ""}`;
        vals.push(buf[fn](i));
      }
      out[format] = vals;
    } else {
      const rows = [];
      for (let i = 0; i < buf.length; i += 16) {
        rows.push(`${(loc.addr + i).toString(16).padStart(8, "0")}: ${buf.subarray(i, i + 16).toString("hex").replace(/(..)/g, "$1 ").trim()}`);
      }
      out.hex = rows.join("\n");
    }
    return ok(out);
  },

  async emu_write({ location, hex, value, size = 4 }) {
    const loc = await resolveLocation(location);
    let bytes = hex;
    if (bytes === undefined) {
      if (value === undefined) return err("Pass hex (bytes) or value (+ size 1/2/4).");
      const b = Buffer.alloc(size);
      if (size === 1) b.writeUInt8(Number(value) & 0xff);
      else if (size === 2) b.writeUInt16LE(Number(value) & 0xffff);
      else b.writeUInt32LE(Number(value) >>> 0);
      bytes = b.toString("hex");
    }
    return ok(await hostSend(`write 0x${loc.addr.toString(16)} ${bytes.replace(/\s+/g, "")}`));
  },

  async emu_break({ location }) {
    const loc = await resolveLocation(location);
    const r = await hostSend(`break 0x${loc.addr.toString(16)}`);
    return ok(decorate({ ...r, location: loc.label }));
  },

  async emu_watch({ location, access = "w" }) {
    const loc = await resolveLocation(location);
    const r = await hostSend(`watch 0x${loc.addr.toString(16)} ${access}`);
    return ok(decorate({ ...r, location: loc.label }));
  },

  async emu_delete({ id }) {
    return ok(await hostSend(`delete ${id}`));
  },

  async emu_list() {
    const r = await hostSend("list");
    for (const b of [...r.breakpoints, ...r.watchpoints]) decorate(b);
    return ok(r);
  },

  async emu_savestate({ file }) {
    return ok(await hostSend(`savestate ${path.resolve(PROJECT_ROOT, file)}`));
  },

  async emu_loadstate({ file }) {
    return result(await hostSend(`loadstate ${path.resolve(PROJECT_ROOT, file)}`), false);
  },

  async emu_reset() {
    return ok(await hostSend("reset"));
  },

  async emu_log() {
    return ok(await hostSend("log"));
  },

  async emu_gdb({ command, timeout_ms = 60000 }) {
    if (!host) return err("Emulator not running. Call emu_start first.");
    if (!host.elf) return err("No ELF next to the ROM, so gdb has no symbols.");
    await gdbStart();
    // Native emu_run/emu_step move the CPU behind gdb's back; drop its caches.
    await gdbSendRaw("maintenance flush register-cache", 10000);
    try {
      return ok((await gdbSendRaw(command, timeout_ms)) || "(no output)");
    } catch (e) {
      gdb?.proc.kill("SIGINT");
      return err(`${e.message}. Sent an interrupt; the target stops on its own after the gdbframes cap (3600 frames).`);
    }
  },
};

function await_nproc() {
  try { return Number(execFileSync("nproc").toString().trim()); } catch { return 4; }
}

// ---------------------------------------------------------------- schemas

const loc = { type: "string", description: 'Address or symbol: "0x02024284", "gPlayerParty", "gPlayerParty+0x64", or for code "src/battle_main.c:1234".' };

export const emuTools = [
  {
    name: "emu_start",
    description: "Start the headless emulator (mGBA core, no display) on a ROM and load its ELF symbols. Default ROM pokeemerald_modern.gba (build with make modern DINFO=1 for source-level gdb). Emulation only advances when you call emu_run/emu_press/emu_step, so runs are deterministic. Boot to the title screen takes ~2000 frames; use sav or state to skip ahead.",
    inputSchema: {
      type: "object",
      properties: {
        rom: { type: "string", description: "ROM path relative to repo root. Default pokeemerald_modern.gba." },
        elf: { type: "string", description: "ELF for symbols. Default: ROM path with .elf." },
        sav: { type: "string", description: "Battery save (.sav) to load. Read-only; the file is never written." },
        state: { type: "string", description: "Savestate file (from emu_savestate) to load after boot." },
        build: { type: "boolean", description: "Run make modern DINFO=1 first. Default false." },
        scale: { type: "number", description: "Screenshot upscale factor 1-4. Default 2." },
      },
    },
  },
  { name: "emu_stop", description: "Stop the emulator (and its gdb session).", inputSchema: { type: "object", properties: {} } },
  {
    name: "emu_run",
    description: "Run the emulator for N frames (60 = one second of game time), stopping early on a breakpoint, watchpoint or illegal opcode. Returns why it stopped, pc with symbol and source line, and any DebugPrintf / emulator error log lines. With breakpoints or watchpoints set, emulation runs about 400 frames/s instead of thousands.",
    inputSchema: {
      type: "object",
      properties: {
        frames: { type: "number", description: "Frames to run. Default 60." },
        until: { ...loc, description: "Optional temporary breakpoint: run until this function/address/file:line is reached (or frames run out)." },
        screenshot: { type: "boolean", description: "Attach a screenshot after running." },
      },
    },
  },
  {
    name: "emu_step",
    description: "Execute N CPU instructions (default 1). For source-level next/finish use emu_gdb.",
    inputSchema: { type: "object", properties: { count: { type: "number" } } },
  },
  {
    name: "emu_press",
    description: "Press buttons: hold them for hold_frames, release, then run after_frames so the game reacts. Repeat with times. Buttons: A B SELECT START UP DOWN LEFT RIGHT L R, combine with '+' (e.g. \"UP+B\").",
    inputSchema: {
      type: "object",
      properties: {
        buttons: { type: "string" },
        hold_frames: { type: "number", description: "Default 4." },
        after_frames: { type: "number", description: "Default 20." },
        times: { type: "number", description: "Default 1." },
        screenshot: { type: "boolean", description: "Attach a screenshot at the end." },
      },
      required: ["buttons"],
    },
  },
  {
    name: "emu_hold",
    description: "Set the buttons held during following emu_run calls (e.g. \"RIGHT\" to walk, \"B+RIGHT\" to run). Empty string releases all.",
    inputSchema: { type: "object", properties: { buttons: { type: "string" } }, required: ["buttons"] },
  },
  {
    name: "emu_screenshot",
    description: "Screenshot the current frame. Returns the image and saves a PNG under build/emu/.",
    inputSchema: { type: "object", properties: { file: { type: "string", description: "Optional file name inside build/emu/." } } },
  },
  { name: "emu_regs", description: "CPU registers, with pc/lr symbolized and the pc source line.", inputSchema: { type: "object", properties: {} } },
  {
    name: "emu_read",
    description: "Read memory by address or symbol. Length defaults to the symbol's size (or 16). format: hex (dump), u8/u16/u32/s8/s16/s32 (little-endian arrays). For typed structs use emu_gdb 'print'.",
    inputSchema: {
      type: "object",
      properties: { location: loc, length: { type: "number" }, format: { type: "string", enum: ["hex", "u8", "u16", "u32", "s8", "s16", "s32"] } },
      required: ["location"],
    },
  },
  {
    name: "emu_write",
    description: "Write memory: hex bytes, or value with size 1/2/4 (little-endian). Writes to ROM patch the loaded image only.",
    inputSchema: {
      type: "object",
      properties: { location: loc, hex: { type: "string" }, value: { type: "number" }, size: { type: "number", enum: [1, 2, 4] } },
      required: ["location"],
    },
  },
  {
    name: "emu_break",
    description: "Set a breakpoint on a function, address or file.c:line. Stops emu_run before that instruction executes. Returns the breakpoint id.",
    inputSchema: { type: "object", properties: { location: loc }, required: ["location"] },
  },
  {
    name: "emu_watch",
    description: "Set a watchpoint on a variable or address: access w (write, default), r (read), rw, c (write that changes the value). emu_run stops with old/new values and the pc that did it.",
    inputSchema: {
      type: "object",
      properties: { location: loc, access: { type: "string", enum: ["w", "r", "rw", "c"] } },
      required: ["location"],
    },
  },
  { name: "emu_delete", description: "Delete a breakpoint or watchpoint by id.", inputSchema: { type: "object", properties: { id: { type: "number" } }, required: ["id"] } },
  { name: "emu_list", description: "List breakpoints and watchpoints.", inputSchema: { type: "object", properties: {} } },
  {
    name: "emu_savestate",
    description: "Save the full emulator state to a file (e.g. build/emu/battle.ss). Only loads into the same ROM build.",
    inputSchema: { type: "object", properties: { file: { type: "string" } }, required: ["file"] },
  },
  {
    name: "emu_loadstate",
    description: "Load a savestate written by emu_savestate.",
    inputSchema: { type: "object", properties: { file: { type: "string" } }, required: ["file"] },
  },
  { name: "emu_reset", description: "Reset the console (keeps breakpoints).", inputSchema: { type: "object", properties: {} } },
  { name: "emu_log", description: "Fetch and clear buffered DebugPrintf output (needs NDEBUG off in include/config.h) and emulator errors.", inputSchema: { type: "object", properties: {} } },
  {
    name: "emu_gdb",
    description: "Run a gdb command against the paused emulator with full DWARF types: 'print gPlayerParty[0]', 'print/x gBattleMons[1].hp', 'ptype struct Pokemon', 'bt', 'info locals', 'frame 1', 'x/8xw $sp', 'list'. 'next', 'step', 'finish', 'continue' and gdb breakpoints also work (continue stops after 3600 frames). Attaches gdb-multiarch on first use; emu_run/emu_break and gdb share breakpoints.",
    inputSchema: {
      type: "object",
      properties: { command: { type: "string" }, timeout_ms: { type: "number", description: "Default 60000." } },
      required: ["command"],
    },
  },
];

export function initEmu(projectRoot) {
  PROJECT_ROOT = projectRoot;
  HOST_BIN = path.join(projectRoot, "tools/gba-emu/gbaemu");
  OUT_DIR = path.join(projectRoot, "build/emu");
  const cleanup = () => { if (host) host.proc.kill(); if (gdb) gdb.proc.kill("SIGKILL"); };
  process.on("exit", cleanup);
}

export async function callEmuTool(name, args) {
  const h = handlers[name];
  if (!h) return null;
  try {
    return await h(args || {});
  } catch (e) {
    return err(e.message);
  }
}
