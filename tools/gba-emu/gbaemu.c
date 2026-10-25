/* gbaemu: a headless mGBA host for agents and scripts.
 *
 * Links libmgba (apt: libmgba-dev, mGBA >= 0.10). Reads one command per line
 * on stdin and answers each with one line of JSON on stdout. Emulation only
 * advances when a command asks for it, so runs are deterministic.
 *
 * Optionally exposes mGBA's GDB stub on a TCP port so gdb-multiarch can attach
 * with the ELF for type-aware inspection (print, bt, info locals, ptype).
 * Breakpoints and watchpoints are shared between the stdin commands and gdb.
 *
 * Build: cc -O2 -o gbaemu gbaemu.c -lmgba   (see tools/gba-emu/Makefile)
 * Commands: see tools/gba-emu/README.md or send "help".
 */
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/serialize.h>
#include <mgba/debugger/debugger.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/debugger/gdb-stub.h>
#include <mgba/internal/gba/gba.h>
#include <mgba-util/png-io.h>
#include <mgba-util/vfs.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define GBA_W 240
#define GBA_H 160
#define VIDEO_STRIDE 256
#define MAX_READ 0x10000
#define GDB_CONTINUE_FRAMES_DEFAULT 3600

static struct mCore* core;
static struct GDBStub* stub;
static color_t* videoBuffer;
static char romPath[1024];
static int gdbPort;
static int gdbContinueFrames = GDB_CONTINUE_FRAMES_DEFAULT;
static uint32_t gdbContinueStartFrame;
static bool gdbWaitingForStop;
static uint16_t heldKeys;
static int shotScale = 1;

static void (*stubEntered)(struct mDebugger*, enum mDebuggerEntryReason, struct mDebuggerEntryInfo*);

/* Last time the debugger stopped emulation. */
static struct {
	bool hit;
	enum mDebuggerEntryReason reason;
	struct mDebuggerEntryInfo info;
} lastStop;

/* ------------------------------------------------------------------ output */

static void jsonString(FILE* f, const char* s) {
	fputc('"', f);
	for (; *s; ++s) {
		unsigned char c = (unsigned char) *s;
		if (c == '"' || c == '\\') {
			fputc('\\', f);
			fputc(c, f);
		} else if (c == '\n') {
			fputs("\\n", f);
		} else if (c < 0x20) {
			fprintf(f, "\\u%04x", c);
		} else {
			fputc(c, f);
		}
	}
	fputc('"', f);
}

/* ----------------------------------------------------------------- logging */

/* Keep the game's DebugPrintf output (mGBA debug registers, category
 * gba.debug) and emulator errors; drop the rest so stdout stays JSON. */
#define LOG_MAX 512
static char* logLines[LOG_MAX];
static int logCount;
static int logDropped;
static int debugCategory = -1;

static void hostLog(struct mLogger* logger, int category, enum mLogLevel level, const char* format, va_list args) {
	(void) logger;
	if (debugCategory < 0) {
		debugCategory = mLogCategoryById("gba.debug");
	}
	if (category != debugCategory && !(level & (mLOG_ERROR | mLOG_FATAL))) {
		return;
	}
	char msg[512];
	int n = 0;
	if (category != debugCategory) {
		n = snprintf(msg, sizeof(msg), "[%s] ", mLogCategoryName(category));
	}
	vsnprintf(msg + n, sizeof(msg) - n, format, args);
	if (logCount == LOG_MAX) {
		free(logLines[0]);
		memmove(logLines, logLines + 1, sizeof(char*) * (LOG_MAX - 1));
		--logCount;
		++logDropped;
	}
	logLines[logCount++] = strdup(msg);
}

static struct mLogger hostLogger = { .log = hostLog };

static void reply(const char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
	fputc('\n', stdout);
	fflush(stdout);
}

