#include "global.h"
#include "regress.h"

// Test hooks for tools/regress. Only built with `make REGRESS=1`.
// The harness writes a command into gRegress.cmd; the overworld runs it once
// the player is free to move. Everything here uses fixed personalities and
// IVs so the run gives the same data no matter how the RNG moved.

#ifdef REGRESS_HOOKS

#include "event_data.h"
#include "field_screen_effect.h"
#include "gba/io_reg.h"
#include "item.h"
#include "main.h"
#include "map_name_popup.h"
#include "money.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "sprite.h"
#include "region_map.h"
#include "script.h"
#include "sound.h"
#include "script_pokemon_util.h"
#include "event_scripts.h"
#include "string_util.h"
#include "constants/global.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/maps.h"
#include "constants/opponents.h"
#include "constants/species.h"
#include "constants/vars.h"
#include "constants/flags.h"
#include "constants/battle_setup.h"
#include "constants/event_objects.h"
#include "constants/trade.h"
#include "daycare.h"
#include "evolution_scene.h"
#include "hall_of_fame.h"
#include "naming_screen.h"
#include "decompress.h"
#include "pokenav.h"
#include "save.h"
#include "gba/flash_internal.h"

EWRAM_DATA struct RegressState gRegress = {0};
static EWRAM_DATA u8 sScript[32] = {0};

extern const u8 SPECIAL_ShowPokemonStorageSystemPC[];
extern const u8 SPECIAL_EggHatch[];
extern const u8 SPECIAL_BedroomPC[];
extern const u8 SPECIAL_FieldShowRegionMap[];
extern const u8 SPECIAL_CreateInGameTradePokemon[];
extern const u8 SPECIAL_DoInGameTradeScene[];

#define SCROP_END           0x02
#define SCROP_SPECIAL       0x25
#define SCROP_WAITSTATE     0x27
#define SCROP_TRAINERBATTLE 0x5c
#define SCROP_SETWILDBATTLE 0xb6
#define SCROP_DOWILDBATTLE  0xb7

#define REGRESS_OT_ID 0x0000C0DE

static const u16 sPartySpecies[PARTY_SIZE] = {
    SPECIES_VENUSAUR, SPECIES_PIKACHU, SPECIES_GENGAR,
    SPECIES_SNORLAX, SPECIES_DRAGONITE, SPECIES_MEW,
};
static const u8 sPartyLevels[PARTY_SIZE] = {50, 45, 48, 50, 55, 60};
// Slot 1 gets a damaging move so mashing A wins battles
static const u16 sPartyFirstMove[PARTY_SIZE] = {
    MOVE_RAZOR_LEAF, MOVE_THUNDERBOLT, MOVE_PSYCHIC,
    MOVE_BODY_SLAM, MOVE_SLAM, MOVE_PSYCHIC,
};

static const u8 sTrainerDefeatText[] = _("Regress battle over.");

void Regress_NoteAlloc(void *heapStart, void *ptr, u32 size)
{
    u32 end;

    if (ptr == NULL)
    {
        gRegress.heapFails++;
        return;
    }
    end = (u8 *)ptr + size - (u8 *)heapStart;
    if (end > gRegress.heapPeak)
        gRegress.heapPeak = end;
}

// Called by HeapFatal (src/malloc.c) just before the game stops
void Regress_NoteHeapFatal(u32 kind, u32 value, u32 caller)
{
    gRegress.heapFatal = kind;
    gRegress.heapFatalValue = value;
    gRegress.heapFatalCaller = caller;
}

