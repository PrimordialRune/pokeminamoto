#include "global.h"
#include "debug_menu.h"

// Battle debug screen, opened by holding L and pressing SELECT on the action menu.
// Edits HP, TP, status and stat stages of any battler, then returns to the battle
// the way the bag does. Not built into release ROMs (`make release`).

#ifdef DEBUG_MENUS

#include "battle.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "bg.h"
#include "data.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "menu_helpers.h"
#include "palette.h"
#include "pokemon.h"
#include "reshow_battle_screen.h"
#include "scanline_effect.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

enum
{
    ROW_HP,
    ROW_TP,
    ROW_STATUS,
    ROW_ATK,
    ROW_DEF,
    ROW_SPEED,
    ROW_SPATK,
    ROW_SPDEF,
    ROW_ACC,
    ROW_EVASION,
    ROW_COUNT
};

enum
{
    STATUS_OPTION_NONE,
    STATUS_OPTION_SLEEP,
    STATUS_OPTION_POISON,
    STATUS_OPTION_BURN,
    STATUS_OPTION_FREEZE,
    STATUS_OPTION_PARALYSIS,
    STATUS_OPTION_TOXIC,
    STATUS_OPTION_COUNT
};

struct BattleDebug
{
    u8 battler;
    u8 row;
};

static EWRAM_DATA struct BattleDebug *sBattleDebug = NULL;

static void CB2_BattleDebugRun(void);
static void VBlankCB_BattleDebug(void);
static void Task_BattleDebug(u8 taskId);
static void Task_BattleDebugExit(u8 taskId);
static void BattleDebug_Draw(void);

static const struct BgTemplate sBgTemplates[] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0,
    },
};

static const struct WindowTemplate sWindowTemplates[] =
{
    {
        .bg = 0,
        .tilemapLeft = 1,
        .tilemapTop = 1,
        .width = 28,
        .height = 18,
        .paletteNum = 0,
        .baseBlock = 1,
    },
    DUMMY_WIN_TEMPLATE
};

static const u16 sPalette[] =
{
    RGB(2, 4, 10),   // background
    RGB_WHITE,       // text
    RGB(9, 10, 14),  // shadow
    RGB(31, 22, 4),  // selected text
    RGB(14, 8, 0),   // selected shadow
};

static const u8 sColor_Normal[] = {0, 1, 2};
static const u8 sColor_Selected[] = {0, 3, 4};

static const u8 sText_Title[] = _("BATTLE DEBUG");
static const u8 sText_Help[] = _("{LEFT_ARROW}{RIGHT_ARROW} change  R: x10  SELECT: next  B: back");
static const u8 sText_Player[] = _(" (player)");
static const u8 sText_Foe[] = _(" (foe)");
static const u8 sText_Slash[] = _("/");

static const u8 *const sRowNames[ROW_COUNT] =
{
    [ROW_HP]      = (const u8[])_("HP"),
    [ROW_TP]      = (const u8[])_("TP"),
    [ROW_STATUS]  = (const u8[])_("Status"),
    [ROW_ATK]     = (const u8[])_("Attack"),
    [ROW_DEF]     = (const u8[])_("Defense"),
    [ROW_SPEED]   = (const u8[])_("Speed"),
    [ROW_SPATK]   = (const u8[])_("Sp. Atk"),
    [ROW_SPDEF]   = (const u8[])_("Sp. Def"),
    [ROW_ACC]     = (const u8[])_("Accuracy"),
    [ROW_EVASION] = (const u8[])_("Evasion"),
};

static const u8 *const sStatusNames[STATUS_OPTION_COUNT] =
{
    [STATUS_OPTION_NONE]      = (const u8[])_("None"),
    [STATUS_OPTION_SLEEP]     = (const u8[])_("Sleep (3)"),
    [STATUS_OPTION_POISON]    = (const u8[])_("Poison"),
    [STATUS_OPTION_BURN]      = (const u8[])_("Burn"),
    [STATUS_OPTION_FREEZE]    = (const u8[])_("Freeze"),
    [STATUS_OPTION_PARALYSIS] = (const u8[])_("Paralysis"),
    [STATUS_OPTION_TOXIC]     = (const u8[])_("Toxic"),
};

