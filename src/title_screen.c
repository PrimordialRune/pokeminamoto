#include "global.h"
#include "bg.h"
#include "clear_save_data_menu.h"
#include "decompress.h"
#include "event_data.h"
#include "gpu_regs.h"
#include "intro.h"
#include "m4a.h"
#include "main.h"
#include "main_menu.h"
#include "menu.h"
#include "palette.h"
#include "reset_rtc_screen.h"
#include "sound.h"
#include "sprite.h"
#include "task.h"
#include "title_screen.h"
#include "constants/rgb.h"
#include "constants/songs.h"

/*
    Minamoto's title screen: a page of the designer's notebook. Paper with brown
    zigzag bands (BG3), a pencil sketch that draws itself (BG1, revealed through
    WIN0/WIN1), then Omar's ポケットモンスター源 logo stamps on with a white flash
    (BG0, 8bpp) and PRESS START blinks in the bottom band (BG2). The sketch keeps a
    2-frame line boil (two tile sets and tilemaps, swapped every BOIL_FRAMES), and
    every PAGE_HOLD frames the logo lifts off, the sketch is erased, the next page
    draws itself, stays alone for SCENE_ALONE frames and the logo stamps on again.
    While the logo is up the sketch fades to ghost lines (sSketchGhost_Pal) so the
    drawing stays behind the logo instead of clashing.
    With TITLE_SKETCH_CORNER the sketchbook pages are small vignettes in the
    bottom-right corner, clear of the logo: the logo stays put, the lines keep
    full strength and the pencil only sweeps from SKETCH_LEFT.

    TITLE_STYLE picks the notebook:
    - SKETCHBOOK: ruled paper; each page is one of the 1990 Capsule Monsters
      sketches (tools/intro_devamp/sketch_pages.py), drawn left to right.
    - FLIPBOOK: graph-paper model sheets; three creatures per page, drawn one at a
      time, top to bottom.
    Art comes from tools/intro_devamp/title_minamoto.py (sources in
    tools/intro_devamp/src/title/) and title_logo.py. The scene state lives in the
    main task's data: no EWRAM.
*/

#define TITLE_STYLE_SKETCHBOOK 0
#define TITLE_STYLE_FLIPBOOK   1
#ifndef TITLE_STYLE
#define TITLE_STYLE TITLE_STYLE_SKETCHBOOK
#endif

// Sketchbook pages are corner vignettes (sketch_pages.py --corner) rather than
// full scenes under the logo.
#ifndef TITLE_SKETCH_CORNER
#define TITLE_SKETCH_CORNER FALSE
#endif
#if TITLE_SKETCH_CORNER
#define SKETCH_LEFT 142   // CORNER_RIGHT - CORNER_W in sketch_pages.py
#else
#define SKETCH_LEFT 0
#endif

#define CLEAR_SAVE_BUTTON_COMBO (B_BUTTON | SELECT_BUTTON | DPAD_UP)
#define RESET_RTC_BUTTON_COMBO  (B_BUTTON | SELECT_BUTTON | DPAD_LEFT)

// Frames before the title screen gives up and replays the intro (FRLG: 45 s);
// long enough to show every sketchbook page once.
#define TITLE_SCREEN_TIMEOUT 3400
// Frames a finished page stays under the logo before it is erased for the next one.
#define PAGE_HOLD   360
// Frames a freshly drawn page is shown without the logo (the first one: FIRST_ALONE).
#define SCENE_ALONE 150
#define FIRST_ALONE 40
// Frames between the two line-boil tilemaps.
#define BOIL_FRAMES 8

// Number of 16-colour palettes the 8bpp logo may use (BG palettes 0-12).
#define NUM_LOGO_PALS 13
#define PAL_SKETCH 13
#define PAL_PAPER  14
#define PAL_TEXT   15

#define LOGO_PALS ((1 << NUM_LOGO_PALS) - 1)

// VRAM: the sketch's boil frames have their own tiles (char blocks 1 and 2) and
// tilemaps (screen blocks 30 and 29); the paper and the text share char block 3.
#define SKETCH_CHAR_A 1
#define SKETCH_CHAR_B 2
#define SKETCH_MAP_A 30
#define SKETCH_MAP_B 29
#define BG1CNT_SKETCH(chr, map) (BGCNT_PRIORITY(2) | BGCNT_CHARBASE(chr) | BGCNT_SCREENBASE(map) | BGCNT_16COLOR | BGCNT_TXT256x256)

