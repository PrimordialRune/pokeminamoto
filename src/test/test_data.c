#include "global.h"
#include "test_runner.h"

// Data tests of the studio "data" workstream: the arena trait table, the Signature scope (default ROM
// and the STARTER_SIGNATURES_PROPOSAL switch), and the Yujirou-only rule for trainer Signatures.
// The functions are registered in gPureTests (src/test/test_pure.c).

#ifdef TEST_BUILD

#include "arena_ecology.h"
#include "battle.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "battle_util2.h"
#include "data.h"
#include "pokemon.h"
#include "special_move.h"
#include "constants/battle.h"
#include "constants/battle_move_effects.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/opponents.h"

// --- A: arena traits ----------------------------------------------------------------

// Five sample species: bits copied from studio/systems/traits.csv (PLACEHOLDER values, arena ecology thread)
void Test_ArenaTraitBits(void)
{
    ASSERT_EQ(gSpeciesArenaTraits[SPECIES_PIDGEOT], TRAIT_AERIAL);
    ASSERT_EQ(gSpeciesArenaTraits[SPECIES_ONIX], TRAIT_MOUNTAIN | TRAIT_BURROWER | TRAIT_HEAVY);
    ASSERT_EQ(gSpeciesArenaTraits[SPECIES_GYARADOS], TRAIT_AQUATIC | TRAIT_HEAVY); // a Flying type, not AERIAL
    ASSERT_EQ(gSpeciesArenaTraits[SPECIES_DODUO], 0);                              // a Flying type, no trait
    ASSERT_EQ(gSpeciesArenaTraits[SPECIES_MAGMAR], TRAIT_THERMAL | TRAIT_MOUNTAIN);
    ASSERT_TRUE(SPECIES_HAS_TRAIT(SPECIES_FARFETCHD, TRAIT_AERIAL));
    ASSERT_TRUE(!SPECIES_HAS_TRAIT(SPECIES_DODUO, TRAIT_AERIAL));
    ASSERT_TRUE(SPECIES_HAS_TRAIT(SPECIES_GEODUDE, TRAIT_MOUNTAIN));
    ASSERT_TRUE(!SPECIES_HAS_TRAIT(SPECIES_CHARMANDER, TRAIT_MOUNTAIN)); // stays Takeshi's hard case
    ASSERT_TRUE(!SPECIES_HAS_TRAIT(SPECIES_EGG, TRAIT_AERIAL));          // out of range: no trait
    ASSERT_TRUE(!SPECIES_HAS_TRAIT(SPECIES_NONE, TRAIT_MOUNTAIN));
}

// The 15 species of the old hand list are still mountain dwellers
void Test_ArenaMountainListKept(void)
{
    static const u16 old[] = {
        SPECIES_GEODUDE, SPECIES_GRAVELER, SPECIES_GOLEM, SPECIES_ONIX, SPECIES_SANDSHREW, SPECIES_SANDSLASH,
        SPECIES_DIGLETT, SPECIES_DUGTRIO, SPECIES_MANKEY, SPECIES_PRIMEAPE, SPECIES_MACHOP, SPECIES_MACHOKE,
        SPECIES_MACHAMP, SPECIES_RHYHORN, SPECIES_RHYDON,
    };
    u32 i;

    for (i = 0; i < ARRAY_COUNT(old); i++)
        ASSERT_TRUE(SPECIES_HAS_TRAIT(old[i], TRAIT_MOUNTAIN));
}

// --- B: Signature scope -------------------------------------------------------------

static void BattleSetupTrainer(u16 trainerId)
{
    u32 i;

    AllocateBattleResources();
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    gTrainerBattleOpponent_A = trainerId;
    gBattlersCount = 2;
    gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;
    gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;
    gBattlerPartyIndexes[0] = 0;
    gBattlerPartyIndexes[1] = 0;
    memset(gBattleMons, 0, sizeof(gBattleMons));
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gEnemyParty[i]);
}