static void replyError(const char* fmt, ...) {
	char msg[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	for (char* c = msg; *c; ++c) {
		if (*c == '"' || *c == '\\' || *c < 0x20) {
			*c = '\'';
		}
	}
	reply("{\"ok\":false,\"error\":\"%s\"}", msg);
}

static struct ARMCore* cpu(void) {
	return core->cpu;
}

/* Address of the instruction that executes next. */
static uint32_t currentPC(void) {
	struct ARMCore* c = cpu();
	return c->gprs[ARM_PC] - (c->executionMode == MODE_THUMB ? 2 : 4);
}

static const char* reasonName(enum mDebuggerEntryReason r) {
	switch (r) {
	case DEBUGGER_ENTER_MANUAL: return "manual";
	case DEBUGGER_ENTER_ATTACHED: return "attached";
	case DEBUGGER_ENTER_BREAKPOINT: return "breakpoint";
	case DEBUGGER_ENTER_WATCHPOINT: return "watchpoint";
	case DEBUGGER_ENTER_ILLEGAL_OP: return "illegal_op";
	case DEBUGGER_ENTER_STACK: return "stack";
	}
	return "unknown";
}

/* Common fields describing where the emulator is now. */
static int stateJson(char* out, size_t len) {
	return snprintf(out, len, "\"frame\":%u,\"pc\":\"0x%08x\",\"thumb\":%s,\"lr\":\"0x%08x\",\"sp\":\"0x%08x\",\"log\":%d",
	                core->frameCounter(core), currentPC(), cpu()->executionMode == MODE_THUMB ? "true" : "false",
	                cpu()->gprs[ARM_LR], cpu()->gprs[ARM_SP], logCount);
}

/* ---------------------------------------------------------------- debugger */

static void hostEntered(struct mDebugger* d, enum mDebuggerEntryReason reason, struct mDebuggerEntryInfo* info) {
	if (reason != DEBUGGER_ENTER_ATTACHED) {
		lastStop.hit = true;
		lastStop.reason = reason;
		if (info) {
			lastStop.info = *info;
		} else {
			memset(&lastStop.info, 0, sizeof(lastStop.info));
		}
	}
	/* Only tell gdb about stops it asked for; stops during stdin "run" commands
	 * happen behind its back while it believes the target is halted. */
	if (gdbWaitingForStop || reason == DEBUGGER_ENTER_ATTACHED) {
		gdbWaitingForStop = false;
		stubEntered(d, reason, info);
	}
}

/* The BIOS Stop call (SWI 3) does nothing in mGBA unless a frontend reacts to
 * the sleep callback. Treat it like Halt: the CPU waits until an enabled
 * interrupt (the game sets the keypad one) is pending. */
static void onSleep(void* context) {
	(void) context;
	GBAHalt((struct GBA*) core->board);
}

static struct mCoreCallbacks coreCallbacks = {.sleep = onSleep};

static bool gdbConnected(void) {
	return stub && !SOCKET_FAILED(stub->connection);
}

/* Run until `frames` frames have elapsed or the debugger stops. */
static void runFrames(uint32_t frames) {
	struct mDebugger* d = &stub->d;
	uint32_t target = core->frameCounter(core) + frames;
	lastStop.hit = false;
	d->state = DEBUGGER_RUNNING;
	while (!lastStop.hit && (int32_t) (core->frameCounter(core) - target) < 0) {
		if (d->platform->hasBreakpoints(d->platform)) {
			core->step(core);
			d->platform->checkBreakpoints(d->platform);
		} else {
			core->runLoop(core);
		}
	}
	d->state = DEBUGGER_PAUSED;
}

static void stopJson(char* out, size_t len) {
	char st[256];
	stateJson(st, sizeof(st));
	if (!lastStop.hit) {
		snprintf(out, len, "{\"ok\":true,\"stop\":\"frames\",%s}", st);
		return;
	}
	const struct mDebuggerEntryInfo* i = &lastStop.info;
	switch (lastStop.reason) {
	case DEBUGGER_ENTER_BREAKPOINT:
		snprintf(out, len, "{\"ok\":true,\"stop\":\"breakpoint\",\"id\":%zd,\"address\":\"0x%08x\",%s}",
		         i->pointId, i->address, st);
		break;
	case DEBUGGER_ENTER_WATCHPOINT:
		snprintf(out, len,
		         "{\"ok\":true,\"stop\":\"watchpoint\",\"id\":%zd,\"address\":\"0x%08x\",\"access\":\"%s\","
		         "\"old\":\"0x%x\",\"new\":\"0x%x\",%s}",
		         i->pointId, i->address, i->type.wp.accessType == WATCHPOINT_READ ? "read" : "write",
		         i->type.wp.oldValue, i->type.wp.newValue, st);
		break;
	default:
		snprintf(out, len, "{\"ok\":true,\"stop\":\"%s\",\"address\":\"0x%08x\",%s}", reasonName(lastStop.reason),
		         i->address, st);
		break;
	}
}

/* ---------------------------------------------------------------- commands */

static bool needCore(void) {
	if (!core) {
		replyError("no ROM loaded; send: load <path.gba>");
		return false;
	}
	if (gdbWaitingForStop) {
		replyError("gdb has the target running; wait for it to stop or interrupt it");
		return false;
	}
	return true;
}

static void unloadCore(void) {
	if (!core) {
		return;
	}
	if (stub) {
		GDBStubShutdown(stub);
	}
	core->detachDebugger(core);
	mCoreConfigDeinit(&core->config);
	core->deinit(core);
	core = NULL;
	free(stub);
	stub = NULL;
	gdbPort = 0;
	gdbWaitingForStop = false;
}

static bool startGdb(int port) {
	if (!GDBStubListen(stub, port, NULL, GDB_WATCHPOINT_STANDARD_LOGIC)) {
		return false;
	}
	gdbPort = port;
	return true;
}

static void cmdLoad(const char* path) {
	unloadCore();
	struct mCore* c = mCoreFind(path);
	if (!c) {
		replyError("not a ROM mGBA recognises: %s", path);
		return;
	}
	c->init(c);
	mCoreInitConfig(c, NULL);
	mCoreConfigSetOverrideIntValue(&c->config, "skipBios", 1);
	mCoreConfigSetOverrideIntValue(&c->config, "audioSync", 0);
	mCoreConfigSetOverrideIntValue(&c->config, "videoSync", 0);
	mCoreLoadConfig(c);
	c->setVideoBuffer(c, videoBuffer, VIDEO_STRIDE);
	c->setAudioBufferSize(c, 2048);
	if (!mCoreLoadFile(c, path)) {
		mCoreConfigDeinit(&c->config);
		c->deinit(c);
		replyError("failed to load %s", path);
		return;
	}
	core = c;
	snprintf(romPath, sizeof(romPath), "%s", path);

	// GBAEMU_RTC=<unix seconds> starts the cartridge clock at that time and
	// advances it with emulated frames, so runs don't depend on the host clock.
	const char* rtc = getenv("GBAEMU_RTC");
	if (rtc && *rtc) {
		core->rtc.override = RTC_FAKE_EPOCH;
		core->rtc.value = strtoll(rtc, NULL, 0) * 1000LL;
	}

	stub = calloc(1, sizeof(*stub));
	GDBStubCreate(stub);
	stubEntered = stub->d.entered;
	stub->d.entered = hostEntered;
	mDebuggerAttach(&stub->d, core);
	core->addCoreCallbacks(core, &coreCallbacks);
	core->reset(core);
	stub->d.state = DEBUGGER_PAUSED;
	heldKeys = 0;
	core->setKeys(core, 0);

	char title[16] = {0}, code[16] = {0};
	core->getGameTitle(core, title);
	core->getGameCode(core, code);
	reply("{\"ok\":true,\"title\":\"%s\",\"code\":\"%s\",\"frame\":%u}", title, code, core->frameCounter(core));
}

static void cmdRun(const char* args) {
	if (!needCore()) {
		return;
	}
	long frames = 1;
	if (*args) {
		frames = strtol(args, NULL, 0);
	}
	if (frames < 1 || frames > 1000000) {
		replyError("frames must be 1..1000000");
		return;
	}
	runFrames((uint32_t) frames);
	char out[512];
	stopJson(out, sizeof(out));
	reply("%s", out);
}


/* profile <frames> <file>: run with single-stepping and write "pc cycles"
 * lines (cycles spent while that instruction ran, halted time included) to
 * <file>. Slow (a few hundred frames per second); tools/regress/profile.py
 * turns it into per-function totals. */
struct ProfEntry {
	uint32_t pc;
	uint64_t cycles;
};

static void cmdProfile(const char* args) {
	if (!needCore()) {
		return;
	}
	char path[1024];
	long frames = 0;
	if (sscanf(args, "%ld %1023s", &frames, path) != 2 || frames < 1) {
		replyError("usage: profile <frames> <file>");
		return;
	}
	const size_t size = 1u << 20;
	struct ProfEntry* tab = calloc(size, sizeof(*tab));
	struct mDebugger* d = &stub->d;
	uint32_t target = core->frameCounter(core) + (uint32_t) frames;
	d->state = DEBUGGER_RUNNING;
	while ((int32_t) (core->frameCounter(core) - target) < 0) {
		uint32_t pc = currentPC();
		uint32_t t0 = mTimingCurrentTime(&((struct GBA*) core->board)->timing);
		core->step(core);
		uint32_t dt = mTimingCurrentTime(&((struct GBA*) core->board)->timing) - t0;
		size_t h = (pc * 2654435761u) >> 12;
		for (;; h++) {
			struct ProfEntry* e = &tab[h & (size - 1)];
			if (e->pc == pc || e->pc == 0) {
				e->pc = pc;
				e->cycles += dt;
				break;
			}
		}
	}
	d->state = DEBUGGER_PAUSED;
	FILE* f = fopen(path, "w");
	if (!f) {
		free(tab);
		replyError("cannot write file");
		return;
	}
	for (size_t i = 0; i < size; i++) {
		if (tab[i].pc) {
			fprintf(f, "%08x %llu\n", tab[i].pc, (unsigned long long) tab[i].cycles);
		}
	}
	fclose(f);
	free(tab);
	reply("{\"ok\":true,\"path\":\"%s\"}", path);
}

static void cmdStep(const char* args) {
	if (!needCore()) {
		return;
	}
	long n = *args ? strtol(args, NULL, 0) : 1;
	if (n < 1 || n > 10000000) {
		replyError("count must be 1..10000000");
		return;
	}
	struct mDebugger* d = &stub->d;
	lastStop.hit = false;
	d->state = DEBUGGER_RUNNING;
	for (long i = 0; i < n && !lastStop.hit; ++i) {
		core->step(core);
		if (i + 1 < n) {
			d->platform->checkBreakpoints(d->platform);
		}
	}
	d->state = DEBUGGER_PAUSED;
	char out[512];
	if (lastStop.hit) {
		stopJson(out, sizeof(out));
	} else {
		char st[256];
		stateJson(st, sizeof(st));
		snprintf(out, sizeof(out), "{\"ok\":true,\"stop\":\"step\",%s}", st);
	}
	reply("%s", out);
}

static void cmdRegs(void) {
	if (!needCore()) {
		return;
	}
	struct ARMCore* c = cpu();
	char out[1024];
	int n = snprintf(out, sizeof(out), "{\"ok\":true,\"regs\":{");
	for (int i = 0; i < 16; ++i) {
		uint32_t v = i == ARM_PC ? currentPC() : (uint32_t) c->gprs[i];
		n += snprintf(out + n, sizeof(out) - n, "\"r%d\":\"0x%08x\",", i, v);
	}
	n += snprintf(out + n, sizeof(out) - n, "\"cpsr\":\"0x%08x\"},\"mode\":\"0x%02x\",", c->cpsr.packed, c->cpsr.priv);
	stateJson(out + n, sizeof(out) - n);
	strncat(out, "}", sizeof(out) - strlen(out) - 1);
	reply("%s", out);
}

static void cmdRead(const char* args) {
	if (!needCore()) {
		return;
	}
	char* end;
	unsigned long addr = strtoul(args, &end, 0);
	unsigned long len = strtoul(end, NULL, 0);
	if (len == 0 || len > MAX_READ) {
		replyError("length must be 1..%d", MAX_READ);
		return;
	}
	char* hex = malloc(len * 2 + 1);
	for (unsigned long i = 0; i < len; ++i) {
		uint8_t b = core->rawRead8(core, (uint32_t) (addr + i), -1);
		sprintf(hex + i * 2, "%02x", b);
	}
	hex[len * 2] = 0;
	printf("{\"ok\":true,\"address\":\"0x%08lx\",\"hex\":\"%s\"}\n", addr, hex);
	fflush(stdout);
	free(hex);
}

static int hexNibble(char c) {
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	return -1;
}

static void cmdWrite(const char* args) {
	if (!needCore()) {
		return;
	}
	char* end;
	unsigned long addr = strtoul(args, &end, 0);
	while (*end == ' ') ++end;
	size_t hl = strlen(end);
	if (hl == 0 || hl % 2) {
		replyError("write <addr> <hexbytes> (even number of hex digits)");
		return;
	}
	for (size_t i = 0; i < hl / 2; ++i) {
		int hi = hexNibble(end[i * 2]), lo = hexNibble(end[i * 2 + 1]);
		if (hi < 0 || lo < 0) {
			replyError("bad hex at byte %zu", i);
			return;
		}
		core->rawWrite8(core, (uint32_t) (addr + i), -1, (uint8_t) (hi << 4 | lo));
	}
	reply("{\"ok\":true,\"written\":%zu}", hl / 2);
}

static void cmdKeys(const char* args) {
	if (!needCore()) {
		return;
	}
	heldKeys = (uint16_t) (strtoul(args, NULL, 0) & 0x3FF);
	core->setKeys(core, heldKeys);
	reply("{\"ok\":true,\"keys\":%u}", heldKeys);
}

static void cmdBreak(const char* args) {
	if (!needCore()) {
		return;
	}
	struct mBreakpoint bp = {
		.address = (uint32_t) strtoul(args, NULL, 0) & ~1u,
		.segment = -1,
		.type = BREAKPOINT_HARDWARE,
	};
	ssize_t id = stub->d.platform->setBreakpoint(stub->d.platform, &bp);
	reply("{\"ok\":true,\"id\":%zd,\"address\":\"0x%08x\"}", id, bp.address);
}

static void cmdWatch(const char* args) {
	if (!needCore()) {
		return;
	}
	char* end;
	struct mWatchpoint wp = {
		.address = (uint32_t) strtoul(args, &end, 0),
		.segment = -1,
		.type = WATCHPOINT_WRITE,
	};
	while (*end == ' ') ++end;
	if (!strncmp(end, "rw", 2)) {
		wp.type = WATCHPOINT_RW;
	} else if (*end == 'r') {
		wp.type = WATCHPOINT_READ;
	} else if (*end == 'c') {
		wp.type = WATCHPOINT_WRITE_CHANGE;
	}
	ssize_t id = stub->d.platform->setWatchpoint(stub->d.platform, &wp);
	reply("{\"ok\":true,\"id\":%zd,\"address\":\"0x%08x\"}", id, wp.address);
}

static void cmdDelete(const char* args) {
	if (!needCore()) {
		return;
	}
	ssize_t id = strtol(args, NULL, 0);
	bool ok = stub->d.platform->clearBreakpoint(stub->d.platform, id);
	if (!ok) {
		replyError("no breakpoint or watchpoint with id %zd", id);
		return;
	}
	reply("{\"ok\":true}");
}

static void cmdList(void) {
	if (!needCore()) {
		return;
	}
	struct mBreakpointList bps;
	struct mWatchpointList wps;
	mBreakpointListInit(&bps, 0);
	mWatchpointListInit(&wps, 0);
	stub->d.platform->listBreakpoints(stub->d.platform, &bps);
	stub->d.platform->listWatchpoints(stub->d.platform, &wps);
	printf("{\"ok\":true,\"breakpoints\":[");
	for (size_t i = 0; i < mBreakpointListSize(&bps); ++i) {
		struct mBreakpoint* b = mBreakpointListGetPointer(&bps, i);
		printf("%s{\"id\":%zd,\"address\":\"0x%08x\"}", i ? "," : "", b->id, b->address);
	}
	printf("],\"watchpoints\":[");
	for (size_t i = 0; i < mWatchpointListSize(&wps); ++i) {
		struct mWatchpoint* w = mWatchpointListGetPointer(&wps, i);
		printf("%s{\"id\":%zd,\"address\":\"0x%08x\",\"type\":%d}", i ? "," : "", w->id, w->address, w->type);
	}
	printf("]}\n");
	fflush(stdout);
	mBreakpointListDeinit(&bps);
	mWatchpointListDeinit(&wps);
}

static void cmdScreenshot(const char* path) {
	if (!needCore()) {
		return;
	}
	struct VFile* vf = VFileOpen(path, O_CREAT | O_TRUNC | O_WRONLY);
	if (!vf) {
		replyError("cannot open %s: %s", path, strerror(errno));
		return;
	}
	int w = GBA_W * shotScale, h = GBA_H * shotScale;
	color_t* pixels = videoBuffer;
	int stride = VIDEO_STRIDE;
	if (shotScale > 1) {
		pixels = malloc(sizeof(color_t) * w * h);
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				pixels[y * w + x] = videoBuffer[(y / shotScale) * VIDEO_STRIDE + x / shotScale];
			}
		}
		stride = w;
	}
	png_structp png = PNGWriteOpen(vf);
	png_infop info = PNGWriteHeader(png, w, h);
	bool ok = PNGWritePixels(png, w, h, stride, pixels);
	PNGWriteClose(png, info);
	vf->close(vf);
	if (pixels != videoBuffer) {
		free(pixels);
	}
	if (!ok) {
		replyError("PNG write failed");
		return;
	}
	reply("{\"ok\":true,\"path\":\"%s\",\"width\":%d,\"height\":%d,\"frame\":%u}", path, w, h,
	      core->frameCounter(core));
}

