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
    REGRESS_CMD_TV,      // run the TV script as if the player checked a TV
    REGRESS_CMD_CABLE_CLUB, // warp in front of the Cable Club attendant in Oldale's Pokémon Center 2F
    REGRESS_CMD_SCENE,   // open the screen gRegress.arg names (REGRESS_SCENE_*)
};

// Screens the main run doesn't reach, for measuring heap use
enum
{
    REGRESS_SCENE_EVOLUTION = 1, // party slot 2 (Pikachu) evolves into Raichu
    REGRESS_SCENE_NAMING,        // nickname screen
    REGRESS_SCENE_EGG_HATCH,     // a Pikachu egg in slot 6 hatches
    REGRESS_SCENE_INGAME_TRADE,  // in-game trade 0, giving party slot 2
    REGRESS_SCENE_HALL_OF_FAME,  // Hall of Fame (not saved), then the credits
    REGRESS_SCENE_DOUBLE_BATTLE, // double battle against TRAINER_GABBY_AND_TY_1
    REGRESS_SCENE_SUBSTITUTE,    // wild battle with Substitute as party slot 1's first move
    REGRESS_SCENE_BEDROOM_PC,    // the player's own PC (item storage)
    REGRESS_SCENE_POKENAV,       // the PokéNav
    REGRESS_SCENE_SAVE_FAILED,   // save-failed screen after a normal save, back to the field
    REGRESS_SCENE_SAVE_FAILED_HOF, // the same for a Hall of Fame save of a test pattern
    REGRESS_SCENE_LOAD_HOF,      // read the Hall of Fame sectors back into gDecompressionBuffer
    REGRESS_SCENE_LINK_FULL_SAVE,  // the LinkFullSave_* steps Task_LinkFullSave runs, without a link
    REGRESS_SCENE_SAVE_BLOCKS,     // WriteSaveBlock2, then WriteSaveBlock1Sector until it is done
    REGRESS_SCENE_SAVE_LINK,       // TrySavingData(SAVE_LINK)
};

#define REGRESS_MAGIC 0x53524752 // "RGRS"
#define REGRESS_DUMP_WORDS 1024

struct RegressState
{
    u32 magic;
    u8 cmd;          // written by the harness, cleared when run
    u8 cmdDone;      // counts finished commands
    u8 fieldReady;   // player can move and no script runs
    u8 arg;          // argument for the command, written before cmd
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
    // Resource limits, appended after the dump (tools/regress reads them at
    // offset 40 + 2 * REGRESS_DUMP_WORDS). The harness zeroes them per scene.
    // The two "Min" fields store the value + 1, so that 0 means "never sampled".
    u16 tileHigh;        // highest OBJ tile index + 1 ever handed out by AllocSpriteTiles
    u16 tileFreeRunMin;  // smallest "largest free run of OBJ tiles" sampled, plus 1
    u16 palFreeMin;      // fewest free OBJ palette slots sampled, plus 1
    u16 spritesMax;      // most gSprites in use
    u16 objEventsMax;    // most active object events
    u16 spriteFails;     // CreateSprite* calls that returned MAX_SPRITES
    u16 paletteFails;    // LoadSpritePalette/AllocSpritePalette with no free slot
    u16 tileFails;       // AllocSpriteTiles calls that found no room
    u16 windowFails;     // AddWindow* calls that returned WINDOW_NONE
    u16 pad;
    const u8 *tileBitmap;   // sSpriteTileAllocBitmap, set by ResetSpriteData
    const u16 *palTags;     // sSpritePaletteTags
    // Appended after the limits; run.py reads it by offset (40 + 2 * REGRESS_DUMP_WORDS + 28).
    u32 stackLowest; // lowest IWRAM address the system stack has touched (0 until measured)
    // Heap errors, after stackLowest (run.py: offset 40 + 2 * REGRESS_DUMP_WORDS + 32). Only ever append.
    u32 heapFatal;       // HEAP_FATAL_* that stopped the game, 0 if none
    u32 heapFatalValue;  // the size asked for, or the pointer freed
    u32 heapFatalCaller; // address of the call
    // Save code (src/save.c), counted from when the harness zeroes them
    u16 saveFlashOps;     // flash writes: whole sectors, commit records, commit bytes
    u16 saveSectorWrites; // whole sectors erased and programmed (game data and Hall of Fame)
    // Power-cut test (tools/regress/save_check.py): the harness sets saveCutAt
    // to n and the n-th flash write never finishes, the CPU stops instead
    u16 saveCutAt;        // 0: never
    u8 saveCutHalf;       // TRUE: that write gets half of its bytes programmed first
    u8 saveCutDone;       // set when the power was "cut"
    u8 battleTransition; // nonzero: wild and trainer battles use B_TRANSITION_* (value - 1)
};