static void MakeLeaderParty(u16 aceSpecies)
{
    CreateMon(&gEnemyParty[0], SPECIES_RATTATA, 5, 0, FALSE, 0, OT_ID_PRESET, 1);
    CreateMon(&gEnemyParty[1], aceSpecies, 10, 0, FALSE, 0, OT_ID_PRESET, 1);
    gBattlerPartyIndexes[1] = 1; // the last slot
}

// Only Farfetch'd (LEEK SLAP) is a trainer Signature. A leader's last-slot Onix has none, in every build.
void Test_LeaderOnixHasNoSignature(void)
{
    ASSERT_EQ(gTrainers[TRAINER_YUJIROU].trainerClass, TRAINER_CLASS_LEADER);
    ASSERT_EQ(gSpecialMoves[SPECIES_ONIX].move, MOVE_NONE);

    BattleSetupTrainer(TRAINER_YUJIROU);
    MakeLeaderParty(SPECIES_ONIX);
    ASSERT_EQ(GetBattlerSpecialMove(1), MOVE_NONE);
    // The ace of a non-leader never has one either
    MakeLeaderParty(SPECIES_FARFETCHD);
    ASSERT_EQ(GetBattlerSpecialMove(1), MOVE_LEEK_SLAP); // control: Yujirou's ace
    gBattlerPartyIndexes[1] = 0;
    ASSERT_EQ(GetBattlerSpecialMove(1), MOVE_NONE);       // not the last slot
    gTrainerBattleOpponent_A = TRAINER_CIRCUIT_TOKIWA_1;
    gBattlerPartyIndexes[1] = 1;
    ASSERT_EQ(GetBattlerSpecialMove(1), MOVE_NONE);       // not a leader
    FreeBattleResources();
}

#if STARTER_SIGNATURES_PROPOSAL
// Own deed on the player's side: GREEDY PLANT after 3 wins with a Rock or Ground foe fainting, SKULL BASH
// after 4 with a Bug or Poison foe. One helper plays a battle in which the foe of `foeType` faints.
static void PlayWin(struct Pokemon *mon, u16 foeSpecies, bool8 foeFaints)
{
    gPlayerParty[0] = *mon;
    memset(gBattleMons, 0, sizeof(gBattleMons));
    gBattleMons[0].species = GetMonData(&gPlayerParty[0], MON_DATA_SPECIES);
    gBattleMons[0].hp = 10;
    gBattleMons[1].species = foeSpecies;
    gBattleMons[1].types[0] = gSpeciesInfo[foeSpecies].types[0];
    gBattleMons[1].types[1] = gSpeciesInfo[foeSpecies].types[1];
    gBattleStruct->specialDeedMons = 0;
    if (foeFaints)
        SpecialMove_OnFoeFainted(1);
    SpecialMove_OnBattleWon();
    *mon = gPlayerParty[0];
}

static void ResetParty(struct Pokemon *mon, u16 species)
{
    u32 i;

    BattleSetupTrainer(TRAINER_CIRCUIT_TOKIWA_1);
    gBattleTypeFlags = 0;
    for (i = 0; i < PARTY_SIZE; i++)
        ZeroMonData(&gPlayerParty[i]);
    CreateMon(&gPlayerParty[0], species, 20, 31, TRUE, 0, OT_ID_PRESET, 1);
    *mon = gPlayerParty[0];
}

static void CheckAwakens(u16 species, u16 move, u8 count, u16 deedFoe, u16 otherFoe)
{
    struct Pokemon mon;
    u32 i;

    ResetParty(&mon, species);
    ASSERT_EQ(GetMonSpecialMove(&mon), move);
    ASSERT_EQ(IsMonSpecialAwakened(&mon), FALSE);
    // a foe of another type is no deed, and a won battle without a faint is no deed
    PlayWin(&mon, otherFoe, TRUE);
    PlayWin(&mon, deedFoe, FALSE);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_SPECIAL_PROGRESS), 0);
    for (i = 0; i < count - 1u; i++)
        PlayWin(&mon, deedFoe, TRUE);
    gPlayerParty[0] = mon;
    ASSERT_EQ(SpecialMove_TryAwakenNext(), -1);
    PlayWin(&mon, deedFoe, TRUE);
    gPlayerParty[0] = mon;
    ASSERT_EQ(GetMonData(&mon, MON_DATA_SPECIAL_PROGRESS), count);
    ASSERT_EQ(SpecialMove_TryAwakenNext(), 0);
    mon = gPlayerParty[0];
    ASSERT_EQ(IsMonSpecialAwakened(&mon), TRUE);
    FreeBattleResources();
}