// Called by the save code before each flash write. Counts them, and cuts the
// "power" at the one gRegress.saveCutAt names: optionally half of its bytes
// get programmed (into the freshly erased sector), then the CPU stops for
// good, as if the GBA was switched off. The harness then resets the emulator,
// which keeps the flash as it is.
void Regress_SaveFlashOp(u16 sector, const u8 *data, u32 size, bool32 wholeSector)
{
    u32 i;

    gRegress.saveFlashOps++;
    if (wholeSector)
        gRegress.saveSectorWrites++;
    if (gRegress.saveCutAt == 0 || gRegress.saveFlashOps != gRegress.saveCutAt)
        return;

    if (gRegress.saveCutHalf && data != NULL)
    {
        EraseFlashSector(sector);
        for (i = 0; i < size / 2; i++)
            ProgramFlashByte(sector, i, data[i]);
    }
    gRegress.saveCutDone = TRUE;
    REG_IME = 0;
    for (;;)
        ;
}

void Regress_NoteDmaQueue(u32 pending)
{
    if (pending > gRegress.dmaPeak)
        gRegress.dmaPeak = pending;
}

void Regress_NoteDmaFull(void)
{
    gRegress.dmaFull++;
}

#define STACK_FILL 0xA5A5A5A5
#define STACK_SCAN_INTERVAL 16 // frames

extern u32 gIwramEnd[]; // end of the IWRAM data, from the linker script
static u32 *sStackFillEnd;

// Paints the free IWRAM below the current stack pointer so Regress_FrameEnd can
// see how deep the stack has been. Call it first thing in AgbMain.
void Regress_StackInit(void)
{
    u32 *p;

    sStackFillEnd = (u32 *)((u32)__builtin_frame_address(0) - 0x80);
    for (p = gIwramEnd; p < sStackFillEnd; p++)
        *p = STACK_FILL;
}

static void NoteStack(void)
{
    u32 *p = gIwramEnd;

    if (sStackFillEnd == NULL)
        return;
    while (p < sStackFillEnd && *p == STACK_FILL)
        p++;
    gRegress.stackLowest = (u32)p;
}

void Regress_NoteFail(u32 kind)
{
    switch (kind)
    {
    case REGRESS_FAIL_SPRITE:
        gRegress.spriteFails++;
        break;
    case REGRESS_FAIL_PALETTE:
        gRegress.paletteFails++;
        break;
    case REGRESS_FAIL_TILES:
        gRegress.tileFails++;
        break;
    case REGRESS_FAIL_WINDOW:
        gRegress.windowFails++;
        break;
    }
}

void Regress_NoteTiles(u32 start, u32 count)
{
    if (start + count > gRegress.tileHigh)
        gRegress.tileHigh = start + count;
}

void Regress_SpriteStatics(const u8 *tileBitmap, const u16 *palTags)
{
    gRegress.tileBitmap = tileBitmap;
    gRegress.palTags = palTags;
}

// Sampled between frames, only when the frame left idle time, so that it does not
// change the lag numbers or the timing of later frames
static void SampleResources(u32 busy)
{
    u32 i, run, best, count;

    count = 0;
    for (i = 0; i < MAX_SPRITES; i++)
        count += gSprites[i].inUse;
    if (count > gRegress.spritesMax)
        gRegress.spritesMax = count;

    count = 0;
    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        count += gObjectEvents[i].active;
    if (count > gRegress.objEventsMax)
        gRegress.objEventsMax = count;

    if (gRegress.tileBitmap != NULL && busy < 120 && (gRegress.frames & 3) == 0)
    {
        const u8 *bitmap = gRegress.tileBitmap;
        u32 bit;

        run = best = 0;
        for (i = gReservedSpriteTileCount; i < TOTAL_OBJ_TILE_COUNT; i++)
        {
            if ((i & 7) == 0 && i + 8 <= TOTAL_OBJ_TILE_COUNT)
            {
                // whole bytes: skip fully used ones, add fully free ones
                if (bitmap[i / 8] == 0xFF)
                {
                    run = 0;
                    i += 7;
                    continue;
                }
                if (bitmap[i / 8] == 0)
                {
                    run += 8;
                    if (run > best)
                        best = run;
                    i += 7;
                    continue;
                }
            }
            bit = (bitmap[i / 8] >> (i % 8)) & 1;
            if (bit)
                run = 0;
            else if (++run > best)
                best = run;
        }
        if (gRegress.tileFreeRunMin == 0 || best + 1 < gRegress.tileFreeRunMin)
            gRegress.tileFreeRunMin = best + 1;
    }
    if (gRegress.palTags != NULL)
    {
        count = 0;
        for (i = gReservedSpritePaletteCount; i < 16; i++)
            count += gRegress.palTags[i] == TAG_NONE;
        if (gRegress.palFreeMin == 0 || count + 1 < gRegress.palFreeMin)
            gRegress.palFreeMin = count + 1;
    }
}

