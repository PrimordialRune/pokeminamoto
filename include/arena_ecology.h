#ifndef GUARD_ARENA_ECOLOGY_H
#define GUARD_ARENA_ECOLOGY_H

// Arena ecology: a gym's field rules (docs/design/boss-battles.md).
// The gym script sets VAR_BATTLE_ARENA before the battle; the state lives in gBattleStruct.

enum
{
    ARENA_NONE,
    ARENA_LUCKY_STRIKE, // Yujirou, Tokiwa: air vents, first-mover crits
    ARENA_GRUNGE_ROCK,  // RETIRED (Notion concept; Takeshi is Ground, so ARENA_FAULT_LINE replaced it). Nothing sets it; the code stays for the lab's old numbers
    ARENA_FAULT_LINE,   // Takeshi, Nibi: Ground, cracking floor, mud, dust (arena ecology v2.1 'Fault Line'; docs: studio ws/takeshi/fault-line.md)
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

// Fault Line terrain (BattleStruct::arenaTerrain). It only ever gets worse.
#define ARENA_TERRAIN_INTACT    0
#define ARENA_TERRAIN_CRACKED   1
#define ARENA_TERRAIN_FRACTURED 2

// First-fire battle messages: bit numbers of BattleStruct::arenaMsgShown/arenaMsgQueue and indexes of
// gArenaMessageStringIds (src/battle_message.c). Each rule says its line once per battle.
#define ARENA_MSG_VENTS_ROAR        0 // Lucky Strike: the vents run and a bird is on the field
#define ARENA_MSG_VENTS_CLOGGED     1 // Lucky Strike: a blocker stopped the vents
#define ARENA_MSG_GROUND_CRACKS     2 // Fault Line: terrain CRACKED
#define ARENA_MSG_FLOOR_MUD         3 // Fault Line: a Water hit made the floor MUDDY
#define ARENA_MSG_FLOOR_FRACTURED   4 // Fault Line: terrain FRACTURED
#define ARENA_MSG_DUST_STORM        5 // Fault Line: the arena raised a sandstorm
#define ARENA_MSG_COUNT             6

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
    // Fault Line (all PLACEHOLDER, arena ecology thread). Append only: the lab and the editor read this struct by position.
    u16 faultSpeedPct;          // field constant: Speed of everything that is not a terrain dweller (MOUNTAIN or BURROWER); 67 = -1 stage, shipped 80 (balance-r2)
    u16 faultDefIntactPct;      // Defense of MOUNTAIN mons while the floor is INTACT (Omar, q72: x2)
    u16 faultDefCrackedPct;     // ...while CRACKED
    u16 faultDefFracturedPct;   // ...while FRACTURED
    u16 faultCrackBlows;        // blows that turn INTACT into CRACKED
    u16 faultFractureBlows;     // blows that turn it into FRACTURED
    u16 faultHeavyPower;        // a hit of at least this power counts as two blows (0 = off)
    u16 faultRootBlows;         // extra blows from a Grass hit: roots crack the floor (0 = off)
    u16 faultMudTurns;          // turns a Water hit leaves the floor MUDDY (0 = Water never muddies; ARENA_FOR_REST_OF_BATTLE = for good)
    u16 faultMudStopsCracks;    // blows do not count while the floor is MUDDY ('Mud doesn't crack')
    u16 faultMudWaterPct;       // power of Water moves while MUDDY (the Squirtle answer: the splash)
    u16 faultWindStormTurns;    // a wind move on a DRY floor raises a sandstorm for this many turns (0 = off); on MUDDY it dries the floor
    u16 faultFractureStormTurns; // the floor FRACTURING raises a sandstorm for this many turns (0 = off)
    u16 faultDryWaterPct;       // power of Water moves on a DRY floor: the thirsty ground drinks the first splash (100 = off)
    u16 faultMudWaterMissStages; // accuracy stages Water moves lose on a MUDDY floor: the splash goes wide (0 = off)
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
const u8 *ArenaTakeMessageScript(void); // NULL, or the script that says the next queued first-fire message (src/battle_script_commands.c, moveend)

#endif // GUARD_ARENA_ECOLOGY_H
