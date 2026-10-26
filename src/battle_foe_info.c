#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_foe_info.h"
#include "battle_interface.h"
#include "battle_main.h"
#include "battle_message.h"
#include "battle_talk.h"
#include "bg.h"
#include "data.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "reshow_battle_screen.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "strings.h"
#include "task.h"
#include "text.h"
#include "trainer_pokemon_sprites.h"
#include "util.h"
#include "window.h"
#include "constants/battle.h"
#include "constants/rgb.h"
#include "constants/songs.h"

// The foe info screen, opened with SELECT on the battle action menu (SEL.).
//
// It shows the foe's species and fills in as the player battles that species
// more: every battle against a species counts once (SaveBlock2 foeBattleCounts,
// 4 bits per species, up to FOE_BATTLE_COUNT_MAX), and each page of the screen
// opens at a count (sPageUnlockCounts). It leaves and comes back to the battle
// the way the bag does, so it uses no RAM while the battle runs.

extern const struct PokedexEntry gPokedexEntries[];

enum
{
    PAGE_TYPES,
    PAGE_STATS,
    PAGE_MOVES,
    PAGE_NOTES,
    PAGE_COUNT
};

// Battles against a species before each page fills in
static const u8 sPageUnlockCounts[PAGE_COUNT] =
{
    [PAGE_TYPES] = 2,
    [PAGE_STATS] = 4,
    [PAGE_MOVES] = 7,
    [PAGE_NOTES] = 12,
};
STATIC_ASSERT(FOE_BATTLE_COUNT_MAX >= 12, FoeBattleCountReachesLastPage);

// BG palette 0
enum
{
    COLOR_BG,         // navy screen
    COLOR_WHITE,
    COLOR_SHADOW,     // shadow of white text
    COLOR_RED,        // title bar, the battle UI's coral frame
    COLOR_LCD,        // page panel, the action menu's LCD
    COLOR_LCD_TEXT,
    COLOR_LCD_SHADOW,
    COLOR_TYPE1,      // the foe's types, set from gTypeTextColors
    COLOR_TYPE2,
    COLOR_DIM,        // locked tabs, hints
    COLOR_YELLOW,     // trainer state
    COLOR_BAR,        // stat bars
    COLOR_TAB,        // unselected tab
};

static const u16 sPalette[] =
{
    [COLOR_BG]         = RGB(3, 4, 10),
    [COLOR_WHITE]      = RGB_WHITE,
    [COLOR_SHADOW]     = RGB(9, 10, 16),
    [COLOR_RED]        = RGB(28, 10, 10),
    [COLOR_LCD]        = RGB(19, 23, 12),
    [COLOR_LCD_TEXT]   = RGB(4, 7, 4),
    [COLOR_LCD_SHADOW] = RGB(15, 19, 9),
    [COLOR_TYPE1]      = RGB_WHITE,
    [COLOR_TYPE2]      = RGB_WHITE,
    [COLOR_DIM]        = RGB(13, 14, 19),
    [COLOR_YELLOW]     = RGB(31, 26, 8),
    [COLOR_BAR]        = RGB(6, 12, 6),
    [COLOR_TAB]        = RGB(7, 9, 18),
};

static const u8 sColor_White[] = {TEXT_COLOR_TRANSPARENT, COLOR_WHITE, COLOR_SHADOW};
static const u8 sColor_Dim[] = {TEXT_COLOR_TRANSPARENT, COLOR_DIM, TEXT_COLOR_TRANSPARENT};
static const u8 sColor_Yellow[] = {TEXT_COLOR_TRANSPARENT, COLOR_YELLOW, COLOR_SHADOW};
static const u8 sColor_Lcd[] = {TEXT_COLOR_TRANSPARENT, COLOR_LCD_TEXT, COLOR_LCD_SHADOW};
static const u8 sColor_Type1[] = {TEXT_COLOR_TRANSPARENT, COLOR_TYPE1, COLOR_SHADOW};
static const u8 sColor_Type2[] = {TEXT_COLOR_TRANSPARENT, COLOR_TYPE2, COLOR_SHADOW};

// Layout, in pixels of the one full-screen window
#define TITLE_HEIGHT  13
#define PIC_X         40 // center of the front pic
#define PIC_Y         48
#define INFO_X        80
#define TAB_Y         82
#define TAB_WIDTH     56
#define TAB_HEIGHT    13
#define PANEL_X       8
#define PANEL_Y       (TAB_Y + TAB_HEIGHT)
#define PANEL_WIDTH   (DISPLAY_WIDTH - 2 * PANEL_X)
#define PANEL_HEIGHT  (DISPLAY_HEIGHT - PANEL_Y - 4)
#define LINE_HEIGHT   13
#define STAT_BAR_WIDTH 48
#define STAT_BAR_MAX   160 // a full bar; Gen 1 base stats rarely go higher

