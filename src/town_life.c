#include "global.h"
#include "day_night.h"
#include "event_data.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "town_life.h"
#include "constants/flags.h"
#include "constants/town_life.h"
#include "constants/vars.h"

// Town-life toolkit: the C side of the script helpers in data/scripts/town_life.pory
// and data/scripts/inn.pory. Everything lives in existing flags and vars: no RAM or
// save data of its own.

// Charisma at or above each entry reaches CHARISMA_TIER_LOW, MID and HIGH.
static const u8 sCharismaTiers[NUM_CHARISMA_TIER_TABLES][3] = {
    [CHARISMA_TIERS_TOKIWA] = {2, 5, 9},
};

// special GetCharismaTier: VAR_RESULT = CHARISMA_TIER_* for the table in VAR_0x8004.
// Use the checkcharisma macro.
void GetCharismaTier(void)
{
    const u8 *tiers = sCharismaTiers[gSpecialVar_0x8004 < NUM_CHARISMA_TIER_TABLES ? gSpecialVar_0x8004 : 0];
    u16 charisma = VarGet(VAR_CHARISMA);
    u16 tier = CHARISMA_TIER_NONE;

    while (tier < ARRAY_COUNT(sCharismaTiers[0]) && charisma >= tiers[tier])
        tier++;
    gSpecialVar_Result = tier;
}

// TRUE when hour is in [start, end); a window can wrap past midnight (22, 2).
static bool32 IsHourInWindow(u8 hour, u8 start, u8 end)
{
    if (start <= end)
        return hour >= start && hour < end;
    return hour >= start || hour < end;
}

// special CheckHourWindow: VAR_RESULT = TRUE when the clock's hour is in
// [VAR_0x8004, VAR_0x8005). Use the checkhour macro.
void CheckHourWindow(void)
{
    gSpecialVar_Result = IsHourInWindow(GetCurrentHour(), gSpecialVar_0x8004, gSpecialVar_0x8005);
}

// special GetTimeOfDayForScript: VAR_RESULT = TIME_OF_DAY_*. Use the checktimeofday macro.
void GetTimeOfDayForScript(void)
{
    gSpecialVar_Result = GetTimeOfDay();
}

// special ShowObjectDuringHours: clears flag VAR_0x8004 (the object's hide flag) when
// the hour is in [VAR_0x8005, VAR_0x8006) and sets it otherwise. Call it from a map's
// ON_TRANSITION script, before the objects spawn. Use the showduringhours macro.
void ShowObjectDuringHours(void)
{
    if (IsHourInWindow(GetCurrentHour(), gSpecialVar_0x8005, gSpecialVar_0x8006))
        FlagClear(gSpecialVar_0x8004);
    else
        FlagSet(gSpecialVar_0x8004);
}

// special ShowObjectDuringTimeOfDay: as above for a TIME_OF_DAY_* in VAR_0x8005.
// Use the showduringtime macro.
void ShowObjectDuringTimeOfDay(void)
{
    if (GetTimeOfDay() == gSpecialVar_0x8005)
        FlagClear(gSpecialVar_0x8004);
    else
        FlagSet(gSpecialVar_0x8004);
}

static u32 CountBadges(void)
{
    u32 i, count = 0;

    for (i = FLAG_BADGE01_GET; i < FLAG_BADGE01_GET + NUM_BADGES; i++)
    {
        if (FlagGet(i))
            count++;
    }
    return count;
}

// special IsInnRoomOpen: VAR_RESULT = TRUE when room VAR_0x8004 (INN_ROOM_*) is open.
bool8 IsInnRoomOpen(void)
{
    bool8 open;

    switch (gSpecialVar_0x8004)
    {
    case INN_ROOM_TAKE:
        open = VarGet(VAR_CHARISMA) >= INN_TAKE_CHARISMA || CountBadges() >= INN_TAKE_BADGES;
        break;
    case INN_ROOM_MATSU:
        open = FlagGet(FLAG_INN_MATSU_OPEN);
        break;
    default:
        open = TRUE;
        break;
    }
    gSpecialVar_Result = open;
    return open;
}

// special InnRest: the party rests in room VAR_0x8004 (INN_ROOM_*).
void InnRest(void)
{
    u32 i;

    RestPlayerParty();
    if (gSpecialVar_0x8004 >= INN_ROOM_TAKE)
    {
        // A fresh stay replaces an item still due from the last one.
        FlagSet(FLAG_HIDE_INN_TAKE_ITEM);
        VarSet(VAR_INN_TAKE_ITEM_STEPS, INN_TAKE_ITEM_STEPS);
    }
    if (gSpecialVar_0x8004 == INN_ROOM_MATSU)
    {
        for (i = 0; i < PARTY_SIZE; i++)
        {
            if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE)
            {
                s32 friendship = GetMonData(&gPlayerParty[i], MON_DATA_FRIENDSHIP) + INN_MATSU_FRIENDSHIP;
                u8 value = min(friendship, MAX_FRIENDSHIP);

                SetMonData(&gPlayerParty[i], MON_DATA_FRIENDSHIP, &value);
            }
        }
    }
}

// Every step in the field: counts down to the Take room's item.
void UpdateInnStepCounter(void)
{
    u16 *steps = GetVarPointer(VAR_INN_TAKE_ITEM_STEPS);

    if (*steps != 0 && --(*steps) == 0)
        FlagClear(FLAG_HIDE_INN_TAKE_ITEM);
}
