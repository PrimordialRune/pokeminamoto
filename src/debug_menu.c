#include "global.h"
#include "debug_menu.h"

// Overworld debug menu, opened by holding R and pressing START.
// Not built into release ROMs (`make release`), see DEBUG_MENUS in config.h.
// Inline strings are written (const u8[])_("..."): preproc turns _("...") into a
// byte list, which makes them compound literals. Inside a function those live on
// the stack, so anything kept past the call (like an input's title) is copied.
// Menu state lives on the heap while the menu is open; only gDebugToggles and
// the battle-test script buffer are static, and only in debug builds.

#ifdef DEBUG_MENUS

#include "battle_setup.h"
#include "data.h"
#include "day_night.h"
#include "event_data.h"
#include "event_object_lock.h"
#include "event_object_movement.h"
#include "field_blend.h"
#include "field_scanline.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "gpu_regs.h"
#include "item.h"
#include "list_menu.h"
#include "load_save.h"
#include "main.h"
#include "malloc.h"
#include "menu.h"
#include "money.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "region_map.h"
#include "save.h"
#include "script.h"
#include "script_menu.h"
#include "script_pokemon_util.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "window.h"
#include "constants/battle_setup.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/map_groups.h"
#include "constants/rgb.h"
#include "constants/songs.h"

EWRAM_DATA u8 gDebugToggles = 0;

// trainerbattle / setwildbattle scripts are built here at runtime, because their
// arguments are read from the script itself. It must outlive the menu: the
// script resumes after the battle.
#define DEBUG_SCRIPT_BUFFER_SIZE 12
static EWRAM_DATA u8 sBattleScript[DEBUG_SCRIPT_BUFFER_SIZE] = {0};

// Script opcodes, from data/script_cmd_table.inc
#define SCROP_END           0x02
#define SCROP_TRAINERBATTLE 0x5c
#define SCROP_SETWILDBATTLE 0xb6
#define SCROP_DOWILDBATTLE  0xb7

extern const u8 Debug_Script_1[];
extern const u8 Debug_Script_2[];
extern const u8 Debug_Script_3[];
extern const u8 Debug_Script_4[];

enum
{
    DEBUG_MENU_MAIN,
    DEBUG_MENU_UTILITIES,
    DEBUG_MENU_POKEMON,
    DEBUG_MENU_ITEMS,
    DEBUG_MENU_FLAGS_VARS,
    DEBUG_MENU_BATTLE,
    DEBUG_MENU_SOUND,
    DEBUG_MENU_SCRIPTS,
    DEBUG_MENU_FIELD_FX,
    DEBUG_MENU_COUNT
};

#define DEBUG_MAX_ITEMS      9
#define DEBUG_NAME_LENGTH    24
#define DEBUG_MENU_WIDTH     12
#define DEBUG_INPUT_X        14
#define DEBUG_INPUT_WIDTH    15
#define DEBUG_INPUT_HEIGHT   6
// Tile numbers in BG0, after the ones the field uses. The two windows need their own.
#define DEBUG_MENU_BASE      100
#define DEBUG_INPUT_BASE     (DEBUG_MENU_BASE + DEBUG_MENU_WIDTH * DEBUG_MAX_ITEMS * 2)

typedef void (*DebugAction)(void);
typedef void (*DebugDescribe)(u32 value, u8 *dest);
typedef void (*DebugConfirm)(u32 value);

struct DebugMenuEntry
{
    const u8 *name;
    DebugAction action;   // NULL for entries that open a submenu
    u8 submenu;
    u8 toggle;            // when not TOGGLE_NONE, ON/OFF is shown after the name
};

struct DebugNumberInput
{
    u8 title[DEBUG_NAME_LENGTH]; // copied: inline strings inside functions live on the stack
    u32 value;
    u32 min;
    u32 max;
    u8 digits;
    u8 digit;             // 0 = ones
    bool8 stayOpen;       // A applies the value and keeps the input open (flags, sounds)
    DebugDescribe describe;
    DebugConfirm confirm;
};

struct DebugMenu
{
    u8 menu;
    u8 menuWindowId;
    u8 inputWindowId;
    u8 listTaskId;
    u16 cursor[DEBUG_MENU_COUNT];
    struct ListMenuItem items[DEBUG_MAX_ITEMS];
    u8 names[DEBUG_MAX_ITEMS][DEBUG_NAME_LENGTH];
    struct DebugNumberInput input;
    u32 scratch[2];       // values from earlier steps of multi-step inputs
    DebugAction afterClose;
};

static EWRAM_DATA struct DebugMenu *sDebug = NULL;

enum
{
    TOGGLE_NONE,
    TOGGLE_COLLISION,
    TOGGLE_ENCOUNTERS,
    TOGGLE_TRAINER_SIGHT,
    TOGGLE_RUNNING_SHOES,
    TOGGLE_POKEDEX,
    TOGGLE_BADGES,
    TOGGLE_HEAT_HAZE,
    TOGGLE_WATER,
    TOGGLE_GRADIENT,
    TOGGLE_GHOSTS,
    TOGGLE_LANTERN,
};

static void Task_DebugMenu(u8 taskId);
static void Task_DebugNumberInput(u8 taskId);
static void DebugMenu_Open(u8 menu);
static void DebugMenu_Close(void);
static void DebugMenu_Refresh(void);
static void DebugInput_Start(const u8 *title, u32 initial, u32 min, u32 max, DebugDescribe describe, DebugConfirm confirm, bool8 stayOpen);
static void DebugInput_Draw(void);
static void DebugInput_Close(void);
static void DebugMessage(const u8 *str);

