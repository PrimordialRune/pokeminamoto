#ifndef GUARD_REGRESS_H
#define GUARD_REGRESS_H

// Test hooks for tools/regress, the headless regression run. They are only
// built with `make REGRESS=1`; every other build compiles the macros below
// to nothing, so normal and release ROMs are unchanged.

#ifdef REGRESS_HOOKS

enum
{
    REGRESS_CMD_NONE,
    REGRESS_CMD_SETUP,   // fixed party, PC mons, items, dex, warp to Oldale
    REGRESS_CMD_WILD,    // wild battle
    REGRESS_CMD_TRAINER, // trainer battle
    REGRESS_CMD_PC,      // open the Pokémon Storage System menu
    REGRESS_CMD_DUMP,    // write the game state into gRegress.dump
    REGRESS_CMD_HEAL,
};

#define REGRESS_MAGIC 0x53524752 // "RGRS"
#define REGRESS_DUMP_WORDS 1024

struct RegressState
{
    u32 magic;
    u8 cmd;          // written by the harness, cleared when run
    u8 cmdDone;      // counts finished commands
    u8 fieldReady;   // player can move and no script runs
    u8 unused;
    // Written by the hooks; the harness zeroes them to start a new scene.
    u32 heapPeak;    // highest heap offset ever handed out
    u32 heapFails;   // Alloc calls that returned NULL
    u16 dmaPeak;     // most DMA3 requests queued at once
    u16 dmaFull;     // requests dropped because the queue was full
    u32 frames;      // main loop iterations
    u32 lagFrames;   // VBlanks the main loop missed
    u32 busyLinesSum;
    u16 busyLinesMax; // scanlines of a frame the main loop used (228 per frame)
    u16 dumpWords;
    u32 lastVblank;
    u16 dump[REGRESS_DUMP_WORDS];
};

extern struct RegressState gRegress;

void Regress_NoteAlloc(void *heapStart, void *ptr, u32 size);
void Regress_NoteDmaQueue(u32 pending);
void Regress_NoteDmaFull(void);
void Regress_FrameEnd(void);
void Regress_OverworldTick(void);
bool8 Regress_ScriptIdle(void);

#define REGRESS_NOTE_ALLOC(start, ptr, size) Regress_NoteAlloc(start, ptr, size)
#define REGRESS_NOTE_DMA_QUEUE(pending) Regress_NoteDmaQueue(pending)
#define REGRESS_NOTE_DMA_FULL() Regress_NoteDmaFull()
#define REGRESS_FRAME_END() Regress_FrameEnd()
#define REGRESS_OVERWORLD_TICK() Regress_OverworldTick()

#else

#define REGRESS_NOTE_ALLOC(start, ptr, size)
#define REGRESS_NOTE_DMA_QUEUE(pending)
#define REGRESS_NOTE_DMA_FULL()
#define REGRESS_FRAME_END()
#define REGRESS_OVERWORLD_TICK()

#endif // REGRESS_HOOKS

#endif // GUARD_REGRESS_H