static const u32 sStatusValues[STATUS_OPTION_COUNT] =
{
    [STATUS_OPTION_NONE]      = 0,
    [STATUS_OPTION_SLEEP]     = 3,
    [STATUS_OPTION_POISON]    = STATUS1_POISON,
    [STATUS_OPTION_BURN]      = STATUS1_BURN,
    [STATUS_OPTION_FREEZE]    = STATUS1_FREEZE,
    [STATUS_OPTION_PARALYSIS] = STATUS1_PARALYSIS,
    [STATUS_OPTION_TOXIC]     = STATUS1_TOXIC_POISON,
};

static const u8 sRowToStat[ROW_COUNT] =
{
    [ROW_ATK]     = STAT_ATK,
    [ROW_DEF]     = STAT_DEF,
    [ROW_SPEED]   = STAT_SPEED,
    [ROW_SPATK]   = STAT_SPATK,
    [ROW_SPDEF]   = STAT_SPDEF,
    [ROW_ACC]     = STAT_ACC,
    [ROW_EVASION] = STAT_EVASION,
};

static struct Pokemon *GetBattlerMon(u8 battler)
{
    if (GetBattlerSide(battler) == B_SIDE_PLAYER)
        return &gPlayerParty[gBattlerPartyIndexes[battler]];
    return &gEnemyParty[gBattlerPartyIndexes[battler]];
}

static u8 *GetTP(u8 battler)
{
    return &gBattleStruct->tp[GetBattlerSide(battler)][gBattlerPartyIndexes[battler]];
}

static u32 GetStatusOption(u32 status1)
{
    u32 i;

    if (status1 & STATUS1_SLEEP)
        return STATUS_OPTION_SLEEP;
    for (i = STATUS_OPTION_POISON; i < STATUS_OPTION_COUNT; i++)
    {
        if (status1 & sStatusValues[i])
            return i;
    }
    return STATUS_OPTION_NONE;
}

void CB2_BattleDebugMenu(void)
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
        sBattleDebug = AllocZeroed(sizeof(*sBattleDebug));
        gMain.state++;
        break;
    case 1:
        DmaClearLarge16(3, (void *)VRAM, VRAM_SIZE, 0x1000);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sBgTemplates, ARRAY_COUNT(sBgTemplates));
        ChangeBgX(0, 0, BG_COORD_SET);
        ChangeBgY(0, 0, BG_COORD_SET);
        InitWindows(sWindowTemplates); // also gives BG0 its tilemap buffer
        DeactivateAllTextPrinters();
        LoadPalette(sPalette, BG_PLTT_ID(0), sizeof(sPalette));
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP);
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        gMain.state++;
        break;
    case 2:
        PutWindowTilemap(0);
        BattleDebug_Draw();
        ShowBg(0);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
        SetVBlankCallback(VBlankCB_BattleDebug);
        CreateTask(Task_BattleDebug, 0);
        SetMainCallback2(CB2_BattleDebugRun);
        break;
    }
}