static void DebugAction_HealParty(void);
static void DebugAction_Warp(void);
static void DebugAction_Money(void);
static void DebugAction_SaveSpace(void);
static void DebugAction_ToggleCollision(void);
static void DebugAction_ToggleEncounters(void);
static void DebugAction_ToggleTrainerSight(void);
static void DebugAction_ToggleRunningShoes(void);
static void DebugAction_GiveMon(void);
static void DebugAction_DexAll(void);
static void DebugAction_DexReset(void);
static void DebugAction_TogglePokedex(void);
static void DebugAction_GiveItem(void);
static void DebugAction_GiveAllTMs(void);
static void DebugAction_SetFlag(void);
static void DebugAction_SetVar(void);
static void DebugAction_ToggleBadges(void);
static void DebugAction_WildBattle(void);
static void DebugAction_TrainerBattle(void);
static void DebugAction_PlayMusic(void);
static void DebugAction_PlaySE(void);
static void DebugAction_StopMusic(void);
static void DebugAction_Script1(void);
static void DebugAction_Script2(void);
static void DebugAction_Script3(void);
static void DebugAction_Script4(void);
static void DebugAction_TimeOfDay(void);
static void DebugAction_ToggleHeatHaze(void);
static void DebugAction_ToggleWater(void);
static void DebugAction_ToggleGradient(void);
static void DebugAction_ToggleGhosts(void);
static void DebugAction_ToggleLantern(void);

static const struct DebugMenuEntry sMenu_Main[] =
{
    {(const u8[])_("Utilities"),     NULL, DEBUG_MENU_UTILITIES},
    {(const u8[])_("Pokémon"),       NULL, DEBUG_MENU_POKEMON},
    {(const u8[])_("Items"),         NULL, DEBUG_MENU_ITEMS},
    {(const u8[])_("Flags & Vars"),  NULL, DEBUG_MENU_FLAGS_VARS},
    {(const u8[])_("Battle test"),   NULL, DEBUG_MENU_BATTLE},
    {(const u8[])_("Sound"),         NULL, DEBUG_MENU_SOUND},
    {(const u8[])_("Scripts"),       NULL, DEBUG_MENU_SCRIPTS},
    {(const u8[])_("Field FX"),      NULL, DEBUG_MENU_FIELD_FX},
};

static const struct DebugMenuEntry sMenu_Utilities[] =
{
    {(const u8[])_("Heal party"),    DebugAction_HealParty},
    {(const u8[])_("Warp…"),         DebugAction_Warp},
    {(const u8[])_("Set money…"),    DebugAction_Money},
    {(const u8[])_("Save space"),    DebugAction_SaveSpace},
    {(const u8[])_("Walls"),         DebugAction_ToggleCollision, 0, TOGGLE_COLLISION},
    {(const u8[])_("Wild mons"),     DebugAction_ToggleEncounters, 0, TOGGLE_ENCOUNTERS},
    {(const u8[])_("Trainer eyes"),  DebugAction_ToggleTrainerSight, 0, TOGGLE_TRAINER_SIGHT},
    {(const u8[])_("Run shoes"),     DebugAction_ToggleRunningShoes, 0, TOGGLE_RUNNING_SHOES},
};

static const struct DebugMenuEntry sMenu_Pokemon[] =
{
    {(const u8[])_("Give Pokémon…"), DebugAction_GiveMon},
    {(const u8[])_("Dex: all 151"),  DebugAction_DexAll},
    {(const u8[])_("Dex: reset"),    DebugAction_DexReset},
    {(const u8[])_("Pokédex"),       DebugAction_TogglePokedex, 0, TOGGLE_POKEDEX},
};

static const struct DebugMenuEntry sMenu_Items[] =
{
    {(const u8[])_("Give item…"),    DebugAction_GiveItem},
    {(const u8[])_("All TMs & HMs"), DebugAction_GiveAllTMs},
};

static const struct DebugMenuEntry sMenu_FlagsVars[] =
{
    {(const u8[])_("Flag…"),         DebugAction_SetFlag},
    {(const u8[])_("Var…"),          DebugAction_SetVar},
    {(const u8[])_("All badges"),    DebugAction_ToggleBadges, 0, TOGGLE_BADGES},
};

static const struct DebugMenuEntry sMenu_Battle[] =
{
    {(const u8[])_("Wild battle…"),  DebugAction_WildBattle},
    {(const u8[])_("Trainer…"),      DebugAction_TrainerBattle},
};

static const struct DebugMenuEntry sMenu_Sound[] =
{
    {(const u8[])_("Music…"),        DebugAction_PlayMusic},
    {(const u8[])_("Sound effect…"), DebugAction_PlaySE},
    {(const u8[])_("Map music"),     DebugAction_StopMusic},
};

static const struct DebugMenuEntry sMenu_Scripts[] =
{
    {(const u8[])_("Script 1"),      DebugAction_Script1},
    {(const u8[])_("Script 2"),      DebugAction_Script2},
    {(const u8[])_("Script 3"),      DebugAction_Script3},
    {(const u8[])_("Script 4"),      DebugAction_Script4},
};

static const struct DebugMenuEntry sMenu_FieldFx[] =
{
    {(const u8[])_("Time of day"),   DebugAction_TimeOfDay},
    {(const u8[])_("Heat haze"),     DebugAction_ToggleHeatHaze, 0, TOGGLE_HEAT_HAZE},
    {(const u8[])_("Water"),         DebugAction_ToggleWater, 0, TOGGLE_WATER},
    {(const u8[])_("Gradient"),      DebugAction_ToggleGradient, 0, TOGGLE_GRADIENT},
    {(const u8[])_("Ghosts"),        DebugAction_ToggleGhosts, 0, TOGGLE_GHOSTS},
    {(const u8[])_("Lantern"),       DebugAction_ToggleLantern, 0, TOGGLE_LANTERN},
};