// Flipbook: three slots of this width; each creature is drawn from SLOT_TOP down
// to the bottom band, SLOT_SPEED rows a frame, with a short pause between them.
#define SLOT_WIDTH  80
#define SLOT_TOP    92
#define SLOT_BOTTOM 142
#define SLOT_SPEED  2
#define SLOT_PAUSE  12

enum TitleScreenScene
{
    TITLESCREENSCENE_INIT = 0,
    TITLESCREENSCENE_DRAW,
    TITLESCREENSCENE_STAMP,
    TITLESCREENSCENE_RUN,
    TITLESCREENSCENE_RESTART,
    TITLESCREENSCENE_CONFIRM
};

struct TitlePage
{
    const u32 *gfx[2];
    const u32 *map[2];
};

static void ResetGpuRegs(void);
static void CB2_TitleScreenRun(void);
static void VBlankCB(void);
static void Task_TitleScreenTimer(u8 taskId);
static void Task_TitleScreenMain(u8 taskId);
static void SetTitleScreenScene(s16 *data, u8 sceneNum);
static void SetTitleScreenScene_Init(s16 *data);
static void SetTitleScreenScene_Draw(s16 *data);
static void SetTitleScreenScene_Stamp(s16 *data);
static void SetTitleScreenScene_Run(s16 *data);
static void SetTitleScreenScene_Restart(s16 *data);
static void SetTitleScreenScene_Confirm(s16 *data);
static void Task_TitleScreen_BlinkPressStart(u8 taskId);
static void SignalEndTitleScreenPaletteSomethingTask(void);
static void CB2_FadeOutTransitionToSaveClearScreen(void);
static void CB2_FadeOutTransitionToResetRtcScreen(void);
static void CB2_GoToMainMenu(void);
static u8 CreateSlashSprite(void);
static void DeactivateSlashSprite(u8 spriteId);
static bool32 IsSlashSpriteDeactivated(u8 spriteId);
static void SpriteCallback_Slash(struct Sprite *sprite);

static const u16 sGameTitleLogo_Pal[] = INCBIN_U16("graphics/title_screen/minamoto/game_title_logo.gbapal");
static const u32 sGameTitleLogo_Gfx[] = INCBIN_U32("graphics/title_screen/minamoto/game_title_logo.8bpp.lz");
static const u32 sGameTitleLogo_Map[] = INCBIN_U32("graphics/title_screen/minamoto/game_title_logo.bin.lz");
static const u16 sSlash_Pal[]         = INCBIN_U16("graphics/title_screen/minamoto/slash.gbapal");
static const u32 sSlash_Gfx[]         = INCBIN_U32("graphics/title_screen/minamoto/slash.4bpp.lz");

#if TITLE_STYLE == TITLE_STYLE_FLIPBOOK
static const u16 sPaper_Pal[]    = INCBIN_U16("graphics/title_screen/minamoto/paper_flipbook.gbapal");
static const u32 sPaper_Gfx[]    = INCBIN_U32("graphics/title_screen/minamoto/paper_flipbook.4bpp.lz");
static const u32 sPaper_Map[]    = INCBIN_U32("graphics/title_screen/minamoto/paper_flipbook.bin.lz");
static const u16 sText_Pal[]     = INCBIN_U16("graphics/title_screen/minamoto/press_start_flipbook.gbapal");
static const u32 sText_Map[]     = INCBIN_U32("graphics/title_screen/minamoto/press_start_flipbook.bin.lz");
static const u16 sSketch_Pal[]   = INCBIN_U16("graphics/title_screen/minamoto/sketch_flipbook.gbapal");
static const u16 sSketchGhost_Pal[] = INCBIN_U16("graphics/title_screen/minamoto/sketch_flipbook_ghost.gbapal");
static const u32 sPage1_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_1_a.4bpp.lz");
static const u32 sPage1_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_1_a.bin.lz");
static const u32 sPage1_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_1_b.4bpp.lz");
static const u32 sPage1_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_1_b.bin.lz");
static const u32 sPage2_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_2_a.4bpp.lz");
static const u32 sPage2_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_2_a.bin.lz");
static const u32 sPage2_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_2_b.4bpp.lz");
static const u32 sPage2_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_flipbook_2_b.bin.lz");