void Regress_FrameEnd(void)
{
    u32 busy = (REG_VCOUNT + 228 - 160) % 228;
    u32 vbl = gMain.vblankCounter1;

    gRegress.magic = REGRESS_MAGIC;
    gRegress.frames++;
    gRegress.busyLinesSum += busy;
    if (busy > gRegress.busyLinesMax)
        gRegress.busyLinesMax = busy;
    if (gRegress.lastVblank != 0 && vbl - gRegress.lastVblank > 1)
        gRegress.lagFrames += vbl - gRegress.lastVblank - 1;
    gRegress.lastVblank = vbl;
    if (gRegress.frames % STACK_SCAN_INTERVAL == 0)
        NoteStack();
    if (busy < 170)
        SampleResources(busy);
}

static void Setup(void)
{
    struct BoxPokemon boxMon;
    u32 i;

    // The trainer ID and RNG seed come from how many cycles the intro took,
    // which any code change shifts. Pin both so runs of different builds match.
    gSaveBlock2Ptr->playerTrainerId[0] = 0x39;
    gSaveBlock2Ptr->playerTrainerId[1] = 0x30;
    gSaveBlock2Ptr->playerTrainerId[2] = 0x05;
    gSaveBlock2Ptr->playerTrainerId[3] = 0x00;
    SeedRng(0x5EED);
    SeedRng2(0x5EED);

    ZeroPlayerPartyMons();
    for (i = 0; i < PARTY_SIZE; i++)
    {
        CreateMon(&gPlayerParty[i], sPartySpecies[i], sPartyLevels[i], 20, TRUE, 0x1000 + i * 0x111, OT_ID_PLAYER_ID, 0);
        SetMonMoveSlot(&gPlayerParty[i], sPartyFirstMove[i], 0);
    }
    CalculatePlayerPartyCount();

    // 40 mons in boxes 1-2, plus a few in box 12 (the last box since the PC
    // went from 14 to 12 boxes; a fixed box keeps runs comparable across that)
    for (i = 0; i < 40; i++)
    {
        u16 species = SPECIES_BULBASAUR + (i * 7) % SPECIES_GEN1_MAX;
        CreateBoxMon(&boxMon, species, 5 + i, 10, TRUE, 0x2000 + i * 0x31, OT_ID_PLAYER_ID, 0);
        SetBoxMonAt(i / IN_BOX_COUNT, i % IN_BOX_COUNT, &boxMon);
    }
    for (i = 0; i < 3; i++)
    {
        CreateBoxMon(&boxMon, SPECIES_EEVEE + i, 20 + i, 10, TRUE, 0x3000 + i, OT_ID_PLAYER_ID, 0);
        SetBoxMonAt(11, i, &boxMon);
    }

    for (i = SPECIES_BULBASAUR; i <= SPECIES_GEN1_MAX; i += 2)
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(i), FLAG_SET_SEEN);
    for (i = 0; i < PARTY_SIZE; i++)
    {
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(sPartySpecies[i]), FLAG_SET_SEEN);
        GetSetPokedexFlag(SpeciesToNationalPokedexNum(sPartySpecies[i]), FLAG_SET_CAUGHT);
    }

    AddBagItem(ITEM_POTION, 10);
    AddBagItem(ITEM_SUPER_POTION, 3);
    AddBagItem(ITEM_RARE_CANDY, 5);
    AddBagItem(ITEM_MASTER_BALL, 1); // first in the pocket, so the catch scene never fails
    AddBagItem(ITEM_POKE_BALL, 20);
    AddBagItem(ITEM_ULTRA_BALL, 2);
    AddBagItem(ITEM_TM01, 1);
    AddBagItem(ITEM_HM01, 1);
    SetMoney(&gSaveBlock1Ptr->money, 123456);

    gSaveBlock2Ptr->optionsBattleStyle = OPTIONS_BATTLE_STYLE_SET; // no "switch?" prompts
    FlagSet(FLAG_SYS_POKEMON_GET);
    FlagSet(FLAG_SYS_POKEDEX_GET);
    FlagSet(FLAG_ADVENTURE_STARTED);
    VarSet(VAR_OLDALE_TOWN_STATE, 1);
    VarSet(VAR_OLDALE_RIVAL_STATE, 2);

    SetWarpDestination(MAP_GROUP(MAP_OLDALE_TOWN), MAP_NUM(MAP_OLDALE_TOWN), WARP_ID_NONE, 10, 12);
    DoWarp();
}