static const struct
{
    const struct DebugMenuEntry *entries;
    u8 count;
    u8 parent;
} sMenus[DEBUG_MENU_COUNT] =
{
    [DEBUG_MENU_MAIN]       = {sMenu_Main,       ARRAY_COUNT(sMenu_Main),       DEBUG_MENU_MAIN},
    [DEBUG_MENU_UTILITIES]  = {sMenu_Utilities,  ARRAY_COUNT(sMenu_Utilities),  DEBUG_MENU_MAIN},
    [DEBUG_MENU_POKEMON]    = {sMenu_Pokemon,    ARRAY_COUNT(sMenu_Pokemon),    DEBUG_MENU_MAIN},
    [DEBUG_MENU_ITEMS]      = {sMenu_Items,      ARRAY_COUNT(sMenu_Items),      DEBUG_MENU_MAIN},
    [DEBUG_MENU_FLAGS_VARS] = {sMenu_FlagsVars,  ARRAY_COUNT(sMenu_FlagsVars),  DEBUG_MENU_MAIN},
    [DEBUG_MENU_BATTLE]     = {sMenu_Battle,     ARRAY_COUNT(sMenu_Battle),     DEBUG_MENU_MAIN},
    [DEBUG_MENU_SOUND]      = {sMenu_Sound,      ARRAY_COUNT(sMenu_Sound),      DEBUG_MENU_MAIN},
    [DEBUG_MENU_SCRIPTS]    = {sMenu_Scripts,    ARRAY_COUNT(sMenu_Scripts),    DEBUG_MENU_MAIN},
    [DEBUG_MENU_FIELD_FX]   = {sMenu_FieldFx,    ARRAY_COUNT(sMenu_FieldFx),    DEBUG_MENU_MAIN},
};

static const struct ListMenuTemplate sDebugListTemplate =
{
    .item_X = 8,
    .upText_Y = 1,
    .cursorPal = 2,
    .fillValue = 1,
    .cursorShadowPal = 3,
    .lettersSpacing = 1,
    .scrollMultiple = LIST_NO_MULTIPLE_SCROLL,
    .fontId = FONT_NORMAL,
};

static const u8 sColor_Normal[] = {TEXT_COLOR_WHITE, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_LIGHT_GRAY};
static const u8 sColor_Selected[] = {TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_LIGHT_RED};

// ---------------------------------------------------------------------------
// Opening, closing and the list
// ---------------------------------------------------------------------------

void Debug_ShowMainMenu(void)
{
    FreezeObjectEvents();
    PlayerFreeze();
    StopPlayerAvatar();
    LockPlayerFieldControls();
    PlaySE(SE_WIN_OPEN);

    sDebug = AllocZeroed(sizeof(*sDebug));
    sDebug->inputWindowId = WINDOW_NONE;
    LoadMessageBoxAndBorderGfx();
    DebugMenu_Open(DEBUG_MENU_MAIN);
    CreateTask(Task_DebugMenu, 80);
}

static u8 AddDebugWindow(u8 x, u8 width, u8 height, u16 baseBlock)
{
    struct WindowTemplate template = CreateWindowTemplate(0, x + 1, 1, width, height, 15, baseBlock);
    u8 windowId = AddWindow(&template);
    PutWindowTilemap(windowId);
    return windowId;
}

static bool8 GetToggleState(u8 toggle)
{
    switch (toggle)
    {
    case TOGGLE_COLLISION:
        return !DebugToggleOn(DEBUG_TOGGLE_NO_COLLISION);
    case TOGGLE_ENCOUNTERS:
        return !DebugToggleOn(DEBUG_TOGGLE_NO_ENCOUNTERS);
    case TOGGLE_TRAINER_SIGHT:
        return !DebugToggleOn(DEBUG_TOGGLE_NO_TRAINER_SIGHT);
    case TOGGLE_RUNNING_SHOES:
        return FlagGet(FLAG_SYS_B_DASH);
    case TOGGLE_POKEDEX:
        return FlagGet(FLAG_SYS_POKEDEX_GET);
    case TOGGLE_BADGES:
        return FlagGet(FLAG_BADGE01_GET);
    case TOGGLE_HEAT_HAZE:
        return FieldScanline_GetEffect() == FIELD_SCANLINE_HEAT_HAZE;
    case TOGGLE_WATER:
        return FieldScanline_GetEffect() == FIELD_SCANLINE_WATER;
    case TOGGLE_GRADIENT:
        return FieldScanline_GetEffect() == FIELD_SCANLINE_GRADIENT;
    case TOGGLE_GHOSTS:
        // A map load resets WININ, and the sprites with it
        return (GetGpuReg(REG_OFFSET_WININ) & WININ_WIN0_CLR) != 0;
    case TOGGLE_LANTERN:
        return FieldLight_IsActive();
    }
    return FALSE;
}

static void DebugMenu_Open(u8 menu)
{
    u32 i;
    const struct DebugMenuEntry *entries = sMenus[menu].entries;
    u8 count = sMenus[menu].count;

    sDebug->menu = menu;
    for (i = 0; i < count; i++)
    {
        u8 *name = StringCopy(sDebug->names[i], entries[i].name);
        if (entries[i].toggle != TOGGLE_NONE)
            StringCopy(name, GetToggleState(entries[i].toggle) ? (const u8[])_(": ON") : (const u8[])_(": OFF"));
        else if (entries[i].action == NULL)
            StringCopy(name, (const u8[])_(" ▶"));
        sDebug->items[i].name = sDebug->names[i];
        sDebug->items[i].id = i;
    }

    sDebug->menuWindowId = AddDebugWindow(0, DEBUG_MENU_WIDTH, count * 2, DEBUG_MENU_BASE);
    SetStandardWindowBorderStyle(sDebug->menuWindowId, FALSE);

    gMultiuseListMenuTemplate = sDebugListTemplate;
    gMultiuseListMenuTemplate.windowId = sDebug->menuWindowId;
    gMultiuseListMenuTemplate.items = sDebug->items;
    gMultiuseListMenuTemplate.totalItems = count;
    gMultiuseListMenuTemplate.maxShowed = count;
    sDebug->listTaskId = ListMenuInit(&gMultiuseListMenuTemplate, 0, sDebug->cursor[menu]);
    CopyWindowToVram(sDebug->menuWindowId, COPYWIN_FULL);
}