static void cmdSaveState(const char* path) {
	if (!needCore()) {
		return;
	}
	struct VFile* vf = VFileOpen(path, O_CREAT | O_TRUNC | O_RDWR);
	if (!vf) {
		replyError("cannot open %s: %s", path, strerror(errno));
		return;
	}
	bool ok = mCoreSaveStateNamed(core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC | SAVESTATE_METADATA);
	vf->close(vf);
	if (!ok) {
		replyError("save state failed");
		return;
	}
	reply("{\"ok\":true,\"path\":\"%s\",\"frame\":%u}", path, core->frameCounter(core));
}

static void cmdLoadState(const char* path) {
	if (!needCore()) {
		return;
	}
	struct VFile* vf = VFileOpen(path, O_RDONLY);
	if (!vf) {
		replyError("cannot open %s: %s", path, strerror(errno));
		return;
	}
	bool ok = mCoreLoadStateNamed(core, vf, SAVESTATE_SAVEDATA | SAVESTATE_RTC);
	vf->close(vf);
	if (!ok) {
		replyError("load state failed (state from a different ROM build?)");
		return;
	}
	core->setKeys(core, heldKeys);
	char st[256];
	stateJson(st, sizeof(st));
	reply("{\"ok\":true,%s}", st);
}

/* Load a battery save (.sav) as an in-memory copy; the file is never written. */
static void cmdLoadSav(const char* path) {
	if (!needCore()) {
		return;
	}
	struct VFile* vf = VFileOpen(path, O_RDONLY);
	if (!vf) {
		replyError("cannot open %s: %s", path, strerror(errno));
		return;
	}
	ssize_t size = vf->size(vf);
	void* data = malloc(size > 0 ? size : 1);
	ssize_t got = vf->read(vf, data, size);
	vf->close(vf);
	if (got != size) {
		free(data);
		replyError("short read on %s", path);
		return;
	}
	bool ok = core->savedataRestore(core, data, size, false);
	free(data);
	if (!ok) {
		replyError("savedata restore failed");
		return;
	}
	core->reset(core);
	reply("{\"ok\":true,\"bytes\":%zd,\"note\":\"core reset with save loaded\"}", size);
}