static const struct TitlePage sPages[] = {
    {{sPage1_GfxA, sPage1_GfxB}, {sPage1_MapA, sPage1_MapB}},
    {{sPage2_GfxA, sPage2_GfxB}, {sPage2_MapA, sPage2_MapB}},
};
#else
static const u16 sPaper_Pal[]    = INCBIN_U16("graphics/title_screen/minamoto/paper_sketchbook.gbapal");
static const u32 sPaper_Gfx[]    = INCBIN_U32("graphics/title_screen/minamoto/paper_sketchbook.4bpp.lz");
static const u32 sPaper_Map[]    = INCBIN_U32("graphics/title_screen/minamoto/paper_sketchbook.bin.lz");
static const u16 sText_Pal[]     = INCBIN_U16("graphics/title_screen/minamoto/press_start_sketchbook.gbapal");
static const u32 sText_Map[]     = INCBIN_U32("graphics/title_screen/minamoto/press_start_sketchbook.bin.lz");
static const u16 sSketch_Pal[]   = INCBIN_U16("graphics/title_screen/minamoto/sketch_sketchbook.gbapal");
static const u16 sSketchGhost_Pal[] = INCBIN_U16("graphics/title_screen/minamoto/sketch_sketchbook_ghost.gbapal");
static const u32 sPage1_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_1_a.4bpp.lz");
static const u32 sPage1_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_1_a.bin.lz");
static const u32 sPage1_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_1_b.4bpp.lz");
static const u32 sPage1_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_1_b.bin.lz");
static const u32 sPage2_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_2_a.4bpp.lz");
static const u32 sPage2_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_2_a.bin.lz");
static const u32 sPage2_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_2_b.4bpp.lz");
static const u32 sPage2_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_2_b.bin.lz");
static const u32 sPage3_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_3_a.4bpp.lz");
static const u32 sPage3_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_3_a.bin.lz");
static const u32 sPage3_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_3_b.4bpp.lz");
static const u32 sPage3_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_3_b.bin.lz");
static const u32 sPage4_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_4_a.4bpp.lz");
static const u32 sPage4_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_4_a.bin.lz");
static const u32 sPage4_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_4_b.4bpp.lz");
static const u32 sPage4_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_4_b.bin.lz");
static const u32 sPage5_GfxA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_5_a.4bpp.lz");
static const u32 sPage5_MapA[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_5_a.bin.lz");
static const u32 sPage5_GfxB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_5_b.4bpp.lz");
static const u32 sPage5_MapB[]  = INCBIN_U32("graphics/title_screen/minamoto/sketch_sketchbook_5_b.bin.lz");

static const struct TitlePage sPages[] = {
    {{sPage1_GfxA, sPage1_GfxB}, {sPage1_MapA, sPage1_MapB}},
    {{sPage2_GfxA, sPage2_GfxB}, {sPage2_MapA, sPage2_MapB}},
    {{sPage3_GfxA, sPage3_GfxB}, {sPage3_MapA, sPage3_MapB}},
    {{sPage4_GfxA, sPage4_GfxB}, {sPage4_MapA, sPage4_MapB}},
    {{sPage5_GfxA, sPage5_GfxB}, {sPage5_MapA, sPage5_MapB}},
};
#endif

static const struct OamData sOamData_SlashSprite = {
    .objMode = ST_OAM_OBJ_WINDOW,
    .shape = ST_OAM_SQUARE,
    .size = ST_OAM_SIZE_3,
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0
};

enum {
    TILE_TAG_SLASH = 3,
};

enum {
    PAL_TAG_SLASH = 2,
};

static const struct SpriteTemplate sSlashSpriteTemplate = {
    .tileTag = TILE_TAG_SLASH,
    .paletteTag = PAL_TAG_SLASH,
    .oam = &sOamData_SlashSprite,
    .anims = gDummySpriteAnimTable,
    .images = NULL,
    .affineAnims = gDummySpriteAffineAnimTable,
    .callback = SpriteCallbackDummy
};

static const struct CompressedSpriteSheet sSlashSheet = {sSlash_Gfx, 0x800, TILE_TAG_SLASH};
static const struct SpritePalette sSlashPal = {sSlash_Pal, PAL_TAG_SLASH};

static const struct BgTemplate sBgTemplates[] = {
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 1, // 8bpp
        .priority = 0,
        .baseTile = 0
    }, {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = SKETCH_MAP_A,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    }, {
        // shares the paper's tiles: its tilemap points past them
        .bg = 2,
        .charBaseIndex = 3,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    }, {
        // the paper's and text's tiles must end before screen block 27 (192 tiles)
        .bg = 3,
        .charBaseIndex = 3,
        .mapBaseIndex = 27,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 3,
        .baseTile = 0
    }
};