static void DebugMenu_Destroy(void)
{
    u16 scroll;

    DestroyListMenuTask(sDebug->listTaskId, &scroll, &sDebug->cursor[sDebug->menu]);
    ClearStdWindowAndFrameToTransparent(sDebug->menuWindowId, TRUE);
    RemoveWindow(sDebug->menuWindowId);
}

// Redraws the current menu, e.g. after a toggle changed.
static void DebugMenu_Refresh(void)
{
    u8 menu = sDebug->menu;
    DebugMenu_Destroy();
    DebugMenu_Open(menu);
}

static void DebugMenu_Close(void)
{
    DebugAction afterClose = sDebug->afterClose;

    DebugInput_Close();
    DebugMenu_Destroy();
    FREE_AND_SET_NULL(sDebug);

    if (afterClose != NULL)
    {
        // Warps and battles take over from here and unlock the player themselves
        afterClose();
    }
    else
    {
        ScriptUnfreezeObjectEvents();
        UnlockPlayerFieldControls();
    }
}

#define tOpened data[0]

static void Task_DebugMenu(u8 taskId)
{
    s32 input;

    if (sDebug->inputWindowId != WINDOW_NONE && FuncIsActiveTask(Task_DebugNumberInput))
        return;

    // A number input confirmed a warp or battle: close the menu and run it
    if (sDebug->afterClose != NULL)
    {
        DebugMenu_Close();
        DestroyTask(taskId);
        return;
    }

    // The START press that opened the menu is still "new" on the first frame
    if (!gTasks[taskId].tOpened)
    {
        gTasks[taskId].tOpened = TRUE;
        return;
    }

    input = ListMenu_ProcessInput(sDebug->listTaskId);
    if (JOY_NEW(START_BUTTON))
        input = LIST_CANCEL;

    switch (input)
    {
    case LIST_NOTHING_CHOSEN:
        break;
    case LIST_CANCEL:
        PlaySE(SE_SELECT);
        if (sDebug->menu == DEBUG_MENU_MAIN || JOY_NEW(START_BUTTON))
        {
            DebugMenu_Close();
            DestroyTask(taskId);
        }
        else
        {
            DebugInput_Close();
            u8 parent = sMenus[sDebug->menu].parent;
            DebugMenu_Destroy();
            DebugMenu_Open(parent);
        }
        break;
    default:
    {
        const struct DebugMenuEntry *entry = &sMenus[sDebug->menu].entries[input];
        PlaySE(SE_SELECT);
        DebugInput_Close();
        if (entry->action == NULL)
        {
            DebugMenu_Destroy();
            DebugMenu_Open(entry->submenu);
        }
        else
        {
            entry->action();
            if (sDebug->afterClose != NULL)
            {
                DebugMenu_Close();
                DestroyTask(taskId);
            }
        }
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// Number input: UP/DOWN change the selected digit, LEFT/RIGHT pick the digit,
// A confirms, B goes back to the list.
// ---------------------------------------------------------------------------

static u8 CountDigits(u32 value)
{
    u8 digits = 1;
    while (value >= 10)
    {
        value /= 10;
        digits++;
    }
    return digits;
}

static const u32 sPowersOfTen[] = {1, 10, 100, 1000, 10000, 100000, 1000000};

static void DebugInput_Start(const u8 *title, u32 initial, u32 min, u32 max, DebugDescribe describe, DebugConfirm confirm, bool8 stayOpen)
{
    struct DebugNumberInput *in = &sDebug->input;

    StringCopy(in->title, title);
    in->min = min;
    in->max = max;
    in->value = (initial < min) ? min : (initial > max ? max : initial);
    in->digits = CountDigits(max);
    in->digit = 0;
    in->describe = describe;
    in->confirm = confirm;
    in->stayOpen = stayOpen;

    if (sDebug->inputWindowId == WINDOW_NONE)
    {
        sDebug->inputWindowId = AddDebugWindow(DEBUG_INPUT_X, DEBUG_INPUT_WIDTH, DEBUG_INPUT_HEIGHT, DEBUG_INPUT_BASE);
        SetStandardWindowBorderStyle(sDebug->inputWindowId, FALSE);
    }
    DebugInput_Draw();
    CreateTask(Task_DebugNumberInput, 81);
}

static void DebugInput_Draw(void)
{
    struct DebugNumberInput *in = &sDebug->input;
    u8 windowId = sDebug->inputWindowId;
    u8 digitStr[2];
    s32 i;

    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    AddTextPrinterParameterized3(windowId, FONT_NORMAL, 0, 1, sColor_Normal, TEXT_SKIP_DRAW, in->title);

    // One digit at a time so the selected one can be drawn in red
    digitStr[1] = EOS;
    for (i = in->digits - 1; i >= 0; i--)
    {
        digitStr[0] = CHAR_0 + (in->value / sPowersOfTen[i]) % 10;
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, 6 * (in->digits - 1 - i), 17,
                                     (i == in->digit) ? sColor_Selected : sColor_Normal, TEXT_SKIP_DRAW, digitStr);
    }

    if (in->describe != NULL)
    {
        gStringVar4[0] = EOS;
        in->describe(in->value, gStringVar4);
        AddTextPrinterParameterized3(windowId, FONT_NORMAL, 0, 33, sColor_Normal, TEXT_SKIP_DRAW, gStringVar4);
    }
    CopyWindowToVram(windowId, COPYWIN_FULL);
}

// Shows a one-line result in the input window.
static void DebugMessage(const u8 *str)
{
    if (sDebug->inputWindowId == WINDOW_NONE)
    {
        sDebug->inputWindowId = AddDebugWindow(DEBUG_INPUT_X, DEBUG_INPUT_WIDTH, DEBUG_INPUT_HEIGHT, DEBUG_INPUT_BASE);
        SetStandardWindowBorderStyle(sDebug->inputWindowId, FALSE);
    }
    FillWindowPixelBuffer(sDebug->inputWindowId, PIXEL_FILL(1));
    AddTextPrinterParameterized3(sDebug->inputWindowId, FONT_NORMAL, 0, 1, sColor_Normal, TEXT_SKIP_DRAW, str);
    CopyWindowToVram(sDebug->inputWindowId, COPYWIN_FULL);
}

static void DebugInput_Close(void)
{
    u8 taskId = FindTaskIdByFunc(Task_DebugNumberInput);
    if (taskId != TASK_NONE)
        DestroyTask(taskId);
    if (sDebug->inputWindowId != WINDOW_NONE)
    {
        ClearStdWindowAndFrameToTransparent(sDebug->inputWindowId, TRUE);
        RemoveWindow(sDebug->inputWindowId);
        sDebug->inputWindowId = WINDOW_NONE;
    }
}

static void Task_DebugNumberInput(u8 taskId)
{
    struct DebugNumberInput *in = &sDebug->input;
    u32 step = sPowersOfTen[in->digit];

    // The A press that opened the input is still "new" on the first frame
    if (!gTasks[taskId].tOpened)
    {
        gTasks[taskId].tOpened = TRUE;
        return;
    }

    if (JOY_NEW(A_BUTTON))
    {
        PlaySE(SE_SELECT);
        if (in->stayOpen)
        {
            in->confirm(in->value);
            DebugInput_Draw();
        }
        else
        {
            DestroyTask(taskId);
            in->confirm(in->value);
        }
    }
    else if (JOY_NEW(B_BUTTON))
    {
        PlaySE(SE_SELECT);
        DestroyTask(taskId);
        DebugInput_Close();
    }
    else if (JOY_REPEAT(DPAD_UP))
    {
        in->value = (in->value + step > in->max) ? in->max : in->value + step;
        PlaySE(SE_DEX_SCROLL);
        DebugInput_Draw();
    }
    else if (JOY_REPEAT(DPAD_DOWN))
    {
        in->value = (in->value < in->min + step) ? in->min : in->value - step;
        PlaySE(SE_DEX_SCROLL);
        DebugInput_Draw();
    }
    else if (JOY_NEW(DPAD_LEFT))
    {
        if (in->digit + 1 < in->digits)
            in->digit++;
        DebugInput_Draw();
    }
    else if (JOY_NEW(DPAD_RIGHT))
    {
        if (in->digit > 0)
            in->digit--;
        DebugInput_Draw();
    }
}

// ---------------------------------------------------------------------------
// Value descriptions shown under the number
// ---------------------------------------------------------------------------

static void Describe_Species(u32 value, u8 *dest)
{
    StringCopy(dest, gSpeciesNames[value]);
}

static void Describe_Item(u32 value, u8 *dest)
{
    CopyItemName(value, dest);
}

static void Describe_Flag(u32 value, u8 *dest)
{
    StringCopy(dest, FlagGet(value) ? (const u8[])_("ON  (A toggles)") : (const u8[])_("OFF (A toggles)"));
}

static void Describe_Var(u32 value, u8 *dest)
{
    u8 *str = StringCopy(dest, (const u8[])_("now "));
    ConvertIntToDecimalStringN(str, VarGet(value), STR_CONV_MODE_LEFT_ALIGN, 5);
}

static void Describe_Trainer(u32 value, u8 *dest)
{
    StringCopy(dest, gTrainers[value].trainerName);
}

static const u8 sMapGroupSizes[MAP_GROUPS_COUNT] = MAP_GROUP_SIZES;

// Only maps from the generated map group tables can be a warp target: the group and
// number must be inside those tables (the removed-map groups 0x7B-0x7E in removed_maps.h
// are not real), and the link rooms are skipped because they need a link partner.
static bool8 Warp_IsValidMap(u32 group, u32 num)
{
    u32 map;

    if (group >= MAP_GROUPS_COUNT || num >= sMapGroupSizes[group])
        return FALSE;
    map = (group << 8) | num;
    if (map == MAP_BATTLE_COLOSSEUM_2P || map == MAP_BATTLE_COLOSSEUM_4P
     || map == MAP_TRADE_CENTER || map == MAP_RECORD_CORNER || map == MAP_UNION_ROOM)
        return FALSE;
    return TRUE;
}

static void Describe_MapGroup(u32 value, u8 *dest)
{
    u8 *str = StringCopy(dest, (const u8[])_("maps: "));
    ConvertIntToDecimalStringN(str, value < MAP_GROUPS_COUNT ? sMapGroupSizes[value] : 0, STR_CONV_MODE_LEFT_ALIGN, 3);
}

static void Describe_Map(u32 value, u8 *dest)
{
    const struct MapHeader *header;

    if (!Warp_IsValidMap(sDebug->scratch[0], value))
    {
        StringCopy(dest, (const u8[])_("(not available)"));
        return;
    }
    header = Overworld_GetMapHeaderByGroupAndId(sDebug->scratch[0], value);
    GetMapName(dest, header->regionMapSectionId, 0);
}

// ---------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------

static void DebugAction_HealParty(void)
{
    HealPlayerParty();
    PlaySE(SE_USE_ITEM);
    DebugMessage((const u8[])_("Party healed."));
}

static void Warp_AfterClose(void)
{
    const struct MapHeader *header;
    s8 group = sBattleScript[0], num = sBattleScript[1];
    const struct WarpEvent *warp;

    if (!Warp_IsValidMap(group, num))
        return;
    header = Overworld_GetMapHeaderByGroupAndId(group, num);
    warp = header->events->warps;

    // Arrive on warp 0 when the map has one and it lies inside the map, else in the middle of the map
    if (header->events->warpCount > 0 && warp[0].x >= 0 && warp[0].y >= 0
     && warp[0].x < header->mapLayout->width && warp[0].y < header->mapLayout->height)
        SetWarpDestinationToMapWarp(group, num, 0);
    else
        SetWarpDestination(group, num, WARP_ID_NONE, header->mapLayout->width / 2, header->mapLayout->height / 2);
    DoWarp();
}

static void Warp_ConfirmMap(u32 value)
{
    if (!Warp_IsValidMap(sDebug->scratch[0], value))
    {
        DebugMessage((const u8[])_("No such map."));
        return;
    }
    // The menu's heap is freed before the warp, so the destination goes in the static buffer
    sBattleScript[0] = sDebug->scratch[0];
    sBattleScript[1] = value;
    sDebug->afterClose = Warp_AfterClose;
}

static void Warp_ConfirmGroup(u32 value)
{
    if (value >= MAP_GROUPS_COUNT)
    {
        DebugMessage((const u8[])_("No such group."));
        return;
    }
    sDebug->scratch[0] = value;
    DebugInput_Start((const u8[])_("Map number"), 0, 0, sMapGroupSizes[value] - 1, Describe_Map, Warp_ConfirmMap, FALSE);
}

static void DebugAction_Warp(void)
{
    DebugInput_Start((const u8[])_("Map group"), gSaveBlock1Ptr->location.mapGroup, 0, MAP_GROUPS_COUNT - 1, Describe_MapGroup, Warp_ConfirmGroup, FALSE);
}

static void Money_Confirm(u32 value)
{
    SetMoney(&gSaveBlock1Ptr->money, value);
    PlaySE(SE_SHOP);
    DebugMessage((const u8[])_("Money set."));
}

static void DebugAction_Money(void)
{
    DebugInput_Start((const u8[])_("Money"), GetMoney(&gSaveBlock1Ptr->money), 0, 999999, NULL, Money_Confirm, FALSE);
}

// Shows how full each save block is: what's left is what new save data can use.
static void DebugAction_SaveSpace(void)
{
    u8 *str = gStringVar4;
    str = StringCopy(str, (const u8[])_("Save1 "));
    str = ConvertIntToDecimalStringN(str, sizeof(struct SaveBlock1), STR_CONV_MODE_LEFT_ALIGN, 5);
    str = StringCopy(str, (const u8[])_("/"));
    str = ConvertIntToDecimalStringN(str, SECTOR_DATA_SIZE * (SECTOR_ID_SAVEBLOCK1_END - SECTOR_ID_SAVEBLOCK1_START + 1), STR_CONV_MODE_LEFT_ALIGN, 5);
    str = StringCopy(str, (const u8[])_("\nSave2 "));
    str = ConvertIntToDecimalStringN(str, sizeof(struct SaveBlock2), STR_CONV_MODE_LEFT_ALIGN, 5);
    str = StringCopy(str, (const u8[])_("/"));
    str = ConvertIntToDecimalStringN(str, SECTOR_DATA_SIZE, STR_CONV_MODE_LEFT_ALIGN, 5);
    str = StringCopy(str, (const u8[])_("\nBoxes "));
    str = ConvertIntToDecimalStringN(str, sizeof(struct PokemonStorage), STR_CONV_MODE_LEFT_ALIGN, 5);
    str = StringCopy(str, (const u8[])_("/"));
    ConvertIntToDecimalStringN(str, SECTOR_DATA_SIZE * (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START + 1), STR_CONV_MODE_LEFT_ALIGN, 5);
    DebugMessage(gStringVar4);
}

static void ToggleDebugBit(u8 bit)
{
    gDebugToggles ^= bit;
    DebugMenu_Refresh();
}

static void DebugAction_ToggleCollision(void)
{
    ToggleDebugBit(DEBUG_TOGGLE_NO_COLLISION);
}

static void DebugAction_ToggleEncounters(void)
{
    ToggleDebugBit(DEBUG_TOGGLE_NO_ENCOUNTERS);
}

static void DebugAction_ToggleTrainerSight(void)
{
    ToggleDebugBit(DEBUG_TOGGLE_NO_TRAINER_SIGHT);
}

static void ToggleFlag(u16 flag)
{
    if (FlagGet(flag))
        FlagClear(flag);
    else
        FlagSet(flag);
}

static void DebugAction_ToggleRunningShoes(void)
{
    ToggleFlag(FLAG_SYS_B_DASH);
    DebugMenu_Refresh();
}

// ---------------------------------------------------------------------------
// Pokémon
// ---------------------------------------------------------------------------

static void GiveMon_ConfirmLevel(u32 value)
{
    u8 *str;

    FlagSet(FLAG_SYS_POKEMON_GET);
    if (ScriptGiveMon(sDebug->scratch[0], value, ITEM_NONE, 0, 0, 0) == MON_CANT_GIVE)
    {
        DebugMessage((const u8[])_("Party and boxes\nare full."));
        return;
    }
    str = StringCopy(gStringVar4, gSpeciesNames[sDebug->scratch[0]]);
    str = StringCopy(str, (const u8[])_(" Lv. "));
    ConvertIntToDecimalStringN(str, value, STR_CONV_MODE_LEFT_ALIGN, 3);
    StringAppend(gStringVar4, (const u8[])_("\nadded."));
    PlaySE(SE_SHOP);
    DebugMessage(gStringVar4);
}

static void GiveMon_ConfirmSpecies(u32 value)
{
    sDebug->scratch[0] = value;
    DebugInput_Start((const u8[])_("Level"), 5, 1, MAX_LEVEL, NULL, GiveMon_ConfirmLevel, FALSE);
}

static void DebugAction_GiveMon(void)
{
    DebugInput_Start((const u8[])_("Species"), sDebug->scratch[0] ? sDebug->scratch[0] : SPECIES_BULBASAUR,
                     SPECIES_BULBASAUR, SPECIES_GEN1_MAX, Describe_Species, GiveMon_ConfirmSpecies, FALSE);
}

static void DebugAction_DexAll(void)
{
    u32 species;

    for (species = SPECIES_BULBASAUR; species <= SPECIES_GEN1_MAX; species++)
    {
        u16 dexNum = SpeciesToNationalPokedexNum(species);
        GetSetPokedexFlag(dexNum, FLAG_SET_SEEN);
        GetSetPokedexFlag(dexNum, FLAG_SET_CAUGHT);
    }
    PlaySE(SE_SHOP);
    DebugMessage((const u8[])_("All 151 seen\nand caught."));
}

static void DebugAction_DexReset(void)
{
    ResetPokedex();
    PlaySE(SE_SELECT);
    DebugMessage((const u8[])_("Pokédex cleared."));
}

static void DebugAction_TogglePokedex(void)
{
    ToggleFlag(FLAG_SYS_POKEDEX_GET);
    DebugMenu_Refresh();
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

static void GiveItem_ConfirmCount(u32 value)
{
    u8 *str;

    if (!AddBagItem(sDebug->scratch[0], value))
    {
        DebugMessage((const u8[])_("No room in the bag."));
        return;
    }
    str = ConvertIntToDecimalStringN(gStringVar4, value, STR_CONV_MODE_LEFT_ALIGN, 3);
    *str++ = CHAR_SPACE;
    CopyItemName(sDebug->scratch[0], str);
    StringAppend(gStringVar4, (const u8[])_("\nadded."));
    PlaySE(SE_SHOP);
    DebugMessage(gStringVar4);
}

static void GiveItem_ConfirmItem(u32 value)
{
    sDebug->scratch[0] = value;
    DebugInput_Start((const u8[])_("How many"), 1, 1, MAX_BAG_ITEM_CAPACITY, NULL, GiveItem_ConfirmCount, FALSE);
}

static void DebugAction_GiveItem(void)
{
    DebugInput_Start((const u8[])_("Item"), sDebug->scratch[0] ? sDebug->scratch[0] : ITEM_POTION,
                     1, ITEMS_COUNT - 1, Describe_Item, GiveItem_ConfirmItem, FALSE);
}

static void DebugAction_GiveAllTMs(void)
{
    u32 item;

    for (item = ITEM_TM01; item <= ITEM_HM08; item++)
    {
        if (!CheckBagHasItem(item, 1))
            AddBagItem(item, 1);
    }
    PlaySE(SE_SHOP);
    DebugMessage((const u8[])_("TM01-50 and\nHM01-08 added."));
}

// ---------------------------------------------------------------------------
// Flags and vars
// ---------------------------------------------------------------------------

static void Flag_Confirm(u32 value)
{
    ToggleFlag(value);
}

static void DebugAction_SetFlag(void)
{
    DebugInput_Start((const u8[])_("Flag"), 1, 1, FLAGS_COUNT - 1, Describe_Flag, Flag_Confirm, TRUE);
}

static void Var_ConfirmValue(u32 value)
{
    VarSet(sDebug->scratch[0], value);
    PlaySE(SE_SELECT);
    DebugMessage((const u8[])_("Var set."));
}

static void Var_ConfirmId(u32 value)
{
    sDebug->scratch[0] = value;
    DebugInput_Start((const u8[])_("Value"), VarGet(value), 0, 0xFFFF, NULL, Var_ConfirmValue, FALSE);
}

static void DebugAction_SetVar(void)
{
    DebugInput_Start((const u8[])_("Var"), VARS_START, VARS_START, VARS_END, Describe_Var, Var_ConfirmId, FALSE);
}

static void DebugAction_ToggleBadges(void)
{
    u32 flag;
    bool8 set = !FlagGet(FLAG_BADGE01_GET);

    for (flag = FLAG_BADGE01_GET; flag <= FLAG_BADGE08_GET; flag++)
    {
        if (set)
            FlagSet(flag);
        else
            FlagClear(flag);
    }
    DebugMenu_Refresh();
}

// ---------------------------------------------------------------------------
// Battle tests. Both run through a script built in sBattleScript, so the
// battle returns to the field the same way a map script's battle does.
// ---------------------------------------------------------------------------

static void RunBattleScript(void)
{
    ScriptContext_SetupScript(sBattleScript);
}

static void WildBattle_ConfirmLevel(u32 value)
{
    u16 species = sDebug->scratch[0];
    u8 *script = sBattleScript;

    *script++ = SCROP_SETWILDBATTLE;
    *script++ = species & 0xFF;
    *script++ = species >> 8;
    *script++ = value;
    *script++ = ITEM_NONE & 0xFF;
    *script++ = ITEM_NONE >> 8;
    *script++ = SCROP_DOWILDBATTLE;
    *script++ = SCROP_END;
    sDebug->afterClose = RunBattleScript;
}

static void WildBattle_ConfirmSpecies(u32 value)
{
    sDebug->scratch[0] = value;
    DebugInput_Start((const u8[])_("Level"), 5, 1, MAX_LEVEL, NULL, WildBattle_ConfirmLevel, FALSE);
}

static void DebugAction_WildBattle(void)
{
    DebugInput_Start((const u8[])_("Wild species"), sDebug->scratch[0] ? sDebug->scratch[0] : SPECIES_PIDGEY,
                     SPECIES_BULBASAUR, SPECIES_GEN1_MAX, Describe_Species, WildBattle_ConfirmSpecies, FALSE);
}

static const u8 sText_DebugTrainerDefeat[] = _("Debug battle over.");

static void TrainerBattle_Confirm(u32 value)
{
    u8 *script = sBattleScript;
    u32 text = (u32)sText_DebugTrainerDefeat;

    // trainerbattle_no_intro: no flag check, so the same trainer can be fought again
    *script++ = SCROP_TRAINERBATTLE;
    *script++ = TRAINER_BATTLE_SINGLE_NO_INTRO_TEXT;
    *script++ = value & 0xFF;
    *script++ = value >> 8;
    *script++ = LOCALID_NONE & 0xFF;
    *script++ = LOCALID_NONE >> 8;
    *script++ = text;
    *script++ = text >> 8;
    *script++ = text >> 16;
    *script++ = text >> 24;
    *script++ = SCROP_END;
    sDebug->afterClose = RunBattleScript;
}

static void DebugAction_TrainerBattle(void)
{
    DebugInput_Start((const u8[])_("Trainer"), 1, 1, TRAINERS_COUNT - 1, Describe_Trainer, TrainerBattle_Confirm, FALSE);
}

// ---------------------------------------------------------------------------
// Sound
// ---------------------------------------------------------------------------

static void Music_Confirm(u32 value)
{
    PlayBGM(value);
}

static void DebugAction_PlayMusic(void)
{
    DebugInput_Start((const u8[])_("Music"), MUS_ROUTE101, MUS_LITTLEROOT_TEST, MUS_RG_TEACHY_TV_MENU, NULL, Music_Confirm, TRUE);
}

static void SE_Confirm(u32 value)
{
    PlaySE(value);
}

static void DebugAction_PlaySE(void)
{
    DebugInput_Start((const u8[])_("Sound effect"), SE_SELECT, 1, SE_SUDOWOODO_SHAKE, NULL, SE_Confirm, TRUE);
}

static void DebugAction_StopMusic(void)
{
    Overworld_PlaySpecialMapMusic();
    DebugMessage((const u8[])_("Map music back."));
}

// ---------------------------------------------------------------------------
// Scratch scripts (data/scripts/debug.pory)
// ---------------------------------------------------------------------------

static EWRAM_DATA const u8 *sScriptToRun = NULL;

static void RunScratchScript(void)
{
    ScriptContext_SetupScript(sScriptToRun);
}

static void DebugAction_Script1(void)
{
    sScriptToRun = Debug_Script_1;
    sDebug->afterClose = RunScratchScript;
}

static void DebugAction_Script2(void)
{
    sScriptToRun = Debug_Script_2;
    sDebug->afterClose = RunScratchScript;
}

static void DebugAction_Script3(void)
{
    sScriptToRun = Debug_Script_3;
    sDebug->afterClose = RunScratchScript;
}

static void DebugAction_Script4(void)
{
    sScriptToRun = Debug_Script_4;
    sDebug->afterClose = RunScratchScript;
}

// ---------------------------------------------------------------------------
// Field effects (day/night, scanline effects, ghosts and lights)
// ---------------------------------------------------------------------------

static void DebugAction_TimeOfDay(void)
{
    u16 value = (VarGet(VAR_TIME_OF_DAY_OVERRIDE) + 1) % (TIME_OF_DAY_COUNT + 1);

    // Outdoors, day_night.c picks the change up on the next frame without a fade
    VarSet(VAR_TIME_OF_DAY_OVERRIDE, value);
    switch (value)
    {
    case 0:
        DebugMessage((const u8[])_("Time: clock"));
        break;
    case TIME_OF_DAY_MORN + 1:
        DebugMessage((const u8[])_("Time: morning"));
        break;
    case TIME_OF_DAY_DAY + 1:
        DebugMessage((const u8[])_("Time: day"));
        break;
    default:
        DebugMessage((const u8[])_("Time: night"));
        break;
    }
}

static void ToggleWave(u8 effect, u8 amplitude, u8 lineStep, u8 frameStep)
{
    struct FieldScanlineWave wave = {.top = 0, .bottom = DISPLAY_HEIGHT};

    if (FieldScanline_GetEffect() == effect)
    {
        FieldScanline_Stop();
    }
    else
    {
        wave.amplitude = amplitude;
        wave.lineStep = lineStep;
        wave.frameStep = frameStep;
        FieldScanline_Stop();
        if (!FieldScanline_StartWave(effect, &wave))
            DebugMessage((const u8[])_("DMA0 is in use\n(Flash?)."));
    }
    DebugMenu_Refresh();
}

static void DebugAction_ToggleHeatHaze(void)
{
    ToggleWave(FIELD_SCANLINE_HEAT_HAZE, 2, 24, 6);
}

static void DebugAction_ToggleWater(void)
{
    ToggleWave(FIELD_SCANLINE_WATER, 6, 6, 2);
}

static void DebugAction_ToggleGradient(void)
{
    // Masara's grass (the Kanto primary's palette 0), darkening to the bottom
    static const struct FieldScanlineGradient gradient =
    {
        .top = 16,
        .bottom = DISPLAY_HEIGHT,
        .palIndex = BG_PLTT_ID(0) + 13,
        .bottomColor = RGB(4, 10, 6),
    };

    if (FieldScanline_GetEffect() == FIELD_SCANLINE_GRADIENT)
    {
        FieldScanline_Stop();
    }
    else
    {
        FieldScanline_Stop();
        if (!FieldScanline_StartGradient(&gradient))
            DebugMessage((const u8[])_("DMA0 is in use\n(Flash?)."));
    }
    DebugMenu_Refresh();
}

static void DebugAction_ToggleGhosts(void)
{
    bool8 ghostly = !GetToggleState(TOGGLE_GHOSTS);
    u32 i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
    {
        if (gObjectEvents[i].active && !gObjectEvents[i].isPlayer)
            FieldBlend_SetObjectEventGhostly(i, ghostly);
    }
    FieldBlend_SetGhostAlpha(8, 8);
    FieldBlend_EnableGhosts(ghostly);
    DebugMenu_Refresh();
}

static void DebugAction_ToggleLantern(void)
{
    if (FieldLight_IsActive())
    {
        FieldLight_End();
    }
    else if (FieldLight_Begin(11))
    {
        struct ObjectEvent *player = &gObjectEvents[gPlayerAvatar.objectEventId];

        FieldLight_AddOnPlayer(FIELD_LIGHT_CIRCLE);
        // A lamp's cone, three tiles to the left
        FieldLight_AddAtMapCoords(FIELD_LIGHT_CONE, player->currentCoords.x - 3, player->currentCoords.y);
    }
    else
    {
        DebugMessage((const u8[])_("Not here (Flash)."));
    }
    DebugMenu_Refresh();
}

#endif // DEBUG_MENUS