// Called when a battle starts or ends (MoveSaveBlocks_ResetHeap) and before
// each battle command. Loading speed changes how many frames pass before a
// random roll, so without this a faster build would roll different damage.
void Regress_Reseed(void)
{
    static u16 sCount;
    sCount++;
    SeedRng(0x5EED + sCount);
    SeedRng2(0x5EED + sCount);
}

static void StartWild(void)
{
    u8 *s = sScript;
    *s++ = SCROP_SETWILDBATTLE;
    *s++ = SPECIES_PIDGEY & 0xFF;
    *s++ = SPECIES_PIDGEY >> 8;
    *s++ = 5;
    *s++ = ITEM_NONE & 0xFF;
    *s++ = ITEM_NONE >> 8;
    *s++ = SCROP_DOWILDBATTLE;
    *s++ = SCROP_END;
    ScriptContext_SetupScript(sScript);
}

static void StartTrainer(void)
{
    u8 *s = sScript;
    u32 text = (u32)sTrainerDefeatText;
    *s++ = SCROP_TRAINERBATTLE;
    *s++ = TRAINER_BATTLE_SINGLE_NO_INTRO_TEXT;
    *s++ = TRAINER_ROXANNE_1 & 0xFF;
    *s++ = TRAINER_ROXANNE_1 >> 8;
    *s++ = LOCALID_NONE & 0xFF;
    *s++ = LOCALID_NONE >> 8;
    *s++ = text;
    *s++ = text >> 8;
    *s++ = text >> 16;
    *s++ = text >> 24;
    *s++ = SCROP_END;
    ScriptContext_SetupScript(sScript);
}

static void OpenPC(void)
{
    u16 special = (u32)SPECIAL_ShowPokemonStorageSystemPC;
    u8 *s = sScript;
    *s++ = SCROP_SPECIAL;
    *s++ = special & 0xFF;
    *s++ = special >> 8;
    *s++ = SCROP_WAITSTATE;
    *s++ = SCROP_END;
    ScriptContext_SetupScript(sScript);
}

static u8 *AddSpecial(u8 *s, const u8 *specialId)
{
    u16 special = (u32)specialId;
    *s++ = SCROP_SPECIAL;
    *s++ = special & 0xFF;
    *s++ = special >> 8;
    return s;
}

