#ifndef GUARD_ARENA_ECOLOGY_H
#define GUARD_ARENA_ECOLOGY_H

// Arena ecology: a gym's field rules (docs/design/boss-battles.md).
// The gym script sets VAR_BATTLE_ARENA before the battle; the state lives in gBattleStruct.

enum
{
    ARENA_NONE,
    ARENA_LUCKY_STRIKE, // Yujirou, Tokiwa: air vents, first-mover crits
    ARENA_GRUNGE_ROCK,  // Takeshi, Nibi: rocky ground, humid air
    ARENA_COUNT
};

#define ARENA_FOR_REST_OF_BATTLE 0xFF

// Tuning knobs. Percentages are of the normal value (100 = unchanged).
struct ArenaTuning
{
    // Lucky Strike
    u16 ventFlyerSpeedPct;      // Flying-type Speed while the vents run
    u16 stillFlyerSpeedPct;     // Flying-type Speed while the vents are blocked
    u16 firstMoverCritStages;   // extra crit stages for the mon that moves first in the turn
    u16 firstMoverNeedsVents;   // the first-mover bonus only while the vents run
    u16 leekAlwaysCrits;        // Farfetch'd holding the Leek always crits (stands in for Leek Slap)
    u16 leekNeedsVents;         // ...only while the vents run
    u16 ventBlockTurns;         // turns a blocking move stops the vents (ARENA_FOR_REST_OF_BATTLE = for good)
    u16 ventBlockers;           // ARENA_BLOCK_* bits: which moves block the vents
    u16 windRestartsVents;      // Gust, Whirlwind and other wind moves restart blocked vents
    u16 sandAttackStormTurns;   // Sand Attack raises a sandstorm for this many turns (0 = off)
    // Grunge Rock
    u16 rockSpeedPct;           // Speed of mons that are neither mountain dwellers nor Flying
    u16 rockDefPct;             // Defense of mountain dwellers
    u16 rockSpDefPct;           // Sp. Def of mountain dwellers
    u16 rockDefFlyers;          // Flying types get the Defense bonus too
    u16 rockFirePowerPct;       // power of Fire moves (humid air)
    u16 wetTurns;               // turns a Water hit leaves the target's side wet (0 = off)
    u16 wetEvasionStages;       // evasion stages a wet side loses
    u16 gustStormTurns;         // wind moves raise a light sandstorm for this many turns (0 = off)
    u16 rockSandAttackStormTurns; // Sand Attack raises a sandstorm in the rocky arena too (0 = off)
    u16 rockSandSpDefPct;       // Sp. Def of mountain dwellers while a sandstorm blows
    u16 seedDrainPct;           // Lucky Strike: Leech Seed drain, % of the normal 1/8 (lab test of a Bulbasaur lever)
};
u32 ArenaLeechSeedDrain(u32 damage);

#define ARENA_BLOCK_STRING_SHOT (1 << 0) // String Shot (Omar's concept)
#define ARENA_BLOCK_OBSTRUCT    (1 << 1) // the other sticky or clouding moves: Smokescreen, Poison Powder clouds...
#define ARENA_BLOCK_VINES       (1 << 2) // Vine Whip and Leech Seed grow into the ducts
#define ARENA_BLOCK_SAND        (1 << 3) // Sand Attack clogs the vents as well as raising the storm

extern const struct ArenaTuning gArenaTuning;

void ArenaBattleStart(void);
u8 GetBattleArena(void);
u32 ArenaModifySpeed(u8 battler, u32 speed);
u16 ArenaCritStages(u8 attacker);
bool8 ArenaForcesCrit(u8 attacker);
void ArenaModifyDamage(struct BattlePokemon *attacker, struct BattlePokemon *defender, u8 type, u16 *defense, u16 *spDefense);
s8 ArenaEvasionStages(u8 target);
void ArenaAfterMove(u16 move, u8 attacker, u8 target);
void ArenaEndTurn(void);

#endif // GUARD_ARENA_ECOLOGY_H
