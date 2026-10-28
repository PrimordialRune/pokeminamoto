#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "arena_ecology.h"
#include "event_data.h"
#include "item.h"
#include "constants/battle.h"
#include "constants/battle_move_effects.h"
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
// GRUNGE ROCK (retired: Takeshi is Ground, not Rock): rocks slow every mon but mountain dwellers and fliers, mountain
// dwellers take hits on their Defense twice as well, and the humid air weakens Fire. Water wets the foe's
// side (lower evasion); Gust raises a light sandstorm for 3 turns. Nothing sets ARENA_GRUNGE_ROCK any more.
//
// FAULT LINE (Takeshi, Nibi; arena ecology v2.1, all numbers PLACEHOLDER for the thread "Design arena ecology for gym
// battles"; design and lab results: studio ws/takeshi/fault-line.md). The arena floor sits on a fault.
//  - TERRAIN INTACT -> CRACKED -> FRACTURED as blows land (every damaging hit is a blow, a heavy hit and a Grass hit
//    count extra, Explosion/Self-Destruct fracture at once). It never heals.
//  - Mountain dwellers (MOUNTAIN trait) take hits on their Defense twice as well (q72), but only while the footing holds:
//    x2 INTACT, x1.5 CRACKED, x1 FRACTURED. So blows are how the player takes the fortress apart.
//  - MOISTURE DRY / MUDDY: a Water hit makes the floor muddy for a few turns. Mud doesn't crack (blows stop counting) and
//    a Water move on mud splashes (power cut): Water is a good first hit and a bad plan (the answer to 'Squirtle 96%').
//    A wind move dries the mud; on a dry floor it raises a sandstorm instead (SANDSTORM overlay).
//  - Field constant: -1 Speed (x0.67) for everything that is not a terrain dweller (MOUNTAIN or BURROWER trait).
// Messages: each rule says its line once per battle (BattleStruct arenaMsgShown, queue in arenaMsgQueue, shown at move end).

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
    .faultSpeedPct = 67,
    .faultDefIntactPct = 200,
    .faultDefCrackedPct = 150,
    .faultDefFracturedPct = 100,
    .faultCrackBlows = 3,
    .faultFractureBlows = 7,
    .faultHeavyPower = 80,
    .faultRootBlows = 1,
    .faultMudTurns = 4,
    .faultMudStopsCracks = TRUE,
    .faultMudWaterPct = 60,
    .faultWindStormTurns = 3,
    .faultFractureStormTurns = 0,
    .faultDryWaterPct = 80,
    .faultMudWaterMissStages = 1,
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

static bool8 IsTerrainDweller(u16 species)
{
    return SPECIES_HAS_TRAIT(species, TRAIT_MOUNTAIN | TRAIT_BURROWER);
}

extern const u8 BattleScript_ArenaMessage[];

// Queues a first-fire message unless this battle has said it already.
static void QueueMessage(u8 msg)
{
    u8 bit = 1 << msg;

    if (gBattleStruct->arenaMsgShown & bit)
        return;
    gBattleStruct->arenaMsgShown |= bit;
    gBattleStruct->arenaMsgQueue |= bit;
}

#ifdef TEST_BUILD
EWRAM_DATA u8 gTestArenaMsgPrinted[ARENA_MSG_COUNT] = {0}; // test builds only: how often each message script really ran
#endif

const u8 *ArenaTakeMessageScript(void)
{
    u32 i;

    if (gBattleStruct == NULL || gBattleStruct->arenaMsgQueue == 0)
        return NULL;
    for (i = 0; i < ARENA_MSG_COUNT; i++)
    {
        if (gBattleStruct->arenaMsgQueue & (1 << i))
        {
            gBattleStruct->arenaMsgQueue &= ~(1 << i);
            gBattleCommunication[MULTISTRING_CHOOSER] = i;
#ifdef TEST_BUILD
            gTestArenaMsgPrinted[i]++;
#endif
            return BattleScript_ArenaMessage;
        }
    }
    return NULL;
}

static bool8 VentsRunning(void)
{
    return gBattleStruct->arenaVentsBlocked == 0;
}

// Returns TRUE if it started a storm (a blowing one is left alone).
static bool8 RaiseSandstorm(u16 turns)
{
    if (turns == 0 || (gBattleWeather & B_WEATHER_SANDSTORM))
        return FALSE;
    gBattleWeather = B_WEATHER_SANDSTORM_TEMPORARY;
    gWishFutureKnock.weatherDuration = turns;
    return TRUE;
}