static void (*const sSceneFuncs[])(s16 *data) = {
    [TITLESCREENSCENE_INIT]    = SetTitleScreenScene_Init,
    [TITLESCREENSCENE_DRAW]    = SetTitleScreenScene_Draw,
    [TITLESCREENSCENE_STAMP]   = SetTitleScreenScene_Stamp,
    [TITLESCREENSCENE_RUN]     = SetTitleScreenScene_Run,
    [TITLESCREENSCENE_RESTART] = SetTitleScreenScene_Restart,
    [TITLESCREENSCENE_CONFIRM] = SetTitleScreenScene_Confirm
};

static void LoadPage(u32 page)
{
    LZ77UnCompVram(sPages[page].gfx[0], (void *)BG_CHAR_ADDR(SKETCH_CHAR_A));
    LZ77UnCompVram(sPages[page].gfx[1], (void *)BG_CHAR_ADDR(SKETCH_CHAR_B));
    LZ77UnCompVram(sPages[page].map[0], (void *)BG_SCREEN_ADDR(SKETCH_MAP_A));
    LZ77UnCompVram(sPages[page].map[1], (void *)BG_SCREEN_ADDR(SKETCH_MAP_B));
}

void CB2_InitTitleScreen(void)
{
    switch (gMain.state)
    {
    default:
        gMain.state = 0;
        // fallthrough
    case 0:
        SetVBlankCallback(NULL);
        StartTimer1();
        ResetTasks();
        ResetSpriteData();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetGpuRegs();
        ResetTempTileDataBuffers();
        DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
        DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
        DmaFill16(3, 0, (void *)PLTT, PLTT_SIZE);
        ResetBgsAndClearDma3BusyFlags(FALSE);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJ_1D_MAP | DISPCNT_OBJ_ON);
        break;
    case 1:
        LoadPalette(sGameTitleLogo_Pal, BG_PLTT_ID(0), NUM_LOGO_PALS * PLTT_SIZE_4BPP);
        DecompressAndCopyTileDataToVram(0, sGameTitleLogo_Gfx, 0, 0, 0);
        DecompressAndCopyTileDataToVram(0, sGameTitleLogo_Map, 0, 0, 1);
        LoadPalette(sSketch_Pal, BG_PLTT_ID(PAL_SKETCH), PLTT_SIZE_4BPP);
        LoadPage(0);
        LoadPalette(sText_Pal, BG_PLTT_ID(PAL_TEXT), PLTT_SIZE_4BPP);
        DecompressAndCopyTileDataToVram(2, sText_Map, 0, 0, 1);
        LoadPalette(sPaper_Pal, BG_PLTT_ID(PAL_PAPER), PLTT_SIZE_4BPP);
        DecompressAndCopyTileDataToVram(3, sPaper_Gfx, 0, 0, 0);
        DecompressAndCopyTileDataToVram(3, sPaper_Map, 0, 0, 1);
        LoadCompressedSpriteSheet(&sSlashSheet);
        LoadSpritePalette(&sSlashPal);
        break;
    case 2:
        if (!FreeTempTileDataBuffersIfPossible())
        {
            BlendPalettes(PALETTES_BG, 16, RGB_BLACK);
            CreateTask(Task_TitleScreenMain, 4);
            CreateTask(Task_TitleScreenTimer, 2);
            SetVBlankCallback(VBlankCB);
            SetMainCallback2(CB2_TitleScreenRun);
            m4aSongNumStart(MUS_RG_TITLE);
        }
        return;
    }
    gMain.state++;
}

static void ResetGpuRegs(void)
{
    SetGpuReg(REG_OFFSET_DISPCNT,  0);
    SetGpuReg(REG_OFFSET_BLDCNT,   0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY,     0);
    SetGpuReg(REG_OFFSET_BG0HOFS,  0);
    SetGpuReg(REG_OFFSET_BG0VOFS,  0);
    SetGpuReg(REG_OFFSET_BG1HOFS,  0);
    SetGpuReg(REG_OFFSET_BG1VOFS,  0);
    SetGpuReg(REG_OFFSET_BG2HOFS,  0);
    SetGpuReg(REG_OFFSET_BG2VOFS,  0);
    SetGpuReg(REG_OFFSET_BG3HOFS,  0);
    SetGpuReg(REG_OFFSET_BG3VOFS,  0);
}

