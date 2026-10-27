#include "global.h"
#include "battle.h"
#include "battle_gfx_sfx_util.h"
#include "berry.h"
#include "data.h"
#include "daycare.h"
#include "decompress.h"
#include "event_data.h"
#include "international_string_util.h"
#include "link.h"
#include "main.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "party_menu.h"
#include "pokedex.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "script.h"
#include "sprite.h"
#include "string_util.h"
#include "tv.h"
#include "constants/items.h"
#include "constants/battle_setup.h"
#include "constants/battle_frontier.h"

static void CB2_ReturnFromChooseHalfParty(void);
static void CB2_ReturnFromChooseBattleFrontierParty(void);

void HealPlayerParty(void)
{
    u8 i;
    u8 arg[4];

    // restore HP.
    for(i = 0; i < gPlayerPartyCount; i++)
    {
        u16 maxHP = GetMonData(&gPlayerParty[i], MON_DATA_MAX_HP);
        arg[0] = maxHP;
        arg[1] = maxHP >> 8;
        SetMonData(&gPlayerParty[i], MON_DATA_HP, arg);

        // since status is u32, the four 0 assignments here are probably for safety to prevent undefined data from reaching SetMonData.
        arg[0] = 0;
        arg[1] = 0;
        arg[2] = 0;
        arg[3] = 0;
        SetMonData(&gPlayerParty[i], MON_DATA_STATUS, arg);
    }
}

// A night's rest: HealPlayerParty plus full TP. TP only refills by resting
// (inns, home, whiteout), not at a nurse.
void RestPlayerParty(void)
{
    u8 i;
    u8 tp = TP_MAX;

    HealPlayerParty();
    for (i = 0; i < gPlayerPartyCount; i++)
        SetMonData(&gPlayerParty[i], MON_DATA_SAVED_TP, &tp);
}

u8 ScriptGiveMon(u16 species, u8 level, u16 item, u32 unused1, u32 unused2, u8 unused3)
{
    u16 nationalDexNum;
    int sentToPc;
    u8 heldItem[2];
    struct Pokemon mon;

    CreateMon(&mon, species, level, USE_RANDOM_IVS, FALSE, 0, OT_ID_PLAYER_ID, 0);
    heldItem[0] = item;
    heldItem[1] = item >> 8;
    SetMonData(&mon, MON_DATA_HELD_ITEM, heldItem);
    sentToPc = GiveMonToPlayer(&mon);
    nationalDexNum = SpeciesToNationalPokedexNum(species);

    // Don't set Pokédex flag for MON_CANT_GIVE
    switch(sentToPc)
    {
    case MON_GIVEN_TO_PARTY:
    case MON_GIVEN_TO_PC:
        GetSetPokedexFlag(nationalDexNum, FLAG_SET_SEEN);
        GetSetPokedexFlag(nationalDexNum, FLAG_SET_CAUGHT);
        break;
    }
    return sentToPc;
}

u8 ScriptGiveEgg(u16 species)
{
    struct Pokemon mon;
    u8 isEgg;

    CreateEgg(&mon, species, TRUE);
    isEgg = TRUE;
    SetMonData(&mon, MON_DATA_IS_EGG, &isEgg);

    return GiveMonToPlayer(&mon);
}

void HasEnoughMonsForDoubleBattle(void)
{
    switch (GetMonsStateToDoubles())
    {
    case PLAYER_HAS_TWO_USABLE_MONS:
        gSpecialVar_Result = PLAYER_HAS_TWO_USABLE_MONS;
        break;
    case PLAYER_HAS_ONE_MON:
        gSpecialVar_Result = PLAYER_HAS_ONE_MON;
        break;
    case PLAYER_HAS_ONE_USABLE_MON:
        gSpecialVar_Result = PLAYER_HAS_ONE_USABLE_MON;
        break;
    }
}

void CreateScriptedWildMon(u16 species, u8 level, u16 item)
{
    u8 heldItem[2];

    ZeroEnemyPartyMons();
    CreateMon(&gEnemyParty[0], species, level, USE_RANDOM_IVS, 0, 0, OT_ID_PLAYER_ID, 0);
    if (item)
    {
        heldItem[0] = item;
        heldItem[1] = item >> 8;
        SetMonData(&gEnemyParty[0], MON_DATA_HELD_ITEM, heldItem);
    }
}

void ScriptSetMonMoveSlot(u8 monIndex, u16 move, u8 slot)
{
// Allows monIndex to go out of bounds of gPlayerParty. Doesn't occur in vanilla
#ifdef BUGFIX
    if (monIndex >= PARTY_SIZE)
#else
    if (monIndex > PARTY_SIZE)
#endif
        monIndex = gPlayerPartyCount - 1;

    SetMonMoveSlot(&gPlayerParty[monIndex], move, slot);
}

// Note: When control returns to the event script, gSpecialVar_Result will be
// TRUE if the party selection was successful.
void ChooseHalfPartyForBattle(void)
{
    gMain.savedCallback = CB2_ReturnFromChooseHalfParty;
    VarSet(VAR_FRONTIER_FACILITY, FACILITY_MULTI_OR_EREADER);
    InitChooseHalfPartyForBattle(0);
}