/* Write the battery save (flash/SRAM contents) to a .sav file. */
static void cmdDumpSav(const char* path) {
	if (!needCore()) {
		return;
	}
	void* data = NULL;
	size_t size = core->savedataClone(core, &data);
	if (!size || !data) {
		free(data);
		replyError("no save data");
		return;
	}
	FILE* f = fopen(path, "wb");
	if (!f) {
		free(data);
		replyError("cannot open %s: %s", path, strerror(errno));
		return;
	}
	size_t wrote = fwrite(data, 1, size, f);
	fclose(f);
	free(data);
	if (wrote != size) {
		replyError("short write on %s", path);
		return;
	}
	reply("{\"ok\":true,\"path\":\"%s\",\"bytes\":%zu}", path, size);
}

static void cmdReset(void) {
	if (!needCore()) {
		return;
	}
	core->reset(core);
	stub->d.state = DEBUGGER_PAUSED;
	core->setKeys(core, heldKeys);
	reply("{\"ok\":true,\"frame\":%u}", core->frameCounter(core));
}

static void cmdGdb(const char* args) {
	if (!needCore()) {
		return;
	}
	int port = *args ? atoi(args) : 2345;
	if (gdbPort) {
		reply("{\"ok\":true,\"port\":%d,\"connected\":%s}", gdbPort, gdbConnected() ? "true" : "false");
		return;
	}
	if (!startGdb(port)) {
		replyError("cannot listen on port %d", port);
		return;
	}
	reply("{\"ok\":true,\"port\":%d}", port);
}