enum
{
    REGRESS_FAIL_SPRITE,
    REGRESS_FAIL_PALETTE,
    REGRESS_FAIL_TILES,
    REGRESS_FAIL_WINDOW,
};

extern struct RegressState gRegress;

void Regress_NoteAlloc(void *heapStart, void *ptr, u32 size);
void Regress_NoteDmaQueue(u32 pending);
void Regress_NoteDmaFull(void);
void Regress_FrameEnd(void);
void Regress_OverworldTick(void);
bool8 Regress_ScriptIdle(void);
void Regress_Reseed(void);
void Regress_StackInit(void);
void Regress_NoteFail(u32 kind);
void Regress_NoteTiles(u32 start, u32 count);
void Regress_SpriteStatics(const u8 *tileBitmap, const u16 *palTags);
void Regress_NoteHeapFatal(u32 kind, u32 value, u32 caller);
void Regress_SaveFlashOp(u16 sector, const u8 *data, u32 size, bool32 wholeSector);

#define REGRESS_NOTE_ALLOC(start, ptr, size) Regress_NoteAlloc(start, ptr, size)
#define REGRESS_NOTE_DMA_QUEUE(pending) Regress_NoteDmaQueue(pending)
#define REGRESS_NOTE_DMA_FULL() Regress_NoteDmaFull()
#define REGRESS_FRAME_END() Regress_FrameEnd()
#define REGRESS_OVERWORLD_TICK() Regress_OverworldTick()
#define REGRESS_RESEED() Regress_Reseed()
#define REGRESS_STACK_INIT() Regress_StackInit()
// Evaluates to `value`, counting a failure of the given kind on the way
#define REGRESS_NOTE_FAIL(kind, value) (Regress_NoteFail(kind), (value))
#define REGRESS_NOTE_TILES(start, count) Regress_NoteTiles(start, count)
#define REGRESS_SPRITE_STATICS(bitmap, tags) Regress_SpriteStatics(bitmap, tags)
#define REGRESS_NOTE_HEAP_FATAL(kind, value, caller) Regress_NoteHeapFatal(kind, value, caller)
#define REGRESS_SAVE_FLASH_OP(sector, data, size, wholeSector) Regress_SaveFlashOp(sector, data, size, wholeSector)

#else

#define REGRESS_NOTE_ALLOC(start, ptr, size)
#define REGRESS_NOTE_DMA_QUEUE(pending)
#define REGRESS_NOTE_DMA_FULL()
#define REGRESS_FRAME_END()
#define REGRESS_OVERWORLD_TICK()
#define REGRESS_RESEED()
#define REGRESS_STACK_INIT()
#define REGRESS_NOTE_FAIL(kind, value) (value)
#define REGRESS_NOTE_TILES(start, count)
#define REGRESS_SPRITE_STATICS(bitmap, tags)
#define REGRESS_NOTE_HEAP_FATAL(kind, value, caller)
#define REGRESS_SAVE_FLASH_OP(sector, data, size, wholeSector)

#endif // REGRESS_HOOKS

#endif // GUARD_REGRESS_H