static void CB2_ReturnFromChooseHalfParty(void)
{
    switch (gSelectedOrderFromParty[0])
    {
    case 0:
        gSpecialVar_Result = FALSE;
        break;
    default:
        gSpecialVar_Result = TRUE;
        break;
    }

    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void ChoosePartyForBattleFrontier(void)
{
    gMain.savedCallback = CB2_ReturnFromChooseBattleFrontierParty;
    InitChooseHalfPartyForBattle(gSpecialVar_0x8004 + 1);
}

static void CB2_ReturnFromChooseBattleFrontierParty(void)
{
    switch (gSelectedOrderFromParty[0])
    {
    case 0:
        gSpecialVar_Result = FALSE;
        break;
    default:
        gSpecialVar_Result = TRUE;
        break;
    }

    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

void ReducePlayerPartyToSelectedMons(void)
{
    struct Pokemon party[MAX_FRONTIER_PARTY_SIZE];
    int i;

    CpuFill32(0, party, sizeof party);

    // copy the selected Pokémon according to the order.
    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
        if (gSelectedOrderFromParty[i]) // as long as the order keeps going (did the player select 1 mon? 2? 3?), do not stop
            party[i] = gPlayerParty[gSelectedOrderFromParty[i] - 1]; // index is 0 based, not literal

    CpuFill32(0, gPlayerParty, sizeof gPlayerParty);

    // overwrite the first 4 with the order copied to.
    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
        gPlayerParty[i] = party[i];

    CalculatePlayerPartyCount();
}

// Party hand-overs (studio slice, core). Quest scripts that take a Pokémon from the
// party first ask CheckPartyMonForHandover, then call RemovePartyMonAtSlot.
//
// special CheckPartyMonForHandover
//   in:  VAR_0x8004 species, VAR_0x8005 minimum level,
//        VAR_0x8006 required met-location mapsec (HANDOVER_ANY_LOCATION = any),
//        VAR_0x8007 TRUE = it must be at full HP
//   out: VAR_RESULT HANDOVER_OK / HANDOVER_NONE / HANDOVER_NOT_FULL_HP / HANDOVER_LAST_MON,
//        VAR_0x8008 = the party slot of the mon that matched (the first one, preferring a
//        mon that is at full HP when full HP is required); unchanged on HANDOVER_NONE.
// "Last party member" is a party of one (eggs count as members).
void CheckPartyMonForHandover(void)
{
    u32 i;
    u32 match = PARTY_SIZE;
    bool32 matchHurt = FALSE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];
        bool32 hurt;

        if (GetMonData(mon, MON_DATA_SPECIES) != gSpecialVar_0x8004 || GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        if (GetMonData(mon, MON_DATA_LEVEL) < gSpecialVar_0x8005)
            continue;
        if (gSpecialVar_0x8006 != HANDOVER_ANY_LOCATION && GetMonData(mon, MON_DATA_MET_LOCATION) != gSpecialVar_0x8006)
            continue;
        hurt = GetMonData(mon, MON_DATA_HP) != GetMonData(mon, MON_DATA_MAX_HP);
        if (match == PARTY_SIZE || (matchHurt && !hurt))
        {
            match = i;
            matchHurt = hurt;
        }
    }

    if (match == PARTY_SIZE)
    {
        gSpecialVar_Result = HANDOVER_NONE;
        return;
    }
    gSpecialVar_0x8008 = match;
    if (gSpecialVar_0x8007 && matchHurt)
        gSpecialVar_Result = HANDOVER_NOT_FULL_HP;
    else if (CalculatePlayerPartyCount() <= 1)
        gSpecialVar_Result = HANDOVER_LAST_MON;
    else
        gSpecialVar_Result = HANDOVER_OK;
}

// special RemovePartyMonAtSlot: takes the party member in slot VAR_0x8008 out of the
// party and closes the gap. Never removes the last member (VAR_RESULT FALSE then).
void RemovePartyMonAtSlot(void)
{
    if (gSpecialVar_0x8008 >= PARTY_SIZE || CalculatePlayerPartyCount() <= 1
     || GetMonData(&gPlayerParty[gSpecialVar_0x8008], MON_DATA_SPECIES) == SPECIES_NONE)
    {
        gSpecialVar_Result = FALSE;
        return;
    }
    ZeroMonData(&gPlayerParty[gSpecialVar_0x8008]);
    CompactPartySlots();
    CalculatePlayerPartyCount();
    gSpecialVar_Result = TRUE;
}

// special SetPartyMonFriendship: party slot VAR_0x8004 gets friendship VAR_0x8005 (0-255).
// Use the setmonfriendship macro.
void SetPartyMonFriendship(void)
{
    u8 friendship = min(gSpecialVar_0x8005, MAX_FRIENDSHIP);

    if (gSpecialVar_0x8004 < PARTY_SIZE)
        SetMonData(&gPlayerParty[gSpecialVar_0x8004], MON_DATA_FRIENDSHIP, &friendship);
}