static void CB2_TitleScreenRun(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void Task_TitleScreenTimer(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (++data[0] >= TITLE_SCREEN_TIMEOUT)
        DestroyTask(taskId);
}

// task data for Task_TitleScreenMain and the scenes
#define tSceneNum      data[0]
#define tState         data[1]
#define tTimer         data[2]
#define tInk           data[3]    // 0 full pencil .. 16 ghost lines
#define tInkTarget     data[4]
#define tSlashSpriteId data[6]
#define tPage          data[7]
#define tBoilTimer     data[8]
#define tBoilFrame     data[9]
#define tDrawPos       data[10]   // SKETCHBOOK: window edge x; FLIPBOOK: slot * 256 + row
#define tPageDone      data[11]   // the current page is fully drawn
#define tStamped       data[12]   // the logo is on screen
#define tHoldTimer     data[13]

// Everything but the sketch is visible outside the reveal windows. The OBJ
// window (the slash) keeps its lighten effect and also hides the undrawn sketch.
static void BeginReveal(s16 *data)
{
    tPageDone = FALSE;
    tDrawPos = 0;
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_BG_ALL | WININ_WIN0_OBJ | WININ_WIN1_BG_ALL | WININ_WIN1_OBJ);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG0 | WINOUT_WIN01_BG2 | WINOUT_WIN01_BG3 | WINOUT_WIN01_OBJ
                                 | WINOUT_WINOBJ_BG0 | WINOUT_WINOBJ_BG2 | WINOUT_WINOBJ_BG3 | WINOUT_WINOBJ_OBJ | WINOUT_WINOBJ_CLR);
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, 0));
    SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, DISPLAY_HEIGHT));
    SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(0, 0));
    SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(0, 0));
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    ShowBg(1);
}

static void EndReveal(s16 *data)
{
    tPageDone = TRUE;
    ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WINOBJ_ALL);
}

// One frame of the pencil. Returns TRUE once the page is fully drawn.
static bool32 UpdateReveal(s16 *data)
{
#if TITLE_STYLE == TITLE_STYLE_FLIPBOOK
    s32 slot = tDrawPos >> 8;
    s32 row = tDrawPos & 0xFF;

    // finished slots through WIN0; the one being drawn grows down through WIN1
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, slot * SLOT_WIDTH));
    SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(slot * SLOT_WIDTH, slot * SLOT_WIDTH + SLOT_WIDTH));
    SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(SLOT_TOP, min(SLOT_TOP + row, SLOT_BOTTOM)));
    row += SLOT_SPEED;
    if (row > SLOT_BOTTOM - SLOT_TOP + SLOT_PAUSE * SLOT_SPEED)
    {
        row = 0;
        if (++slot >= DISPLAY_WIDTH / SLOT_WIDTH)
            return TRUE;
    }
    tDrawPos = (slot << 8) | row;
    return FALSE;
#else
#if TITLE_SKETCH_CORNER
    tDrawPos += 1;
#else
    tDrawPos += 2;
#endif
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, min(SKETCH_LEFT + tDrawPos, DISPLAY_WIDTH)));
    return SKETCH_LEFT + tDrawPos >= DISPLAY_WIDTH;
#endif
}

static void UpdateBoil(s16 *data)
{
    if (++tBoilTimer >= BOIL_FRAMES)
    {
        tBoilTimer = 0;
        tBoilFrame ^= 1;
        if (tBoilFrame)
            SetGpuReg(REG_OFFSET_BG1CNT, BG1CNT_SKETCH(SKETCH_CHAR_B, SKETCH_MAP_B));
        else
            SetGpuReg(REG_OFFSET_BG1CNT, BG1CNT_SKETCH(SKETCH_CHAR_A, SKETCH_MAP_A));
    }
}

// Steps the sketch's palette one sixteenth toward full pencil or ghost lines.
static void UpdateInk(s16 *data)
{
    s32 i;

    if (tInk == tInkTarget)
        return;
    tInk += tInk < tInkTarget ? 1 : -1;
    for (i = 1; i < 16; i++)
    {
        u16 a = sSketch_Pal[i], b = sSketchGhost_Pal[i];
        s32 r = GET_R(a) + ((GET_R(b) - GET_R(a)) * tInk >> 4);
        s32 g = GET_G(a) + ((GET_G(b) - GET_G(a)) * tInk >> 4);
        s32 bl = GET_B(a) + ((GET_B(b) - GET_B(a)) * tInk >> 4);
        gPlttBufferUnfaded[BG_PLTT_ID(PAL_SKETCH) + i] = RGB2(r, g, bl);
        gPlttBufferFaded[BG_PLTT_ID(PAL_SKETCH) + i] = RGB2(r, g, bl);
    }
}