static void StartScene(u8 scene)
{
    u8 *s = sScript;
    u32 text = (u32)sTrainerDefeatText;

    switch (scene)
    {
    case REGRESS_SCENE_EVOLUTION:
        gCB2_AfterEvolution = CB2_ReturnToField;
        BeginEvolutionScene(&gPlayerParty[1], SPECIES_RAICHU, TRUE, 1);
        break;
    case REGRESS_SCENE_NAMING:
        DoNamingScreen(NAMING_SCREEN_NICKNAME, gStringVar3, SPECIES_PIKACHU, MON_MALE, 0x1111, CB2_ReturnToField);
        break;
    case REGRESS_SCENE_EGG_HATCH:
        CreateEgg(&gPlayerParty[5], SPECIES_PIKACHU, FALSE);
        gSpecialVar_0x8004 = 5;
        s = AddSpecial(s, SPECIAL_EggHatch);
        *s++ = SCROP_WAITSTATE;
        *s++ = SCROP_END;
        ScriptContext_SetupScript(sScript);
        break;
    case REGRESS_SCENE_INGAME_TRADE:
        gSpecialVar_0x8004 = 0;
        gSpecialVar_0x8005 = 1;
        s = AddSpecial(s, SPECIAL_CreateInGameTradePokemon);
        s = AddSpecial(s, SPECIAL_DoInGameTradeScene);
        *s++ = SCROP_WAITSTATE;
        *s++ = SCROP_END;
        ScriptContext_SetupScript(sScript);
        break;
    case REGRESS_SCENE_HALL_OF_FAME:
        SetMainCallback2(CB2_DoHallOfFameScreenDontSaveData);
        break;
    case REGRESS_SCENE_DOUBLE_BATTLE:
        *s++ = SCROP_TRAINERBATTLE;
        *s++ = TRAINER_BATTLE_DOUBLE;
        *s++ = TRAINER_GABBY_AND_TY_1 & 0xFF;
        *s++ = TRAINER_GABBY_AND_TY_1 >> 8;
        *s++ = LOCALID_NONE & 0xFF;
        *s++ = LOCALID_NONE >> 8;
        for (scene = 0; scene < 3; scene++) // intro, defeat, not enough mons
        {
            *s++ = text;
            *s++ = text >> 8;
            *s++ = text >> 16;
            *s++ = text >> 24;
        }
        *s++ = SCROP_END;
        ScriptContext_SetupScript(sScript);
        break;
    case REGRESS_SCENE_SUBSTITUTE:
        SetMonMoveSlot(&gPlayerParty[0], MOVE_SUBSTITUTE, 0);
        StartWild();
        break;
    case REGRESS_SCENE_BEDROOM_PC:
        s = AddSpecial(s, SPECIAL_BedroomPC);
        *s++ = SCROP_WAITSTATE;
        *s++ = SCROP_END;
        ScriptContext_SetupScript(sScript);
        break;
    case REGRESS_SCENE_POKENAV:
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_InitPokeNav);
        break;
    case REGRESS_SCENE_SAVE_FAILED:
        // A damaged sector makes the screen wipe it and save again.
        gDamagedSaveSectors = 1 << SECTOR_ID_HOF_2;
        gGameContinueCallback = CB2_ReturnToField;
        DoSaveFailedScreen(SAVE_NORMAL);
        break;
    case REGRESS_SCENE_SAVE_FAILED_HOF:
    {
        u32 i;
        // The save-failed screen saves the record again from this buffer,
        // so the pattern must reach the Hall of Fame sectors intact.
        for (i = 0; i < 0x2000; i++) // two save sectors
            gDecompressionBuffer[i] = i * 7 + 3;
        gDamagedSaveSectors = 1 << SECTOR_ID_HOF_2;
        gGameContinueCallback = CB2_ReturnToField;
        DoSaveFailedScreen(SAVE_HALL_OF_FAME);
        break;
    }
    case REGRESS_SCENE_LOAD_HOF:
        CpuFill32(0, gDecompressionBuffer, sizeof(gDecompressionBuffer));
        gSpecialVar_Result = LoadGameSave(SAVE_HALL_OF_FAME);
        break;
    // The partial and spread-out saves link screens use, run in one go
    case REGRESS_SCENE_LINK_FULL_SAVE:
        LinkFullSave_Init();
        while (!LinkFullSave_WriteSector())
            ;
        LinkFullSave_ReplaceLastSector();
        LinkFullSave_SetLastSectorSignature();
        break;
    case REGRESS_SCENE_SAVE_BLOCKS:
        WriteSaveBlock2();
        while (!WriteSaveBlock1Sector())
            ;
        break;
    case REGRESS_SCENE_SAVE_LINK:
        TrySavingData(SAVE_LINK);
        break;
    case REGRESS_SCENE_TOWN_MAP:
        s = AddSpecial(s, SPECIAL_FieldShowRegionMap);
        *s++ = SCROP_WAITSTATE;
        *s++ = SCROP_END;
        ScriptContext_SetupScript(sScript);
        break;
    case REGRESS_SCENE_FLY_MAP:
        CleanupOverworldWindowsAndTilemaps();
        SetMainCallback2(CB2_OpenFlyMap);
        break;
    }
}

