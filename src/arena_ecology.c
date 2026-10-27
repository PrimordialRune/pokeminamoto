#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "arena_ecology.h"
#include "event_data.h"
#include "item.h"
#include "constants/battle.h"
#include "constants/hold_effects.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/vars.h"

// Arena ecology: each gym's field rules, from Omar's BOSS BATTLES concept (Notion).
// docs/design/boss-battles.md has the concept; the knobs below are swept by the Balance Lab (minasim/arena.py).
//
// LUCKY STRIKE (Yujirou): air vents run; while they do, AERIAL species (trait bit, not the Flying type) are twice as fast. The mon that
// moves first in a turn has a better crit chance and Farfetch'd with the Leek always crits.
// String Shot blocks the vents; Sand Attack raises a sandstorm (1/16 a turn to all but Rock/Ground).
//
// GRUNGE ROCK (Takeshi): rocks slow every mon but mountain dwellers and fliers, mountain dwellers
// take hits on their Defense twice as well, and the humid air weakens Fire. Water wets the foe's
// side (lower evasion); Gust raises a light sandstorm for 3 turns.

// The balance lab (tools/balance_sim) builds this file with the table writable to sweep the knobs.
const struct ArenaTuning gArenaTuning =
{
    .ventFlyerSpeedPct = 200,
    .stillFlyerSpeedPct = 50,        // blocked vents: the birds lose the current (Omar, answer 68)
    .firstMoverCritStages = 1,
    .firstMoverNeedsVents = FALSE,
    .leekAlwaysCrits = FALSE,        // a high crit ratio, not a sure crit (answer 70)
    .leekNeedsVents = FALSE,
    .ventBlockTurns = ARENA_FOR_REST_OF_BATTLE,
    .ventBlockers = ARENA_BLOCK_STRING_SHOT | ARENA_BLOCK_OBSTRUCT | ARENA_BLOCK_VINES | ARENA_BLOCK_SAND, // obstruction moves (answer 69)
    .windRestartsVents = FALSE,
    .sandAttackStormTurns = 5,
    .rockSpeedPct = 67,
    .rockDefPct = 200,
    .rockSpDefPct = 100,
    .rockDefFlyers = TRUE,
    .rockFirePowerPct = 100,         // humidity Fire cut dropped for now (answer 75)
    .wetTurns = 5,
    .wetEvasionStages = 1,
    .gustStormTurns = 3,
    .rockSandAttackStormTurns = 0,
    .rockSandSpDefPct = 100,
    .seedDrainPct = 100,
};

#define TUNE gArenaTuning

#include "data/pokemon/arena_traits.h"

// Mountain dwellers and fliers come from the species trait table (MOUNTAIN and AERIAL bits).
static bool8 IsMountainDweller(u16 species)
{
    return SPECIES_HAS_TRAIT(species, TRAIT_MOUNTAIN);
}

static bool8 IsAerial(u16 species)
{
    return SPECIES_HAS_TRAIT(species, TRAIT_AERIAL);
}

static bool8 VentsRunning(void)
{
    return gBattleStruct->arenaVentsBlocked == 0;
}

static void RaiseSandstorm(u16 turns)
{
    if (turns == 0 || (gBattleWeather & B_WEATHER_SANDSTORM))
        return;
    gBattleWeather = B_WEATHER_SANDSTORM_TEMPORARY;
    gWishFutureKnock.weatherDuration = turns;
}

void ArenaBattleStart(void)
{
    u16 arena = VarGet(VAR_BATTLE_ARENA);

    gBattleStruct->arenaId = arena < ARENA_COUNT ? arena : ARENA_NONE;
    gBattleStruct->arenaVentsBlocked = 0;
    gBattleStruct->arenaWet[0] = gBattleStruct->arenaWet[1] = 0;
}

u8 GetBattleArena(void)
{
    return gBattleStruct ? gBattleStruct->arenaId : ARENA_NONE;
}

u32 ArenaModifySpeed(u8 battler, u32 speed)
{
    switch (GetBattleArena())
    {
    case ARENA_LUCKY_STRIKE:
        if (IsAerial(gBattleMons[battler].species))
            speed = speed * (VentsRunning() ? TUNE.ventFlyerSpeedPct : TUNE.stillFlyerSpeedPct) / 100;
        break;
    case ARENA_GRUNGE_ROCK:
        if (!IsAerial(gBattleMons[battler].species) && !IsMountainDweller(gBattleMons[battler].species))
            speed = speed * TUNE.rockSpeedPct / 100;
        break;
    }
    return speed;
}

u16 ArenaCritStages(u8 attacker)
{
    if (GetBattleArena() == ARENA_LUCKY_STRIKE
     && gBattlerByTurnOrder[0] == attacker
     && (!TUNE.firstMoverNeedsVents || VentsRunning()))
        return TUNE.firstMoverCritStages;
    return 0;
}