static void ShowWholeTitle(s16 *data)
{
    LoadPalette(sGameTitleLogo_Pal, BG_PLTT_ID(0), NUM_LOGO_PALS * PLTT_SIZE_4BPP);
    if (TITLE_SKETCH_CORNER)
    {
        LoadPalette(sSketch_Pal, BG_PLTT_ID(PAL_SKETCH), PLTT_SIZE_4BPP);
    }
    else
    {
        LoadPalette(sSketchGhost_Pal, BG_PLTT_ID(PAL_SKETCH), PLTT_SIZE_4BPP);
        tInk = tInkTarget = 16;
    }
    LoadPalette(sText_Pal, BG_PLTT_ID(PAL_TEXT), PLTT_SIZE_4BPP);
    LoadPalette(sPaper_Pal, BG_PLTT_ID(PAL_PAPER), PLTT_SIZE_4BPP);
    EndReveal(data);
    ShowBg(0);
    ShowBg(1);
    ShowBg(2);
    ShowBg(3);
    tStamped = TRUE;
}

static void Task_TitleScreenMain(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    if (JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON)
        && tSceneNum != TITLESCREENSCENE_RUN
        && tSceneNum != TITLESCREENSCENE_RESTART
        && tSceneNum != TITLESCREENSCENE_CONFIRM)
    {
        ResetPaletteFadeControl();
        ShowWholeTitle(data);
        SetTitleScreenScene(data, TITLESCREENSCENE_RUN);
    }
    else
    {
        sSceneFuncs[tSceneNum](data);
    }
    if (tSceneNum != TITLESCREENSCENE_INIT)
    {
        UpdateBoil(data);
        UpdateInk(data);
    }
}

static void SetTitleScreenScene(s16 *data, u8 sceneNum)
{
    tState = 0;
    tTimer = 0;
    tSceneNum = sceneNum;
}

// The bands and paper fade in from black.
static void SetTitleScreenScene_Init(s16 *data)
{
    switch (tState)
    {
    case 0:
        HideBg(0);
        HideBg(1);
        HideBg(2);
        ShowBg(3);
        BeginNormalPaletteFade(PALETTES_BG, 1, 16, 0, RGB_BLACK);
        tState++;
        break;
    case 1:
        if (!gPaletteFade.active && ++tTimer > 20)
        {
            BeginReveal(data);
            SetTitleScreenScene(data, TITLESCREENSCENE_DRAW);
        }
        break;
    }
}

static void SetTitleScreenScene_Draw(s16 *data)
{
    if (UpdateReveal(data))
    {
        EndReveal(data);
        SetTitleScreenScene(data, tStamped ? TITLESCREENSCENE_RUN : TITLESCREENSCENE_STAMP);
    }
}

// The logo appears white and settles into its colours; the sketch fades back.
static void StampLogo(s16 *data)
{
    if (!TITLE_SKETCH_CORNER)
        tInkTarget = 16;
    BlendPalettes(LOGO_PALS, 16, RGB_WHITE);
    ShowBg(0);
    BeginNormalPaletteFade(LOGO_PALS, 1, 16, 0, RGB_WHITE);
    PlaySE(SE_PIN);
}

static void SetTitleScreenScene_Stamp(s16 *data)
{
    switch (tState)
    {
    case 0:
        if (++tTimer > FIRST_ALONE)
        {
            StampLogo(data);
            tState++;
        }
        break;
    case 1:
        if (!gPaletteFade.active)
        {
            tStamped = TRUE;
            ShowBg(2);
            SetTitleScreenScene(data, TITLESCREENSCENE_RUN);
        }
        break;
    }
}