static const u8 sText_Title[] = _("FOE INFO");
static const u8 sText_Help[] = _("{LEFT_ARROW}{RIGHT_ARROW}PAGE  B:BACK");
static const u8 sText_Lv[] = _("{LV}");
static const u8 sText_Battled[] = _("Battled: ");
static const u8 sText_Times[] = _(" times");
static const u8 sText_Once[] = _("Battled: once");
static const u8 sText_TrainerCalm[] = _("{B_TRAINER1_NAME}: calm");
static const u8 sText_TrainerResolute[] = _("{B_TRAINER1_NAME}: RESOLUTE");
static const u8 sText_TrainerPanic[] = _("{B_TRAINER1_NAME}: PANIC");
static const u8 sText_TurnsLeft[] = _(", {STR_VAR_1} turns left");
static const u8 sText_Locked[] = _("???");
static const u8 sText_BattleMore[] = _("Battle {STR_VAR_1} more to fill this in.");
static const u8 sText_BattleOneMore[] = _("Battle it once more to fill this in.");
static const u8 sText_Weak[] = _("WEAK");
static const u8 sText_Resists[] = _("RESISTS");
static const u8 sText_Immune[] = _("IMMUNE");
static const u8 sText_None[] = _("-");
static const u8 sText_x4[] = _("x4");
static const u8 sText_Pokemon[] = _(" POKéMON");
static const u8 sText_Space[] = _(" ");

static const u8 *const sPageNames[PAGE_COUNT] =
{
    [PAGE_TYPES] = (const u8[])_("TYPES"),
    [PAGE_STATS] = (const u8[])_("STATS"),
    [PAGE_MOVES] = (const u8[])_("MOVES"),
    [PAGE_NOTES] = (const u8[])_("NOTES"),
};

enum
{
    STAT_ROW_HP,
    STAT_ROW_ATK,
    STAT_ROW_DEF,
    STAT_ROW_SPD,
    STAT_ROW_SPC,
    STAT_ROW_INT,
    STAT_ROW_COUNT
};

static const u8 *const sStatNames[STAT_ROW_COUNT] =
{
    [STAT_ROW_HP]  = (const u8[])_("HP"),
    [STAT_ROW_ATK] = (const u8[])_("ATK"),
    [STAT_ROW_DEF] = (const u8[])_("DEF"),
    [STAT_ROW_SPD] = (const u8[])_("SPD"),
    [STAT_ROW_SPC] = (const u8[])_("SPC"),
    [STAT_ROW_INT] = (const u8[])_("INT"),
};

struct FoeInfo
{
    u16 species;
    u8 level;
    u8 page;
    u8 count;
    u8 spriteId;
};

static EWRAM_DATA struct FoeInfo *sFoeInfo = NULL;

static void CB2_FoeInfoRun(void);
static void VBlankCB_FoeInfo(void);
static void Task_FoeInfo(u8 taskId);
static void Task_FoeInfoExit(u8 taskId);
static void FoeInfo_Draw(void);

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = DISPLAY_TILE_WIDTH,
        .height = DISPLAY_TILE_HEIGHT,
        .paletteNum = 0,
        .baseBlock = 1,
    },
    DUMMY_WIN_TEMPLATE
};

// --- Battle counts ----------------------------------------------------------

u8 GetFoeBattleCount(u16 species)
{
    if (species == SPECIES_NONE || species >= NUM_SPECIES)
        return 0;
    if (species & 1)
        return gSaveBlock2Ptr->foeBattleCounts[species / 2] >> 4;
    return gSaveBlock2Ptr->foeBattleCounts[species / 2] & 0xF;
}

static void SetFoeBattleCount(u16 species, u8 count)
{
    u8 *byte = &gSaveBlock2Ptr->foeBattleCounts[species / 2];

    if (species & 1)
        *byte = (*byte & 0x0F) | (count << 4);
    else
        *byte = (*byte & 0xF0) | count;
}