static void cmdInfo(void) {
	if (!core) {
		reply("{\"ok\":true,\"loaded\":false}");
		return;
	}
	char st[256];
	stateJson(st, sizeof(st));
	reply("{\"ok\":true,\"loaded\":true,\"rom\":\"%s\",\"keys\":%u,\"gdb_port\":%d,\"gdb_connected\":%s,"
	      "\"gdb_running\":%s,%s}",
	      romPath, heldKeys, gdbPort, gdbConnected() ? "true" : "false", gdbWaitingForStop ? "true" : "false", st);
}

static void cmdLog(void) {
	printf("{\"ok\":true,\"dropped\":%d,\"lines\":[", logDropped);
	for (int i = 0; i < logCount; ++i) {
		if (i) fputc(',', stdout);
		jsonString(stdout, logLines[i]);
		free(logLines[i]);
	}
	printf("]}\n");
	fflush(stdout);
	logCount = 0;
	logDropped = 0;
}

static void cmdHelp(void) {
	reply("{\"ok\":true,\"commands\":\"load <rom> | reset | loadsav <file> | dumpsav <file> | run [frames] | step [n] | regs | "
	      "read <addr> <len> | write <addr> <hex> | keys <mask> | break <addr> | watch <addr> [w|r|rw|c] | "
	      "profile <frames> <file> | delete <id> | list | screenshot <png> | savestate <file> | loadstate <file> | gdb [port] | "
	      "gdbframes <n> | scale <1-4> | log | info | quit\"}");
}