void ArenaBattleStart(void)
{
    u16 arena = VarGet(VAR_BATTLE_ARENA);

    gBattleStruct->arenaId = arena < ARENA_COUNT ? arena : ARENA_NONE;
    gBattleStruct->arenaVentsBlocked = 0;
    gBattleStruct->arenaWet[0] = gBattleStruct->arenaWet[1] = 0;
    gBattleStruct->arenaTerrain = ARENA_TERRAIN_INTACT;
    gBattleStruct->arenaMud = 0;
    gBattleStruct->arenaBlows = 0;
    gBattleStruct->arenaMsgShown = 0;
    gBattleStruct->arenaMsgQueue = 0;
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
    case ARENA_FAULT_LINE:
        if (!IsTerrainDweller(gBattleMons[battler].species))
            speed = speed * TUNE.faultSpeedPct / 100;
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
    if (GetBattleArena() == ARENA_FAULT_LINE)
    {
        u16 pct;

        switch (gBattleStruct->arenaTerrain)
        {
        case ARENA_TERRAIN_INTACT: pct = TUNE.faultDefIntactPct; break;
        case ARENA_TERRAIN_CRACKED: pct = TUNE.faultDefCrackedPct; break;
        default: pct = TUNE.faultDefFracturedPct; break;
        }
        if (IsMountainDweller(defender->species))
            *defense = *defense * pct / 100;
        if (type == TYPE_WATER)
            gBattleMovePower = gBattleMovePower * (gBattleStruct->arenaMud ? TUNE.faultMudWaterPct : TUNE.faultDryWaterPct) / 100;
        return;
    }
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
    if (GetBattleArena() == ARENA_FAULT_LINE && gBattleStruct->arenaMud
     && gMovesInfo[gCurrentMove].type == TYPE_WATER && gMovesInfo[gCurrentMove].power != 0)
        return TUNE.faultMudWaterMissStages;
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

static void FaultLineSetTerrain(u8 terrain)
{
    if (terrain <= gBattleStruct->arenaTerrain)
        return;
    gBattleStruct->arenaTerrain = terrain;
    if (terrain == ARENA_TERRAIN_CRACKED)
        QueueMessage(ARENA_MSG_GROUND_CRACKS);
    else
    {
        QueueMessage(ARENA_MSG_FLOOR_FRACTURED);
        if (RaiseSandstorm(TUNE.faultFractureStormTurns))
            QueueMessage(ARENA_MSG_DUST_STORM);
    }
}

// One move has just been used: its blows land on the floor, Water muddies it, wind dries it or stirs the dust.
static void FaultLineAfterMove(u16 move, u8 attacker, u8 target)
{
    bool8 hit = gMovesInfo[move].power != 0 && attacker != target
             && !(gMoveResultFlags & (MOVE_RESULT_NO_EFFECT | MOVE_RESULT_MISSED));
    u32 blows;

    if (gMovesInfo[move].effect == EFFECT_EXPLOSION)
        FaultLineSetTerrain(ARENA_TERRAIN_FRACTURED);
    if (hit && !(gBattleStruct->arenaMud && TUNE.faultMudStopsCracks))
    {
        blows = 1;
        if (TUNE.faultHeavyPower && gMovesInfo[move].power >= TUNE.faultHeavyPower)
            blows++;
        if (gMovesInfo[move].type == TYPE_GRASS)
            blows += TUNE.faultRootBlows;
        blows += gBattleStruct->arenaBlows;
        gBattleStruct->arenaBlows = blows > 255 ? 255 : blows;
        if (gBattleStruct->arenaBlows >= TUNE.faultFractureBlows)
            FaultLineSetTerrain(ARENA_TERRAIN_FRACTURED);
        else if (gBattleStruct->arenaBlows >= TUNE.faultCrackBlows)
            FaultLineSetTerrain(ARENA_TERRAIN_CRACKED);
    }
    if (hit && gMovesInfo[move].type == TYPE_WATER && TUNE.faultMudTurns)
    {
        if (gBattleStruct->arenaMud == 0)
            QueueMessage(ARENA_MSG_FLOOR_MUD);
        gBattleStruct->arenaMud = TUNE.faultMudTurns;
    }
    if (IsWindMove(move))
    {
        if (gBattleStruct->arenaMud)
            gBattleStruct->arenaMud = 0;
        else if (RaiseSandstorm(TUNE.faultWindStormTurns))
            QueueMessage(ARENA_MSG_DUST_STORM);
    }
}

static bool8 AerialOnField(void)
{
    u32 i;

    for (i = 0; i < gBattlersCount; i++)
        if (gBattleMons[i].hp != 0 && IsAerial(gBattleMons[i].species))
            return TRUE;
    return FALSE;
}

// Called once per used move, hit or miss
void ArenaAfterMove(u16 move, u8 attacker, u8 target)
{
    switch (GetBattleArena())
    {
    case ARENA_LUCKY_STRIKE:
    {
        bool8 wasRunning = VentsRunning();

        if (BlocksVents(move))
            gBattleStruct->arenaVentsBlocked = TUNE.ventBlockTurns ? TUNE.ventBlockTurns : 1;
        else if (TUNE.windRestartsVents && IsWindMove(move))
            gBattleStruct->arenaVentsBlocked = 0;
        if (move == MOVE_SAND_ATTACK)
            RaiseSandstorm(TUNE.sandAttackStormTurns);
        if (wasRunning && !VentsRunning())
            QueueMessage(ARENA_MSG_VENTS_CLOGGED);
        else if (VentsRunning() && AerialOnField())
            QueueMessage(ARENA_MSG_VENTS_ROAR);
        break;
    }
    case ARENA_FAULT_LINE:
        FaultLineAfterMove(move, attacker, target);
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
    if (gBattleStruct->arenaMud && gBattleStruct->arenaMud != ARENA_FOR_REST_OF_BATTLE)
        gBattleStruct->arenaMud--;
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