// A foe mon came out: count a battle against its species, once per enemy party
// slot and battle. Link and Frontier battles don't count, as for the Pokédex.
void RecordFoeBattled(u8 battler)
{
    u8 slotBit = 1 << gBattlerPartyIndexes[battler];
    u16 species = gBattleMons[battler].species;
    u8 count;

    if (GetBattlerSide(battler) != B_SIDE_OPPONENT
     || (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_FRONTIER))
     || (gBattleStruct->foeInfoCounted & slotBit)
     || species == SPECIES_NONE || species >= NUM_SPECIES)
        return;

    gBattleStruct->foeInfoCounted |= slotBit;
    count = GetFoeBattleCount(species);
    if (count < FOE_BATTLE_COUNT_MAX)
        SetFoeBattleCount(species, count + 1);
}

// --- Screen -------------------------------------------------------------------

static u8 GetShownFoe(void)
{
    u8 foe = GetBattlerAtPosition(B_POSITION_OPPONENT_LEFT);

    if ((gBattleTypeFlags & BATTLE_TYPE_DOUBLE) && (gAbsentBattlerFlags & gBitTable[foe]))
        foe = GetBattlerAtPosition(B_POSITION_OPPONENT_RIGHT);
    return foe;
}

static bool32 IsPageUnlocked(u8 page)
{
    return sFoeInfo->count >= sPageUnlockCounts[page];
}

void CB2_FoeInfoScreen(void)
{
    switch (gMain.state)
    {
    case 0:
        SetVBlankHBlankCallbacksToNull();
        ClearScheduledBgCopiesToVram();
        ScanlineEffect_Stop();
        FreeAllSpritePalettes();
        ResetPaletteFade();
        ResetSpriteData();
        ResetTasks();
        ResetAllPicSprites();
        sFoeInfo = AllocZeroed(sizeof(*sFoeInfo));
        gMain.state++;
        break;
    case 1:
    {
        u8 foe = GetShownFoe();
        u8 page;

        sFoeInfo->species = gBattleMons[foe].species;
        sFoeInfo->level = gBattleMons[foe].level;
        sFoeInfo->count = GetFoeBattleCount(sFoeInfo->species);
        // Open on the last page that has filled in
        for (page = PAGE_COUNT - 1; page > 0 && !IsPageUnlocked(page); page--)
            ;
        sFoeInfo->page = page;

        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        InitWindows(sWindowTemplates);
        DeactivateAllTextPrinters();
        LoadPalette(sPalette, BG_PLTT_ID(0), sizeof(sPalette));
        LoadPalette(&gTypeTextColors[gSpeciesInfo[sFoeInfo->species].types[0]], BG_PLTT_ID(0) + COLOR_TYPE1, PLTT_SIZEOF(1));
        LoadPalette(&gTypeTextColors[gSpeciesInfo[sFoeInfo->species].types[1]], BG_PLTT_ID(0) + COLOR_TYPE2, PLTT_SIZEOF(1));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        gMain.state++;
        break;
    }
    case 2:
    {
        struct Pokemon *mon = &gEnemyParty[gBattlerPartyIndexes[GetShownFoe()]];

        sFoeInfo->spriteId = CreateMonPicSprite_HandleDeoxys(sFoeInfo->species,
                                                             GetMonData(mon, MON_DATA_OT_ID),
                                                             GetMonData(mon, MON_DATA_PERSONALITY),
                                                             TRUE, PIC_X, PIC_Y, 0, TAG_NONE);
        gSprites[sFoeInfo->spriteId].oam.priority = 0;
        PutWindowTilemap(0);
        FoeInfo_Draw();
        ShowBg(0);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_FoeInfo);
        CreateTask(Task_FoeInfo, 0);
        SetMainCallback2(CB2_FoeInfoRun);
        break;
    }
    }
}