static bool dispatch(char* line) {
	while (*line == ' ' || *line == '\t') ++line;
	size_t l = strlen(line);
	while (l && (line[l - 1] == '\r' || line[l - 1] == ' ')) line[--l] = 0;
	if (!*line) {
		return true;
	}
	char* args = strchr(line, ' ');
	if (args) {
		*args++ = 0;
		while (*args == ' ') ++args;
	} else {
		args = line + l;
	}
	if (!strcmp(line, "quit")) {
		reply("{\"ok\":true}");
		return false;
	}
	if (!strcmp(line, "load")) cmdLoad(args);
	else if (!strcmp(line, "reset")) cmdReset();
	else if (!strcmp(line, "loadsav")) cmdLoadSav(args);
	else if (!strcmp(line, "dumpsav")) cmdDumpSav(args);
	else if (!strcmp(line, "run")) cmdRun(args);
	else if (!strcmp(line, "step")) cmdStep(args);
	else if (!strcmp(line, "profile")) cmdProfile(args);
	else if (!strcmp(line, "regs")) cmdRegs();
	else if (!strcmp(line, "read")) cmdRead(args);
	else if (!strcmp(line, "write")) cmdWrite(args);
	else if (!strcmp(line, "keys")) cmdKeys(args);
	else if (!strcmp(line, "break")) cmdBreak(args);
	else if (!strcmp(line, "watch")) cmdWatch(args);
	else if (!strcmp(line, "delete")) cmdDelete(args);
	else if (!strcmp(line, "list")) cmdList();
	else if (!strcmp(line, "screenshot")) cmdScreenshot(args);
	else if (!strcmp(line, "savestate")) cmdSaveState(args);
	else if (!strcmp(line, "loadstate")) cmdLoadState(args);
	else if (!strcmp(line, "gdb")) cmdGdb(args);
	else if (!strcmp(line, "gdbframes")) {
		gdbContinueFrames = atoi(args) > 0 ? atoi(args) : GDB_CONTINUE_FRAMES_DEFAULT;
		reply("{\"ok\":true,\"gdbframes\":%d}", gdbContinueFrames);
	}
	else if (!strcmp(line, "info")) cmdInfo();
	else if (!strcmp(line, "log")) cmdLog();
	else if (!strcmp(line, "scale")) {
		int sc = atoi(args);
		shotScale = sc >= 1 && sc <= 4 ? sc : 1;
		reply("{\"ok\":true,\"scale\":%d}", shotScale);
	}
	else if (!strcmp(line, "help")) cmdHelp();
	else replyError("unknown command '%s' (send help)", line);
	return true;
}