// tState 0: first frame; 1: holding a page; 2: logo and sketch fading out;
// 3: drawing the next page; 4: the page alone; 5: logo stamping on
static void SetTitleScreenScene_Run(s16 *data)
{
    switch (tState)
    {
    case 0:
        if (!FuncIsActiveTask(Task_TitleScreen_BlinkPressStart))
        {
            CreateTask(Task_TitleScreen_BlinkPressStart, 0);
            SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_LIGHTEN);
            SetGpuReg(REG_OFFSET_BLDY, 13);
            tSlashSpriteId = CreateSlashSprite();
        }
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WINOBJ_ALL);
        tHoldTimer = 0;
        tState = 1;
        break;
    case 1:
        if (++tHoldTimer >= PAGE_HOLD && !gPaletteFade.active)
        {
            // the pencil lines fade into the paper (full scenes: the logo lifts off too)
            BeginNormalPaletteFade((TITLE_SKETCH_CORNER ? 0 : LOGO_PALS) | (1 << PAL_SKETCH), 2, 0, 16, sPaper_Pal[1]);
            tState = 2;
        }
        break;
    case 2:
        if (!gPaletteFade.active)
        {
            if (!TITLE_SKETCH_CORNER)
                HideBg(0);
            HideBg(1);
            if (++tPage >= (s16)ARRAY_COUNT(sPages))
                tPage = 0;
            LoadPage(tPage);
            LoadPalette(sSketch_Pal, BG_PLTT_ID(PAL_SKETCH), PLTT_SIZE_4BPP);
            tInk = tInkTarget = 0;
            BeginReveal(data);
            tState = 3;
        }
        break;
    case 3:
        if (UpdateReveal(data))
        {
            EndReveal(data);
            SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WINOBJ_ALL);
            tHoldTimer = 0;
            tState = TITLE_SKETCH_CORNER ? 1 : 4;
        }
        break;
    case 4:
        if (++tHoldTimer >= SCENE_ALONE)
        {
            StampLogo(data);
            tState = 5;
        }
        break;
    case 5:
        if (!gPaletteFade.active)
        {
            tHoldTimer = 0;
            tState = 1;
        }
        break;
    }

    if (JOY_HELD(CLEAR_SAVE_BUTTON_COMBO) == CLEAR_SAVE_BUTTON_COMBO)
    {
        DeactivateSlashSprite(tSlashSpriteId);
        DestroyTask(FindTaskIdByFunc(Task_TitleScreenMain));
        SetMainCallback2(CB2_FadeOutTransitionToSaveClearScreen);
    }
    else if (JOY_HELD(RESET_RTC_BUTTON_COMBO) == RESET_RTC_BUTTON_COMBO && CanResetRTC() == TRUE)
    {
        DeactivateSlashSprite(tSlashSpriteId);
        DestroyTask(FindTaskIdByFunc(Task_TitleScreenMain));
        FadeOutBGM(4);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        SetMainCallback2(CB2_FadeOutTransitionToResetRtcScreen);
    }
    else if (JOY_NEW(A_BUTTON | START_BUTTON))
    {
        SetTitleScreenScene(data, TITLESCREENSCENE_CONFIRM);
    }
    else if (!FuncIsActiveTask(Task_TitleScreenTimer) && tState == 1)
    {
        SetTitleScreenScene(data, TITLESCREENSCENE_RESTART);
    }
}

static void SetTitleScreenScene_Restart(s16 *data)
{
    switch (tState)
    {
    case 0:
        DeactivateSlashSprite(tSlashSpriteId);
        tState++;
        break;
    case 1:
        if (!gPaletteFade.active && !IsSlashSpriteDeactivated(tSlashSpriteId))
        {
            FadeOutMapMusic(10);
            BeginNormalPaletteFade(PALETTES_ALL, 3, 0, 16, RGB_BLACK);
            SignalEndTitleScreenPaletteSomethingTask();
            tState++;
        }
        break;
    case 2:
        if (IsNotWaitingForBGMStop() && !gPaletteFade.active)
        {
            DestroyTask(FindTaskIdByFunc(Task_TitleScreen_BlinkPressStart));
            tState++;
        }
        break;
    case 3:
        if (++tTimer >= 20)
            tState++;
        break;
    case 4:
        DestroyTask(FindTaskIdByFunc(Task_TitleScreenMain));
        SetMainCallback2(CB2_InitCopyrightScreenAfterTitleScreen);
        break;
    }
}

// The save was loaded at boot (CB2_InitCopyrightScreenAfterBootup), so PRESS
// START goes straight to the main menu.
static void SetTitleScreenScene_Confirm(s16 *data)
{
    switch (tState)
    {
    case 0:
        if (!gPaletteFade.active)
        {
            PlaySE(SE_SELECT);
            DeactivateSlashSprite(tSlashSpriteId);
            tState++;
        }
        break;
    case 1:
        if (tTimer < 30)
            tTimer++;
        else if (!IsSlashSpriteDeactivated(tSlashSpriteId))
        {
            BeginNormalPaletteFade(PALETTES_BG | (PALETTES_OBJECTS & 0x0FFF0000), 0, 0, 16, RGB_WHITE);
            SignalEndTitleScreenPaletteSomethingTask();
            FadeOutBGM(4);
            tState++;
        }
        break;
    case 2:
        if (!gPaletteFade.active)
        {
            DestroyTask(FindTaskIdByFunc(Task_TitleScreenMain));
            SetMainCallback2(CB2_GoToMainMenu);
        }
        break;
    }
}