// The dump is a list of u16 words that only depends on what the player owns,
// not on how the save blocks are laid out, so it can be compared across builds.
static u16 *DumpU32(u16 *d, u32 v)
{
    *d++ = v;
    *d++ = v >> 16;
    return d;
}

static void Dump(void)
{
    u16 *d = gRegress.dump;
    u16 *countPtr;
    u32 i, j, box, pos;

    *d++ = 0xD00D;

    // Party
    *d++ = gPlayerPartyCount;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        *d++ = GetMonData(mon, MON_DATA_SPECIES);
        *d++ = GetMonData(mon, MON_DATA_LEVEL);
        *d++ = GetMonData(mon, MON_DATA_HP);
        *d++ = GetMonData(mon, MON_DATA_MAX_HP);
        *d++ = GetMonData(mon, MON_DATA_ATK);
        *d++ = GetMonData(mon, MON_DATA_DEF);
        *d++ = GetMonData(mon, MON_DATA_SPEED);
        *d++ = GetMonData(mon, MON_DATA_SPATK);
        *d++ = GetMonData(mon, MON_DATA_SPDEF);
        for (j = 0; j < MAX_MON_MOVES; j++)
            *d++ = GetMonData(mon, MON_DATA_MOVE1 + j);
        for (j = 0; j < MAX_MON_MOVES; j++)
            *d++ = 0; // was PP (removed); kept so the dump layout stays the same
        d = DumpU32(d, GetMonData(mon, MON_DATA_EXP));
        d = DumpU32(d, GetMonData(mon, MON_DATA_PERSONALITY));
        d = DumpU32(d, GetMonData(mon, MON_DATA_OT_ID));
        *d++ = GetMonData(mon, MON_DATA_HELD_ITEM);
        *d++ = GetMonData(mon, MON_DATA_FRIENDSHIP);
        *d++ = GetMonData(mon, MON_DATA_STATUS);
        *d++ = GetMonData(mon, MON_DATA_HP_IV) | (GetMonData(mon, MON_DATA_SPEED_IV) << 8);
    }

    // PC: count, then the first 64 mons
    countPtr = d++;
    *countPtr = 0;
    for (box = 0; box < TOTAL_BOXES_COUNT; box++)
    {
        for (pos = 0; pos < IN_BOX_COUNT; pos++)
        {
            struct BoxPokemon *boxMon = GetBoxedMonPtr(box, pos);
            if (GetBoxMonData(boxMon, MON_DATA_SPECIES) == SPECIES_NONE)
                continue;
            if (++*countPtr > 64)
                continue;
            *d++ = (box << 8) | pos;
            *d++ = GetBoxMonData(boxMon, MON_DATA_SPECIES);
            *d++ = GetLevelFromBoxMonExp(boxMon);
            d = DumpU32(d, GetBoxMonData(boxMon, MON_DATA_PERSONALITY));
            for (j = 0; j < MAX_MON_MOVES; j++)
                *d++ = GetBoxMonData(boxMon, MON_DATA_MOVE1 + j);
        }
    }

    // Bag
    for (i = 0; i < POCKETS_COUNT; i++)
    {
        countPtr = d++;
        *countPtr = 0;
        for (j = 0; j < gBagPockets[i].capacity; j++)
        {
            struct ItemSlot *slot = &gBagPockets[i].itemSlots[j];
            if (slot->itemId == ITEM_NONE)
                continue;
            (*countPtr)++;
            *d++ = slot->itemId;
            *d++ = slot->quantity ^ (u16)gSaveBlock2Ptr->encryptionKey;
        }
    }

    // Player
    d = DumpU32(d, GetMoney(&gSaveBlock1Ptr->money));
    for (i = 0; i < PLAYER_NAME_LENGTH + 1; i += 2)
        *d++ = gSaveBlock2Ptr->playerName[i] | (gSaveBlock2Ptr->playerName[i + 1] << 8);
    *d++ = gSaveBlock2Ptr->playerGender;
    *d++ = gSaveBlock2Ptr->playerTrainerId[0] | (gSaveBlock2Ptr->playerTrainerId[1] << 8);
    *d++ = gSaveBlock2Ptr->playerTrainerId[2] | (gSaveBlock2Ptr->playerTrainerId[3] << 8);
    *d++ = (gSaveBlock1Ptr->location.mapGroup << 8) | (u8)gSaveBlock1Ptr->location.mapNum;
    *d++ = gSaveBlock1Ptr->pos.x;
    *d++ = gSaveBlock1Ptr->pos.y;

    // Pokédex seen and caught, 1-151, 16 per word
    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 160; j += 16)
        {
            u32 k, bits = 0;
            for (k = 0; k < 16; k++)
            {
                u32 dexNum = j + k + 1;
                if (dexNum <= 151 && GetSetPokedexFlag(dexNum, i == 0 ? FLAG_GET_SEEN : FLAG_GET_CAUGHT))
                    bits |= 1 << k;
            }
            *d++ = bits;
        }
    }

    // A few story flags and vars the run sets
    *d++ = FlagGet(FLAG_SYS_POKEMON_GET) | (FlagGet(FLAG_SYS_POKEDEX_GET) << 1) | (FlagGet(FLAG_ADVENTURE_STARTED) << 2);
    *d++ = VarGet(VAR_OLDALE_TOWN_STATE);
    *d++ = VarGet(VAR_OLDALE_RIVAL_STATE);

    *d++ = 0xE0D0;
    gRegress.dumpWords = d - gRegress.dump;
}