/* ---------------------------------------------------------------- gdb loop */

/* gdb sent continue: run until a breakpoint, an interrupt from gdb, or the
 * frame cap, in slices so stdin is still serviced. */
static void serviceGdbRunning(void) {
	struct mDebugger* d = &stub->d;
	if (!gdbWaitingForStop) {
		gdbWaitingForStop = true;
		gdbContinueStartFrame = core->frameCounter(core);
	}
	uint32_t frame = core->frameCounter(core);
	while (d->state == DEBUGGER_CALLBACK && core->frameCounter(core) == frame) {
		mDebuggerRun(d);
	}
	if (d->state == DEBUGGER_CALLBACK &&
	    core->frameCounter(core) - gdbContinueStartFrame >= (uint32_t) gdbContinueFrames) {
		mDebuggerEnter(d, DEBUGGER_ENTER_MANUAL, NULL);
	}
	if (d->state != DEBUGGER_CALLBACK) {
		gdbWaitingForStop = false;
		d->state = DEBUGGER_PAUSED;
	}
}

int main(int argc, char** argv) {
	videoBuffer = calloc(VIDEO_STRIDE * GBA_H, sizeof(color_t));
	mLogSetDefaultLogger(&hostLogger);

	int argi = 1;
	int wantGdb = 0;
	for (; argi < argc; ++argi) {
		if (!strcmp(argv[argi], "--gdb") && argi + 1 < argc) {
			wantGdb = atoi(argv[++argi]);
		} else {
			break;
		}
	}
	if (argi < argc) {
		cmdLoad(argv[argi]);
		if (core && wantGdb && !startGdb(wantGdb)) {
			replyError("cannot listen on port %d", wantGdb);
		}
	}

	char buf[8192];
	size_t have = 0;
	bool running = true;
	while (running) {
		bool gdbBusy = stub && stub->d.state == DEBUGGER_CALLBACK;
		struct pollfd fds[3];
		int nfds = 0;
		fds[nfds++] = (struct pollfd) { .fd = STDIN_FILENO, .events = POLLIN };
		if (gdbPort && stub) {
			if (!SOCKET_FAILED(stub->socket)) fds[nfds++] = (struct pollfd) { .fd = stub->socket, .events = POLLIN };
			if (!SOCKET_FAILED(stub->connection)) fds[nfds++] = (struct pollfd) { .fd = stub->connection, .events = POLLIN };
		}
		int pr = poll(fds, nfds, gdbBusy ? 0 : 200);
		if (pr < 0 && errno != EINTR) {
			break;
		}

		if (fds[0].revents & (POLLIN | POLLHUP)) {
			ssize_t r = read(STDIN_FILENO, buf + have, sizeof(buf) - 1 - have);
			if (r <= 0) {
				break;
			}
			have += r;
			buf[have] = 0;
			char* start = buf;
			char* nl;
			while (running && (nl = strchr(start, '\n'))) {
				*nl = 0;
				running = dispatch(start);
				start = nl + 1;
			}
			have = strlen(start);
			memmove(buf, start, have + 1);
			if (have == sizeof(buf) - 1) {
				replyError("command line too long");
				have = 0;
			}
		}

		if (!running || !core || !gdbPort) {
			continue;
		}
		if (stub->d.state == DEBUGGER_CALLBACK) {
			serviceGdbRunning();
		} else {
			stub->shouldBlock = false;
			GDBStubUpdate(stub);
			/* The stub drops to RUNNING when gdb detaches; we only run on request. */
			if (stub->d.state == DEBUGGER_RUNNING) {
				stub->d.state = DEBUGGER_PAUSED;
			}
		}
	}
	unloadCore();
	free(videoBuffer);
	return 0;
}
