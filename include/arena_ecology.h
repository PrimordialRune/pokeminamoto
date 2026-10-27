#ifndef GUARD_ARENA_ECOLOGY_H
#define GUARD_ARENA_ECOLOGY_H

// Arena ecology: a gym's field rules (docs/design/boss-battles.md).
// The gym script sets VAR_BATTLE_ARENA before the battle; the state lives in gBattleStruct.

enum
{
    ARENA_NONE,
    ARENA_LUCKY_STRIKE, // Yujirou, Tokiwa: air vents, first-mover crits
    ARENA_GRUNGE_ROCK,  // Takeshi, Nibi: rocky ground, humid air
    ARENA_FAULT_LINE,   // Takeshi, Nibi (slice): no effect yet, every arena hook treats it as ARENA_NONE until the takeshi workstream implements it
    ARENA_COUNT
};

// Habitat-interaction traits (studio proposal v1, PLACEHOLDER values for the arena ecology thread).
// A species has any number of them (u16 bitmask in ROM, 2 bytes x NUM_SPECIES = 304 B, no RAM,
// no save change). They are NOT the habitat and NOT the type: Doduo is a Flying type without
// AERIAL, Gyarados is a Flying type that swims. Species left out of the table have no trait.
#define TRAIT_AERIAL      (1 << 0)
#define TRAIT_MOUNTAIN    (1 << 1)
#define TRAIT_BURROWER    (1 << 2)
#define TRAIT_AQUATIC     (1 << 3)
#define TRAIT_ROOTED      (1 << 4)
#define TRAIT_SQUEEZER    (1 << 5)
#define TRAIT_HEAVY       (1 << 6)
#define TRAIT_FLOATER     (1 << 7)
#define TRAIT_NOCTURNAL   (1 << 8)
#define TRAIT_THERMAL     (1 << 9)
#define TRAIT_CONDUCTIVE  (1 << 10)
#define TRAIT_SHELLED     (1 << 11)
#define TRAIT_SENSITIVE   (1 << 12)
// bits 13-15 are free

extern const u16 gSpeciesArenaTraits[NUM_SPECIES];

// Out-of-range species (eggs, none) have no trait.
#define SPECIES_HAS_TRAIT(species, trait) ((species) < NUM_SPECIES \
                                         && (gSpeciesArenaTraits[species] & (trait)) != 0)

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