static void CB2_BattleDebugRun(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB_BattleDebug(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void PrintRowValue(u8 row, u8 *dest)
{
    u8 battler = sBattleDebug->battler;
    struct BattlePokemon *mon = &gBattleMons[battler];

    switch (row)
    {
    case ROW_HP:
        dest = ConvertIntToDecimalStringN(dest, mon->hp, STR_CONV_MODE_LEFT_ALIGN, 3);
        dest = StringCopy(dest, sText_Slash);
        ConvertIntToDecimalStringN(dest, mon->maxHP, STR_CONV_MODE_LEFT_ALIGN, 3);
        break;
    case ROW_TP:
        dest = ConvertIntToDecimalStringN(dest, *GetTP(battler), STR_CONV_MODE_LEFT_ALIGN, 3);
        dest = StringCopy(dest, sText_Slash);
        ConvertIntToDecimalStringN(dest, TP_MAX, STR_CONV_MODE_LEFT_ALIGN, 3);
        break;
    case ROW_STATUS:
        StringCopy(dest, sStatusNames[GetStatusOption(mon->status1)]);
        break;
    default:
    {
        s32 stage = mon->statStages[sRowToStat[row]] - DEFAULT_STAT_STAGE;
        if (stage >= 0)
            *dest++ = CHAR_PLUS;
        else
            *dest++ = CHAR_HYPHEN;
        ConvertIntToDecimalStringN(dest, (stage >= 0) ? stage : -stage, STR_CONV_MODE_LEFT_ALIGN, 1);
        break;
    }
    }
}

static void BattleDebug_Draw(void)
{
    u8 battler = sBattleDebug->battler;
    u8 *str;
    u32 row;

    FillWindowPixelBuffer(0, PIXEL_FILL(0));
    AddTextPrinterParameterized3(0, FONT_NORMAL, 0, 0, sColor_Selected, TEXT_SKIP_DRAW, sText_Title);

    str = StringCopy(gStringVar4, gSpeciesNames[gBattleMons[battler].species]);
    StringCopy(str, (GetBattlerSide(battler) == B_SIDE_PLAYER) ? sText_Player : sText_Foe);
    AddTextPrinterParameterized3(0, FONT_NORMAL, 96, 0, sColor_Normal, TEXT_SKIP_DRAW, gStringVar4);

    for (row = 0; row < ROW_COUNT; row++)
    {
        const u8 *color = (row == sBattleDebug->row) ? sColor_Selected : sColor_Normal;
        AddTextPrinterParameterized3(0, FONT_NORMAL, 8, 16 + row * 11, color, TEXT_SKIP_DRAW, sRowNames[row]);
        PrintRowValue(row, gStringVar4);
        AddTextPrinterParameterized3(0, FONT_NORMAL, 80, 16 + row * 11, color, TEXT_SKIP_DRAW, gStringVar4);
    }

    AddTextPrinterParameterized3(0, FONT_SMALL, 0, 131, sColor_Normal, TEXT_SKIP_DRAW, sText_Help);
    CopyWindowToVram(0, COPYWIN_FULL);
}

static s32 Clamp(s32 value, s32 min, s32 max)
{
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

// Applies a change to the selected row, writing the party mon too where the
// healthbox reads it.
static void BattleDebug_Change(s32 delta)
{
    u8 battler = sBattleDebug->battler;
    struct BattlePokemon *mon = &gBattleMons[battler];
    struct Pokemon *partyMon = GetBattlerMon(battler);

    switch (sBattleDebug->row)
    {
    case ROW_HP:
        mon->hp = Clamp((s32)mon->hp + delta, 0, mon->maxHP);
        SetMonData(partyMon, MON_DATA_HP, &mon->hp);
        break;
    case ROW_TP:
        *GetTP(battler) = Clamp((s32)*GetTP(battler) + delta, 0, TP_MAX);
        break;
    case ROW_STATUS:
    {
        s32 option = (GetStatusOption(mon->status1) + (delta > 0 ? 1 : STATUS_OPTION_COUNT - 1)) % STATUS_OPTION_COUNT;
        mon->status1 = sStatusValues[option];
        SetMonData(partyMon, MON_DATA_STATUS, &mon->status1);
        break;
    }
    default:
    {
        u8 stat = sRowToStat[sBattleDebug->row];
        mon->statStages[stat] = Clamp((s32)mon->statStages[stat] + (delta > 0 ? 1 : -1), 0, MAX_STAT_STAGE);
        break;
    }
    }
}

static void Task_BattleDebug(u8 taskId)
{
    s32 step = JOY_HELD(R_BUTTON) ? 10 : 1;

    if (gPaletteFade.active)
        return;

    if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        gTasks[taskId].func = Task_BattleDebugExit;
        return;
    }

    if (JOY_NEW(SELECT_BUTTON))
    {
        sBattleDebug->battler = (sBattleDebug->battler + 1) % gBattlersCount;
    }
    else if (JOY_REPEAT(DPAD_UP))
    {
        sBattleDebug->row = (sBattleDebug->row == 0) ? ROW_COUNT - 1 : sBattleDebug->row - 1;
    }
    else if (JOY_REPEAT(DPAD_DOWN))
    {
        sBattleDebug->row = (sBattleDebug->row + 1) % ROW_COUNT;
    }
    else if (JOY_REPEAT(DPAD_RIGHT))
    {
        BattleDebug_Change(step);
    }
    else if (JOY_REPEAT(DPAD_LEFT))
    {
        BattleDebug_Change(-step);
    }
    else
    {
        return;
    }
    PlaySE(SE_DEX_SCROLL);
    BattleDebug_Draw();
}

static void Task_BattleDebugExit(u8 taskId)
{
    if (gPaletteFade.active)
        return;

    DestroyTask(taskId);
    FreeAllWindowBuffers();
    FREE_AND_SET_NULL(sBattleDebug);
    SetMainCallback2(ReshowBattleScreenAfterMenu);
}

#endif // DEBUG_MENUS