#undef tSceneNum
#undef tState
#undef tTimer
#undef tInk
#undef tInkTarget
#undef tSlashSpriteId
#undef tPage
#undef tBoilTimer
#undef tBoilFrame
#undef tDrawPos
#undef tPageDone
#undef tStamped
#undef tHoldTimer

static void CB2_GoToMainMenu(void)
{
    if (!UpdatePaletteFade())
        SetMainCallback2(CB2_InitMainMenu);
}

// PRESS START blinks by swapping its 5 colours (BG palette 15, 1-5) for the
// paper colour under it (entry 6).
static void Task_TitleScreen_BlinkPressStart(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    s32 i;

    if (data[15] && gPaletteFade.active)
        data[14] = 1;
    if (data[14] && !gPaletteFade.active)
    {
        DestroyTask(taskId);
        return;
    }
    data[2] = data[1] ? 30 : 60;
    if (++data[0] >= data[2])
    {
        data[0] = 0;
        data[1] ^= 1;
        for (i = 0; i < 5; i++)
        {
            u16 c = data[1] ? sText_Pal[6] : sText_Pal[1 + i];
            gPlttBufferUnfaded[BG_PLTT_ID(PAL_TEXT) + 1 + i] = c;
            gPlttBufferFaded[BG_PLTT_ID(PAL_TEXT) + 1 + i] = c;
        }
        if (data[14])
            BlendPalettes(1 << PAL_TEXT, gPaletteFade.y, gPaletteFade.blendColor);
    }
}

static void SignalEndTitleScreenPaletteSomethingTask(void)
{
    u8 taskId = FindTaskIdByFunc(Task_TitleScreen_BlinkPressStart);
    if (taskId != TASK_NONE)
        gTasks[taskId].data[15] = TRUE;
}

static void CB2_FadeOutTransitionToSaveClearScreen(void)
{
    if (!UpdatePaletteFade())
        SetMainCallback2(CB2_InitClearSaveDataScreen);
}

static void CB2_FadeOutTransitionToResetRtcScreen(void)
{
    if (!UpdatePaletteFade())
        SetMainCallback2(CB2_InitResetRtcScreen);
}

#define sState       data[0]
#define sTimer       data[1]
#define sDeactivate  data[2]

// The slash is an OBJ window sprite: where it passes, the logo (BG0) is lightened.
// It follows the logo's slant: 10 px lower than FRLG's, as the logo sits lower.
static u8 CreateSlashSprite(void)
{
    u8 spriteId = CreateSprite(&sSlashSpriteTemplate, -32, 37, 1);
    if (spriteId != MAX_SPRITES)
    {
        gSprites[spriteId].callback = SpriteCallback_Slash;
        gSprites[spriteId].sTimer = 540;
    }
    return spriteId;
}

static void DeactivateSlashSprite(u8 spriteId)
{
    if (spriteId != MAX_SPRITES)
        gSprites[spriteId].sDeactivate = TRUE;
}

static bool32 IsSlashSpriteDeactivated(u8 spriteId)
{
    if (spriteId != MAX_SPRITES)
        return gSprites[spriteId].sState ^ 2 ? TRUE : FALSE;
    else
        return FALSE;
}

static void SpriteCallback_Slash(struct Sprite *sprite)
{
    switch (sprite->sState)
    {
    case 0:
        if (sprite->sDeactivate)
        {
            sprite->invisible = TRUE;
            sprite->sState = 2;
        }
        sprite->sTimer--;
        if (sprite->sTimer == 0)
        {
            sprite->invisible = FALSE;
            sprite->sState = 1;
        }
        break;
    case 1:
        sprite->x += 9;
        if (sprite->x == 67)
            sprite->y -= 7;

        if (sprite->x == 148)
            sprite->y += 7;

        if (sprite->x > DISPLAY_WIDTH + 32)
        {
            sprite->invisible = TRUE;
            if (sprite->sDeactivate)
                sprite->sState = 2;
            else
            {
                sprite->x = -32;
                sprite->sTimer = 540;
                sprite->sState = 0;
            }
        }
        break;
    case 2:
        break;
    }
}

#undef sState
#undef sTimer
#undef sDeactivate