void Test_GreedyPlantAwakens(void)
{
    CheckAwakens(SPECIES_BULBASAUR, MOVE_GREEDY_PLANT, 3, SPECIES_GEODUDE, SPECIES_PIDGEY);
    CheckAwakens(SPECIES_IVYSAUR, MOVE_GREEDY_PLANT, 3, SPECIES_SANDSHREW, SPECIES_RATTATA); // Ground
}

void Test_SkullBashAwakens(void)
{
    CheckAwakens(SPECIES_SQUIRTLE, MOVE_SKULL_BASH_SIGNATURE, 4, SPECIES_CATERPIE, SPECIES_PIDGEY);   // Bug
    CheckAwakens(SPECIES_BLASTOISE, MOVE_SKULL_BASH_SIGNATURE, 4, SPECIES_EKANS, SPECIES_RATTATA);    // Poison
}

// Names, numbers and scope of the two proposals
void Test_StarterSignatureData(void)
{
    ASSERT_EQ(gMovesInfo[MOVE_GREEDY_PLANT].effect, EFFECT_ABSORB);
    ASSERT_EQ(gMovesInfo[MOVE_GREEDY_PLANT].power, 90);
    ASSERT_EQ(gMovesInfo[MOVE_GREEDY_PLANT].type, TYPE_GRASS);
    ASSERT_EQ(gMovesInfo[MOVE_GREEDY_PLANT].tpCost, 45);
    ASSERT_EQ(gMovesInfo[MOVE_SKULL_BASH_SIGNATURE].effect, EFFECT_SKULL_BASH);
    ASSERT_EQ(gMovesInfo[MOVE_SKULL_BASH_SIGNATURE].power, 100);
    ASSERT_EQ(gMovesInfo[MOVE_SKULL_BASH_SIGNATURE].type, TYPE_WATER);
    ASSERT_EQ(gMovesInfo[MOVE_SKULL_BASH_SIGNATURE].tpCost, 45);
    ASSERT_TRUE(IS_MOVE_SIGNATURE(MOVE_GREEDY_PLANT));
    ASSERT_TRUE(IS_MOVE_SIGNATURE(MOVE_SKULL_BASH_SIGNATURE));
    ASSERT_TRUE(!IS_MOVE_SIGNATURE(MOVE_SKULL_BASH)); // the regular Gen 1 move stays a regular move
    ASSERT_EQ(gMovesInfo[MOVE_SKULL_BASH].type, TYPE_NORMAL);
    ASSERT_EQ(gSpecialMoves[SPECIES_VENUSAUR].move, MOVE_GREEDY_PLANT);
    ASSERT_EQ(gSpecialMoves[SPECIES_BLASTOISE].move, MOVE_SKULL_BASH_SIGNATURE);
}
#else
// Default ROM: no new Signature. Only Charmander's line and Farfetch'd have one.
void Test_DefaultRomNoNewSignature(void)
{
    u32 species, count = 0;

    ASSERT_EQ(MOVES_COUNT, 357);
    for (species = 0; species < NUM_SPECIES; species++)
        if (gSpecialMoves[species].move != MOVE_NONE)
            count++;
    ASSERT_EQ(count, 4);
    ASSERT_EQ(gSpecialMoves[SPECIES_BULBASAUR].move, MOVE_NONE);
    ASSERT_EQ(gSpecialMoves[SPECIES_SQUIRTLE].move, MOVE_NONE);
    ASSERT_EQ(gSpecialMoves[SPECIES_CHARMANDER].move, MOVE_FLAME_CLAWS);
    ASSERT_EQ(gSpecialMoves[SPECIES_FARFETCHD].move, MOVE_LEEK_SLAP);
}
#endif

#endif // TEST_BUILD