static void CB2_FoeInfoRun(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_FoeInfo(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

// Prints a string at x and returns the x after it
static u32 PrintAt(const u8 *str, u8 fontId, u32 x, u32 y, const u8 *color)
{
    AddTextPrinterParameterized3(0, fontId, x, y, color, TEXT_SKIP_DRAW, str);
    return x + GetStringWidth(fontId, str, 0);
}

static void DrawHeader(void)
{
    u16 species = sFoeInfo->species;
    u8 type1 = gSpeciesInfo[species].types[0];
    u8 type2 = gSpeciesInfo[species].types[1];
    u8 *str;
    u32 x;

    FillWindowPixelRect(0, COLOR_RED, 0, 0, DISPLAY_WIDTH, TITLE_HEIGHT);
    PrintAt(sText_Title, FONT_SMALL, 4, 0, sColor_White);
    PrintAt(sText_Help, FONT_SMALL, DISPLAY_WIDTH - 4 - GetStringWidth(FONT_SMALL, sText_Help, 0), 0, sColor_White);

    // Name and level
    x = PrintAt(gSpeciesNames[species], FONT_NORMAL, INFO_X, 16, sColor_White);
    str = StringCopy(gStringVar4, sText_Lv);
    ConvertIntToDecimalStringN(str, sFoeInfo->level, STR_CONV_MODE_LEFT_ALIGN, 3);
    PrintAt(gStringVar4, FONT_SMALL, x + 6, 18, sColor_White);

    // Types, each in its color
    x = PrintAt(gTypeNames[type1], FONT_SMALL, INFO_X, 31, sColor_Type1);
    if (type2 != type1)
    {
        x = PrintAt(gText_Slash, FONT_SMALL, x + 2, 31, sColor_White);
        PrintAt(gTypeNames[type2], FONT_SMALL, x + 2, 31, sColor_Type2);
    }

    // How often it was battled
    if (sFoeInfo->count == 1)
    {
        PrintAt(sText_Once, FONT_SMALL, INFO_X, 44, sColor_White);
    }
    else
    {
        str = StringCopy(gStringVar4, sText_Battled);
        str = ConvertIntToDecimalStringN(str, sFoeInfo->count, STR_CONV_MODE_LEFT_ALIGN, 2);
        if (sFoeInfo->count >= FOE_BATTLE_COUNT_MAX)
            *str++ = CHAR_PLUS;
        StringCopy(str, sText_Times);
        PrintAt(gStringVar4, FONT_SMALL, INFO_X, 44, sColor_White);
    }

    // The foe trainer's mood (Talk)
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) && !(gBattleTypeFlags & BATTLE_TYPE_LINK))
    {
        u8 state = GetBattlerTrainerState(GetShownFoe());
        const u8 *text = sText_TrainerCalm;

        if (state == TRAINER_STATE_RESOLUTE)
            text = sText_TrainerResolute;
        else if (state == TRAINER_STATE_PANIC)
            text = sText_TrainerPanic;
        BattleStringExpandPlaceholders(text, gStringVar4);
        if (state != TRAINER_STATE_NONE)
        {
            ConvertIntToDecimalStringN(gStringVar1, gBattleStruct->trainerStateTurns, STR_CONV_MODE_LEFT_ALIGN, 1);
            StringExpandPlaceholders(gStringVar4 + StringLength(gStringVar4), sText_TurnsLeft);
        }
        PrintAt(gStringVar4, FONT_SMALL, INFO_X, 57, (state == TRAINER_STATE_NONE) ? sColor_Dim : sColor_Yellow);
    }
}

static void DrawTabs(void)
{
    u32 page;

    for (page = 0; page < PAGE_COUNT; page++)
    {
        u32 x = PANEL_X + page * TAB_WIDTH;
        const u8 *name = IsPageUnlocked(page) ? sPageNames[page] : sText_Locked;
        const u8 *color;

        if (page == sFoeInfo->page)
        {
            FillWindowPixelRect(0, COLOR_LCD, x, TAB_Y, TAB_WIDTH - 2, TAB_HEIGHT);
            color = sColor_Lcd;
        }
        else
        {
            FillWindowPixelRect(0, COLOR_TAB, x, TAB_Y, TAB_WIDTH - 2, TAB_HEIGHT);
            color = IsPageUnlocked(page) ? sColor_White : sColor_Dim;
        }
        PrintAt(name, FONT_SMALL, x + (TAB_WIDTH - 2 - GetStringWidth(FONT_SMALL, name, 0)) / 2, TAB_Y, color);
    }
}

static u8 GetTypeMultiplier(u8 atkType, u16 species)
{
    u32 mul = TYPE_MUL_NORMAL;
    u32 i;

    for (i = 0; TYPE_EFFECT_ATK_TYPE(i) != TYPE_ENDTABLE; i += 3)
    {
        if (TYPE_EFFECT_ATK_TYPE(i) != atkType)
            continue;
        if (TYPE_EFFECT_DEF_TYPE(i) == gSpeciesInfo[species].types[0])
            mul = mul * TYPE_EFFECT_MULTIPLIER(i) / TYPE_MUL_NORMAL;
        if (TYPE_EFFECT_DEF_TYPE(i) == gSpeciesInfo[species].types[1]
         && gSpeciesInfo[species].types[1] != gSpeciesInfo[species].types[0])
            mul = mul * TYPE_EFFECT_MULTIPLIER(i) / TYPE_MUL_NORMAL;
    }
    return mul;
}