void Regress_OverworldTick(void)
{
    u8 cmd;

    gRegress.fieldReady = !ArePlayerFieldControlsLocked() && Regress_ScriptIdle() && !gPaletteFade.active;
    cmd = gRegress.cmd;
    if (cmd == REGRESS_CMD_NONE)
        return;
    if (cmd != REGRESS_CMD_DUMP && !gRegress.fieldReady)
        return;

    gRegress.cmd = REGRESS_CMD_NONE;
    switch (cmd)
    {
    case REGRESS_CMD_SETUP:
        Setup();
        break;
    case REGRESS_CMD_WILD:
        Regress_Reseed();
        StartWild();
        break;
    case REGRESS_CMD_MUSIC:
        PlayNewMapMusic(gRegress.song);
        break;
    case REGRESS_CMD_TRAINER:
        Regress_Reseed();
        StartTrainer();
        break;
    case REGRESS_CMD_PC:
        OpenPC();
        break;
    case REGRESS_CMD_DUMP:
        Dump();
        break;
    case REGRESS_CMD_HEAL:
        HealPlayerParty();
        break;
    case REGRESS_CMD_TV:
        FlagSet(FLAG_SYS_TV_START);
        FlagClear(FLAG_SYS_TV_WATCH);
        HideMapNamePopUpWindow(); // what pressing A on a TV does first
        ScriptContext_SetupScript(EventScript_TV);
        break;
    case REGRESS_CMD_SCENE:
        Regress_Reseed();
        StartScene(gRegress.arg);
        break;
    case REGRESS_CMD_CABLE_CLUB:
        SetWarpDestination(MAP_GROUP(MAP_OLDALE_TOWN_POKEMON_CENTER_2F), MAP_NUM(MAP_OLDALE_TOWN_POKEMON_CENTER_2F), WARP_ID_NONE, 10, 3);
        DoWarp();
        break;
    }
    gRegress.cmdDone++;
}

#endif // REGRESS_HOOKS