bool8 ArenaForcesCrit(u8 attacker)
{
    return GetBattleArena() == ARENA_LUCKY_STRIKE
        && TUNE.leekAlwaysCrits
        && (!TUNE.leekNeedsVents || VentsRunning())
        && gBattleMons[attacker].species == SPECIES_FARFETCHD
        && GetItemHoldEffect(gBattleMons[attacker].item) == HOLD_EFFECT_STICK
        && gMovesInfo[gCurrentMove].power != 0;
}

void ArenaModifyDamage(struct BattlePokemon *attacker, struct BattlePokemon *defender, u8 type, u16 *defense, u16 *spDefense)
{
    if (GetBattleArena() != ARENA_GRUNGE_ROCK)
        return;
    if (IsMountainDweller(defender->species)
     || (TUNE.rockDefFlyers && IsAerial(defender->species)))
    {
        *defense = *defense * TUNE.rockDefPct / 100;
        *spDefense = *spDefense * TUNE.rockSpDefPct / 100;
        if (gBattleWeather & B_WEATHER_SANDSTORM)
            *spDefense = *spDefense * TUNE.rockSandSpDefPct / 100;
    }
    if (type == TYPE_FIRE)
        gBattleMovePower = gBattleMovePower * TUNE.rockFirePowerPct / 100;
}

s8 ArenaEvasionStages(u8 target)
{
    if (GetBattleArena() == ARENA_GRUNGE_ROCK && gBattleStruct->arenaWet[GetBattlerSide(target)])
        return -(s8)TUNE.wetEvasionStages;
    return 0;
}

static bool8 BlocksVents(u16 move)
{
    switch (move)
    {
    case MOVE_STRING_SHOT:
        return (TUNE.ventBlockers & ARENA_BLOCK_STRING_SHOT) != 0;
    case MOVE_SMOKESCREEN:
    case MOVE_POISON_POWDER:
    case MOVE_STUN_SPORE:
    case MOVE_SLEEP_POWDER:
    case MOVE_SPORE:
        return (TUNE.ventBlockers & ARENA_BLOCK_OBSTRUCT) != 0;
    case MOVE_VINE_WHIP:
    case MOVE_LEECH_SEED:
        return (TUNE.ventBlockers & ARENA_BLOCK_VINES) != 0;
    case MOVE_SAND_ATTACK:
        return (TUNE.ventBlockers & ARENA_BLOCK_SAND) != 0;
    }
    return FALSE;
}

static bool8 IsWindMove(u16 move)
{
    switch (move)
    {
    case MOVE_GUST:
    case MOVE_WHIRLWIND:
    case MOVE_RAZOR_WIND:
    case MOVE_WING_ATTACK:
    case MOVE_SKY_ATTACK:
        return TRUE;
    }
    return FALSE;
}

// Called once per used move, hit or miss
void ArenaAfterMove(u16 move, u8 attacker, u8 target)
{
    switch (GetBattleArena())
    {
    case ARENA_LUCKY_STRIKE:
        if (BlocksVents(move))
            gBattleStruct->arenaVentsBlocked = TUNE.ventBlockTurns ? TUNE.ventBlockTurns : 1;
        else if (TUNE.windRestartsVents && IsWindMove(move))
            gBattleStruct->arenaVentsBlocked = 0;
        if (move == MOVE_SAND_ATTACK)
            RaiseSandstorm(TUNE.sandAttackStormTurns);
        break;
    case ARENA_GRUNGE_ROCK:
        if (gMovesInfo[move].type == TYPE_WATER && gMovesInfo[move].power != 0
         && !(gMoveResultFlags & MOVE_RESULT_NO_EFFECT) && TUNE.wetTurns)
            gBattleStruct->arenaWet[GetBattlerSide(target)] = TUNE.wetTurns;
        if (IsWindMove(move))
            RaiseSandstorm(TUNE.gustStormTurns);
        if (move == MOVE_SAND_ATTACK)
            RaiseSandstorm(TUNE.rockSandAttackStormTurns);
        break;
    }
}

void ArenaEndTurn(void)
{
    u32 i;

    if (GetBattleArena() == ARENA_NONE)
        return;
    if (gBattleStruct->arenaVentsBlocked && gBattleStruct->arenaVentsBlocked != ARENA_FOR_REST_OF_BATTLE)
        gBattleStruct->arenaVentsBlocked--;
    for (i = 0; i < NUM_BATTLE_SIDES; i++)
        if (gBattleStruct->arenaWet[i] && gBattleStruct->arenaWet[i] != ARENA_FOR_REST_OF_BATTLE)
            gBattleStruct->arenaWet[i]--;
}

u32 ArenaLeechSeedDrain(u32 damage)
{
    if (GetBattleArena() == ARENA_LUCKY_STRIKE)
        damage = damage * TUNE.seedDrainPct / 100;
    return damage;
}