enum
{
    MATCHUP_WEAK,
    MATCHUP_RESISTS,
    MATCHUP_IMMUNE,
};

static bool32 IsMatchup(u8 matchup, u32 mul)
{
    switch (matchup)
    {
    case MATCHUP_WEAK:
        return mul > TYPE_MUL_NORMAL;
    case MATCHUP_RESISTS:
        return mul != TYPE_MUL_NO_EFFECT && mul < TYPE_MUL_NORMAL;
    default:
        return mul == TYPE_MUL_NO_EFFECT;
    }
}

// Lists the attacking types of one matchup after its label, wrapping onto the
// next line when they don't fit. Returns the line after the last one used.
static u32 DrawMatchupLine(u8 matchup, const u8 *label, u32 line)
{
    u32 x0 = PANEL_X + 46, x = x0;
    u32 type;
    bool32 any = FALSE;

    PrintAt(label, FONT_SMALL, PANEL_X + 4, PANEL_Y + 2 + line * LINE_HEIGHT, sColor_Lcd);
    for (type = 0; type < NUMBER_OF_MON_TYPES; type++)
    {
        u32 mul, width;

        if (type == TYPE_MYSTERY)
            continue;
        mul = GetTypeMultiplier(type, sFoeInfo->species);
        if (!IsMatchup(matchup, mul))
            continue;
        width = GetStringWidth(FONT_SMALL_NARROW, gTypeNames[type], 0) + 6;
        if (mul > TYPE_MUL_SUPER_EFFECTIVE)
            width += GetStringWidth(FONT_SMALL_NARROW, sText_x4, 0);
        if (x + width > PANEL_X + PANEL_WIDTH && x != x0)
        {
            x = x0;
            line++;
        }
        x = PrintAt(gTypeNames[type], FONT_SMALL_NARROW, x, PANEL_Y + 2 + line * LINE_HEIGHT, sColor_Lcd);
        if (mul > TYPE_MUL_SUPER_EFFECTIVE)
            x = PrintAt(sText_x4, FONT_SMALL_NARROW, x, PANEL_Y + 2 + line * LINE_HEIGHT, sColor_Lcd);
        x += 6;
        any = TRUE;
    }
    if (!any)
        PrintAt(sText_None, FONT_SMALL_NARROW, x0, PANEL_Y + 2 + line * LINE_HEIGHT, sColor_Lcd);
    return line + 1;
}

static void DrawTypesPage(void)
{
    u32 line = 0;

    line = DrawMatchupLine(MATCHUP_WEAK, sText_Weak, line);
    line = DrawMatchupLine(MATCHUP_RESISTS, sText_Resists, line);
    DrawMatchupLine(MATCHUP_IMMUNE, sText_Immune, line);
}

static u8 GetBaseStat(u8 row, u16 species)
{
    switch (row)
    {
    case STAT_ROW_HP:
        return gSpeciesInfo[species].baseHP;
    case STAT_ROW_ATK:
        return gSpeciesInfo[species].baseAttack;
    case STAT_ROW_DEF:
        return gSpeciesInfo[species].baseDefense;
    case STAT_ROW_SPD:
        return gSpeciesInfo[species].baseSpeed;
    case STAT_ROW_SPC:
        return gSpeciesInfo[species].baseSpAttack;
    default:
        return GetSpeciesIntelligence(species);
    }
}

// Base stats in two columns, each with a bar
static void DrawStatsPage(void)
{
    u32 row;

    for (row = 0; row < STAT_ROW_COUNT; row++)
    {
        u32 x = PANEL_X + 6 + (row / 3) * (PANEL_WIDTH / 2);
        u32 y = PANEL_Y + 4 + (row % 3) * (LINE_HEIGHT + 4);
        u8 value = GetBaseStat(row, sFoeInfo->species);

        PrintAt(sStatNames[row], FONT_SMALL, x, y, sColor_Lcd);
        ConvertIntToDecimalStringN(gStringVar4, value, STR_CONV_MODE_RIGHT_ALIGN, 3);
        PrintAt(gStringVar4, FONT_SMALL, x + 20, y, sColor_Lcd);
        FillWindowPixelRect(0, COLOR_LCD_SHADOW, x + 42, y + 4, STAT_BAR_WIDTH, 5);
        FillWindowPixelRect(0, COLOR_BAR, x + 42, y + 4, max(1, min(value, STAT_BAR_MAX) * STAT_BAR_WIDTH / STAT_BAR_MAX), 5);
    }
}

