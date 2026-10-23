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
#include "money.h"
#include "overworld.h"
#include "palette.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "script_pokemon_util.h"
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

EWRAM_DATA struct RegressState gRegress = {0};
static EWRAM_DATA u8 sScript[16] = {0};

extern const u8 SPECIAL_ShowPokemonStorageSystemPC[];

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

void Regress_NoteDmaQueue(u32 pending)
{
    if (pending > gRegress.dmaPeak)
        gRegress.dmaPeak = pending;
}

void Regress_NoteDmaFull(void)
{
    gRegress.dmaFull++;
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
            *d++ = GetMonData(mon, MON_DATA_PP1 + j);
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
    }
    gRegress.cmdDone++;
}

#endif // REGRESS_HOOKS