// Level-up moves, three columns of four
static void DrawMovesPage(void)
{
    const u16 *learnset = gLevelUpLearnsets[sFoeInfo->species];
    u32 i;

    for (i = 0; learnset[i] != LEVEL_UP_END && i < 12; i++)
    {
        u32 x = PANEL_X + 4 + (i / 4) * (PANEL_WIDTH / 3);
        u32 y = PANEL_Y + 2 + (i % 4) * LINE_HEIGHT;

        ConvertIntToDecimalStringN(gStringVar4, (learnset[i] & LEVEL_UP_MOVE_LV) >> 9, STR_CONV_MODE_RIGHT_ALIGN, 2);
        PrintAt(gStringVar4, FONT_SMALL_NARROW, x, y, sColor_Lcd);
        PrintAt(gMovesInfo[learnset[i] & LEVEL_UP_MOVE_ID].name, FONT_SMALL_NARROW, x + 12, y, sColor_Lcd);
    }
}

// Pokédex category and text
static void DrawNotesPage(void)
{
    const struct PokedexEntry *entry = &gPokedexEntries[SpeciesToNationalPokedexNum(sFoeInfo->species)];
    u8 *str;

    str = StringCopy(gStringVar4, entry->categoryName);
    StringCopy(str, sText_Pokemon);
    PrintAt(gStringVar4, FONT_SMALL, PANEL_X + 4, PANEL_Y + 2, sColor_Lcd);
    AddTextPrinterParameterized4(0, FONT_SMALL_NARROW, PANEL_X + 4, PANEL_Y + 2 + LINE_HEIGHT, 0, 0,
                                 sColor_Lcd, TEXT_SKIP_DRAW, entry->description);
}

static void DrawLockedPage(void)
{
    u8 missing = sPageUnlockCounts[sFoeInfo->page] - sFoeInfo->count;
    const u8 *text;

    if (missing == 1)
    {
        text = sText_BattleOneMore;
    }
    else
    {
        ConvertIntToDecimalStringN(gStringVar1, missing, STR_CONV_MODE_LEFT_ALIGN, 2);
        StringExpandPlaceholders(gStringVar4, sText_BattleMore);
        text = gStringVar4;
    }
    PrintAt(sText_Locked, FONT_NORMAL, PANEL_X + (PANEL_WIDTH - GetStringWidth(FONT_NORMAL, sText_Locked, 0)) / 2, PANEL_Y + 10, sColor_Lcd);
    PrintAt(text, FONT_SMALL, PANEL_X + (PANEL_WIDTH - GetStringWidth(FONT_SMALL, text, 0)) / 2, PANEL_Y + 28, sColor_Lcd);
}

static void FoeInfo_Draw(void)
{
    FillWindowPixelBuffer(0, PIXEL_FILL(COLOR_BG));
    DrawHeader();
    DrawTabs();
    FillWindowPixelRect(0, COLOR_LCD, PANEL_X, PANEL_Y, PANEL_WIDTH, PANEL_HEIGHT);

    if (!IsPageUnlocked(sFoeInfo->page))
        DrawLockedPage();
    else if (sFoeInfo->page == PAGE_TYPES)
        DrawTypesPage();
    else if (sFoeInfo->page == PAGE_STATS)
        DrawStatsPage();
    else if (sFoeInfo->page == PAGE_MOVES)
        DrawMovesPage();
    else
        DrawNotesPage();

    CopyWindowToVram(0, COPYWIN_FULL);
}

static void Task_FoeInfo(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    if (JOY_NEW(A_BUTTON | B_BUTTON | SELECT_BUTTON))
    {
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_FoeInfoExit;
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        PlaySE(SE_DEX_PAGE);
        sFoeInfo->page = (sFoeInfo->page + 1) % PAGE_COUNT;
        FoeInfo_Draw();
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        PlaySE(SE_DEX_PAGE);
        sFoeInfo->page = (sFoeInfo->page + PAGE_COUNT - 1) % PAGE_COUNT;
        FoeInfo_Draw();
    }
}

static void Task_FoeInfoExit(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    DestroyTask(taskId);
    FreeAndDestroyMonPicSprite(sFoeInfo->spriteId);
    FreeAllWindowBuffers();
    FREE_AND_SET_NULL(sFoeInfo);
    SetMainCallback2(ReshowBattleScreenAfterMenu);
}
