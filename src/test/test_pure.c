#include "global.h"
#include "test_runner.h"

// Pure-function rule tests: TP, damage, type chart, stats, exp, RNG, mon data and encryption.
// Each test is a void function using ASSERT_*; add one to gPureTests at the bottom.
// TP tests read the TP_* constants and the move table, so a changed cost or regen formula
// only means editing the expected values in the tests marked "TP:".

#ifdef TEST_BUILD

#include "battle.h"
#include "battle_main.h"
#include "battle_util.h"
#include "battle_util2.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"

// --- helpers -------------------------------------------------------------------

static void MakeCharmander(struct Pokemon *mon, u8 level)
{
    CreateMon(mon, SPECIES_CHARMANDER, level, 31, TRUE, 0, OT_ID_PRESET, 0x00010002);
}

static void BattleSetup(void)
{
    AllocateBattleResources();
    gBattleTypeFlags = 0;
    gBattleWeather = 0;
    gCritMultiplier = 1;
    gBattlersCount = 2;
    gBattlerPositions[0] = B_POSITION_PLAYER_LEFT;
    gBattlerPositions[1] = B_POSITION_OPPONENT_LEFT;
    gBattlerPartyIndexes[0] = 0;
    gBattlerPartyIndexes[1] = 0;
    memset(gBattleMons, 0, sizeof(gBattleMons));
    InitBattleTP();
}

static void BattleTeardown(void)
{
    FreeBattleResources();
}

// A bare battler: neutral stages, no ability, no item
static void SetBattler(struct BattlePokemon *b, u16 species, u8 level, u16 atk, u16 def, u16 spAtk, u16 spDef)
{
    u32 i;

    memset(b, 0, sizeof(*b));
    b->species = species;
    b->level = level;
    b->attack = atk;
    b->defense = def;
    b->spAttack = spAtk;
    b->spDefense = spDef;
    b->hp = b->maxHP = 100;
    for (i = 0; i < NUM_BATTLE_STATS; i++)
        b->statStages[i] = DEFAULT_STAT_STAGE;
}

// Type multiplier from the table (10 = x1), scanning like TypeCalc: marker entries are skipped
static u8 TypeMul(u8 atk, u8 def)
{
    u32 i;

    for (i = 0; gTypeEffectiveness[i] != TYPE_ENDTABLE; i += 3)
    {
        if (gTypeEffectiveness[i] == TYPE_FORESIGHT)
            continue;
        if (gTypeEffectiveness[i] == atk && gTypeEffectiveness[i + 1] == def)
            return gTypeEffectiveness[i + 2];
    }
    // Entries after the Foresight marker still count (Normal/Fighting vs Ghost)
    for (i += 3; gTypeEffectiveness[i] != TYPE_ENDTABLE; i += 3)
    {
        if (gTypeEffectiveness[i] == atk && gTypeEffectiveness[i + 1] == def)
            return gTypeEffectiveness[i + 2];
    }
    return TYPE_MUL_NORMAL;
}

// --- TP ---------------------------------------------------------------------------

static void Test_TpCostTable(void)
{
    u32 move;

    // TP: every Gen 1 move costs 1..TP_MAX, Struggle is free
    for (move = MOVE_POUND; move < MOVE_STRUGGLE; move++)
    {
        if (GetMoveTPCost(move) == 0 || GetMoveTPCost(move) > TP_MAX)
            ASSERT_EQ(move, 0);
    }
    ASSERT_EQ(GetMoveTPCost(MOVE_STRUGGLE), 0);
    ASSERT_EQ(GetMoveTPCost(MOVE_NONE), 0);
}

static void Test_TpCostSamples(void)
{
    // TP: placeholder costs (status 10, damaging power^2/320 rounded to 5, Notion has Pound = 5)
    ASSERT_EQ(GetMoveTPCost(MOVE_POUND), 5);
    ASSERT_EQ(GetMoveTPCost(MOVE_SPLASH), 10);
    ASSERT_EQ(GetMoveTPCost(MOVE_FLAMETHROWER), 30);
    ASSERT_TRUE(GetMoveTPCost(MOVE_HYPER_BEAM) > GetMoveTPCost(MOVE_FLAMETHROWER));
}

static void Test_TpCostFollowsPower(void)
{
    u32 move;

    // TP: a stronger damaging move never costs less than a weaker one of the same type
    for (move = MOVE_POUND; move < MOVE_STRUGGLE; move++)
    {
        u32 other;

        if (gMovesInfo[move].power == 0)
            continue;
        for (other = MOVE_POUND; other < MOVE_STRUGGLE; other++)
        {
            if (gMovesInfo[other].power != 0 && gMovesInfo[other].power > gMovesInfo[move].power
             && gMovesInfo[other].type == gMovesInfo[move].type
             && gMovesInfo[other].effect == gMovesInfo[move].effect
             && GetMoveTPCost(other) < GetMoveTPCost(move))
                ASSERT_EQ(other, move);
        }
    }
}

static void Test_TpInit(void)
{
    u32 side, i;

    BattleSetup();
    for (side = 0; side < NUM_BATTLE_SIDES; side++)
        for (i = 0; i < PARTY_SIZE; i++)
            ASSERT_EQ(gBattleStruct->tp[side][i], TP_MAX);
    BattleTeardown();
}

static void Test_TpRegenFormula(void)
{
    u8 expect;

    BattleSetup();
    gBattleMons[0].species = SPECIES_CHARMANDER;
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 40;
    RegenBattlerTP(0);
    // TP: +TP_REGEN_BASE + INT / TP_REGEN_INT_DIVISOR, INT = base Sp. Def
    expect = 40 + TP_REGEN_BASE + gSpeciesInfo[SPECIES_CHARMANDER].baseSpDefense / TP_REGEN_INT_DIVISOR;
    ASSERT_EQ(GetBattlerTP(0), expect);
    ASSERT_EQ(GetBattlerTP(0), 40 + 5 + 50 / 8); // Charmander INT 50
    BattleTeardown();
}

static void Test_TpRegenScalesWithInt(void)
{
    u8 low, high;

    BattleSetup();
    gBattleMons[0].species = SPECIES_MAGIKARP;   // base Sp. Def 20
    gBattleMons[1].species = SPECIES_ALAKAZAM;   // base Sp. Def 85
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 0;
    gBattleStruct->tp[B_SIDE_OPPONENT][0] = 0;
    RegenBattlerTP(0);
    RegenBattlerTP(1);
    low = GetBattlerTP(0);
    high = GetBattlerTP(1);
    ASSERT_EQ(low, TP_REGEN_BASE + 20 / TP_REGEN_INT_DIVISOR);
    ASSERT_EQ(high, TP_REGEN_BASE + 85 / TP_REGEN_INT_DIVISOR);
    ASSERT_TRUE(high > low);
    BattleTeardown();
}

static void Test_TpRegenCap(void)
{
    BattleSetup();
    gBattleMons[0].species = SPECIES_ALAKAZAM;
    gBattleStruct->tp[B_SIDE_PLAYER][0] = TP_MAX - 1;
    RegenBattlerTP(0);
    ASSERT_EQ(GetBattlerTP(0), TP_MAX);
    RegenBattlerTP(0);
    ASSERT_EQ(GetBattlerTP(0), TP_MAX);
    BattleTeardown();
}

static void Test_TpSpend(void)
{
    BattleSetup();
    SpendBattlerTP(0, 30);
    ASSERT_EQ(GetBattlerTP(0), TP_MAX - 30);
    SpendBattlerTP(0, 0);
    ASSERT_EQ(GetBattlerTP(0), TP_MAX - 30);
    // Never wraps below zero, and spending exactly what is left empties it
    SpendBattlerTP(0, TP_MAX);
    ASSERT_EQ(GetBattlerTP(0), 0);
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 20;
    SpendBattlerTP(0, 20);
    ASSERT_EQ(GetBattlerTP(0), 0);
    BattleTeardown();
}

static void Test_TpAffordMove(void)
{
    BattleSetup();
    gBattleMons[0].moves[0] = MOVE_POUND;
    gBattleMons[0].moves[1] = MOVE_FLAMETHROWER;
    gBattleMons[0].moves[2] = MOVE_NONE;
    gBattleStruct->tp[B_SIDE_PLAYER][0] = GetMoveTPCost(MOVE_POUND);
    ASSERT_TRUE(CanBattlerAffordMoveSlot(0, 0));
    ASSERT_TRUE(!CanBattlerAffordMoveSlot(0, 1));
    gBattleStruct->tp[B_SIDE_PLAYER][0] = GetMoveTPCost(MOVE_POUND) - 1;
    ASSERT_TRUE(!CanBattlerAffordMoveSlot(0, 0));
    gBattleStruct->tp[B_SIDE_PLAYER][0] = GetMoveTPCost(MOVE_FLAMETHROWER);
    ASSERT_TRUE(CanBattlerAffordMoveSlot(0, 1));
    BattleTeardown();
}

static void Test_TpUnusableMoves(void)
{
    u8 mask;

    BattleSetup();
    gBattleMons[0].moves[0] = MOVE_POUND;
    gBattleMons[0].moves[1] = MOVE_FLAMETHROWER;
    gBattleMons[0].moves[2] = MOVE_NONE;
    gBattleMons[0].moves[3] = MOVE_NONE;
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 10;
    mask = CheckMoveLimitations(0, 0, MOVE_LIMITATIONS_ALL);
    ASSERT_EQ(mask & 1, 0);      // Pound (5) is fine
    ASSERT_EQ((mask >> 1) & 1, 1); // Flamethrower (30) is not
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 0;
    mask = CheckMoveLimitations(0, 0, MOVE_LIMITATIONS_ALL);
    ASSERT_EQ(mask, 0xF);        // nothing left: the battle uses Struggle
    BattleTeardown();
}

// --- damage ----------------------------------------------------------------------

static s32 Damage(struct BattlePokemon *atk, struct BattlePokemon *def, u16 move)
{
    gCurrentMove = move;
    return CalculateBaseDamage(atk, def, move, 0, 0, 0, 1, 0);
}

static void Test_DamageBase(void)
{
    // Lv50, Atk/Def 100, power 40: 100*40*22/100/50 = 17, plus Emerald's flat +2
    BattleSetup();
    SetBattler(&gBattleMons[1], SPECIES_RATTATA, 50, 100, 100, 100, 100);
    SetBattler(&gBattleMons[0], SPECIES_PIDGEY, 50, 100, 100, 100, 100);
    ASSERT_EQ(Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND), 19);
    BattleTeardown();
}

static void Test_DamageStages(void)
{
    BattleSetup();
    SetBattler(&gBattleMons[1], SPECIES_RATTATA, 50, 100, 100, 100, 100);
    SetBattler(&gBattleMons[0], SPECIES_PIDGEY, 50, 100, 100, 100, 100);
    gBattleMons[1].statStages[STAT_ATK] = DEFAULT_STAT_STAGE + 1; // x1.5: 150*40*22/100/50 = 26
    ASSERT_EQ(Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND), 28);
    gBattleMons[1].statStages[STAT_ATK] = DEFAULT_STAT_STAGE;
    gBattleMons[0].statStages[STAT_DEF] = DEFAULT_STAT_STAGE + 2; // x2 defense: 100*40*22/200/50 = 8
    ASSERT_EQ(Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND), 10);
    BattleTeardown();
}

static void Test_DamageBurnAndReflect(void)
{
    BattleSetup();
    SetBattler(&gBattleMons[1], SPECIES_RATTATA, 50, 100, 100, 100, 100);
    SetBattler(&gBattleMons[0], SPECIES_PIDGEY, 50, 100, 100, 100, 100);
    gBattleMons[1].status1 = STATUS1_BURN;
    ASSERT_EQ(Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND), 17 / 2 + 2);
    gBattleMons[1].status1 = 0;
    gCurrentMove = MOVE_POUND;
    ASSERT_EQ(CalculateBaseDamage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND, SIDE_STATUS_REFLECT, 0, 0, 1, 0), 17 / 2 + 2);
    BattleTeardown();
}

static void Test_DamageSpecialAndMinimum(void)
{
    BattleSetup();
    SetBattler(&gBattleMons[1], SPECIES_CHARMANDER, 50, 100, 100, 100, 100);
    SetBattler(&gBattleMons[0], SPECIES_BULBASAUR, 50, 100, 100, 100, 100);
    // Ember (power 40, Fire = special) uses Sp. Atk / Sp. Def
    gBattleMons[1].spAttack = 200;
    gBattleMons[0].spDefense = 100;
    ASSERT_EQ(Damage(&gBattleMons[1], &gBattleMons[0], MOVE_EMBER), 200 * 40 * 22 / 100 / 50 + 2);
    // A physical hit always does at least 1 (+2)
    SetBattler(&gBattleMons[1], SPECIES_RATTATA, 5, 1, 1, 1, 1);
    SetBattler(&gBattleMons[0], SPECIES_PIDGEY, 5, 1, 255, 1, 255);
    ASSERT_EQ(Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND), 3);
    BattleTeardown();
}

static void Test_DamageScalesWithLevel(void)
{
    s32 lo, hi;

    BattleSetup();
    SetBattler(&gBattleMons[1], SPECIES_RATTATA, 20, 100, 100, 100, 100);
    SetBattler(&gBattleMons[0], SPECIES_PIDGEY, 50, 100, 100, 100, 100);
    lo = Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND);
    gBattleMons[1].level = 80;
    hi = Damage(&gBattleMons[1], &gBattleMons[0], MOVE_POUND);
    ASSERT_EQ(lo, 100 * 40 * (2 * 20 / 5 + 2) / 100 / 50 + 2);
    ASSERT_EQ(hi, 100 * 40 * (2 * 80 / 5 + 2) / 100 / 50 + 2);
    BattleTeardown();
}

// --- type chart ------------------------------------------------------------------

static void Test_TypeImmunities(void)
{
    ASSERT_EQ(TypeMul(TYPE_NORMAL, TYPE_GHOST), TYPE_MUL_NO_EFFECT);
    ASSERT_EQ(TypeMul(TYPE_FIGHTING, TYPE_GHOST), TYPE_MUL_NO_EFFECT);
    ASSERT_EQ(TypeMul(TYPE_GHOST, TYPE_NORMAL), TYPE_MUL_NO_EFFECT);
    ASSERT_EQ(TypeMul(TYPE_ELECTRIC, TYPE_GROUND), TYPE_MUL_NO_EFFECT);
    ASSERT_EQ(TypeMul(TYPE_GROUND, TYPE_FLYING), TYPE_MUL_NO_EFFECT);
}

static void Test_TypeSuperEffective(void)
{
    ASSERT_EQ(TypeMul(TYPE_FIRE, TYPE_GRASS), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_WATER, TYPE_FIRE), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_GRASS, TYPE_WATER), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_ELECTRIC, TYPE_WATER), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_ELECTRIC, TYPE_FLYING), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_ICE, TYPE_DRAGON), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_FIGHTING, TYPE_NORMAL), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_PSYCHIC, TYPE_POISON), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_GROUND, TYPE_ROCK), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_ROCK, TYPE_FLYING), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_BUG, TYPE_GRASS), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_GHOST, TYPE_GHOST), TYPE_MUL_SUPER_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_DRAGON, TYPE_DRAGON), TYPE_MUL_SUPER_EFFECTIVE);
}

static void Test_TypeNotEffective(void)
{
    ASSERT_EQ(TypeMul(TYPE_FIRE, TYPE_WATER), TYPE_MUL_NOT_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_WATER, TYPE_GRASS), TYPE_MUL_NOT_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_GRASS, TYPE_FIRE), TYPE_MUL_NOT_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_NORMAL, TYPE_ROCK), TYPE_MUL_NOT_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_ELECTRIC, TYPE_GRASS), TYPE_MUL_NOT_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_PSYCHIC, TYPE_PSYCHIC), TYPE_MUL_NOT_EFFECTIVE);
    ASSERT_EQ(TypeMul(TYPE_NORMAL, TYPE_NORMAL), TYPE_MUL_NORMAL);
    ASSERT_EQ(TypeMul(TYPE_WATER, TYPE_ELECTRIC), TYPE_MUL_NORMAL);
}

static void Test_TypeTableWellFormed(void)
{
    u32 i, entries = 0;

    for (i = 0; gTypeEffectiveness[i] != TYPE_ENDTABLE; i += 3)
    {
        u8 mul = gTypeEffectiveness[i + 2];

        entries++;
        ASSERT_TRUE(entries < 120);
        if (gTypeEffectiveness[i] == TYPE_FORESIGHT)
            continue;
        ASSERT_TRUE(gTypeEffectiveness[i] < NUMBER_OF_MON_TYPES);
        ASSERT_TRUE(gTypeEffectiveness[i + 1] < NUMBER_OF_MON_TYPES);
        ASSERT_TRUE(mul == TYPE_MUL_NO_EFFECT || mul == TYPE_MUL_NOT_EFFECTIVE || mul == TYPE_MUL_SUPER_EFFECTIVE);
    }
    ASSERT_TRUE(entries > 80);
}

// --- stats, exp, species data ------------------------------------------------------

static void Test_StatCalc(void)
{
    struct Pokemon mon;

    MakeCharmander(&mon, 50);
    // base 39/52/43/65/60/50, IV 31, EV 0, neutral nature (Hardy)
    ASSERT_EQ(GetMonData(&mon, MON_DATA_MAX_HP), (2 * 39 + 31) * 50 / 100 + 50 + 10);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_MAX_HP), 114);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_ATK), (2 * 52 + 31) * 50 / 100 + 5);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_DEF), (2 * 43 + 31) * 50 / 100 + 5);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_SPEED), (2 * 65 + 31) * 50 / 100 + 5);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_SPATK), (2 * 60 + 31) * 50 / 100 + 5);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_SPDEF), (2 * 50 + 31) * 50 / 100 + 5);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_HP), 114);
}

static void Test_StatCalcEvs(void)
{
    struct Pokemon mon;
    u8 ev = 252;

    MakeCharmander(&mon, 100);
    SetMonData(&mon, MON_DATA_HP_EV, &ev);
    SetMonData(&mon, MON_DATA_ATK_EV, &ev);
    CalculateMonStats(&mon);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_MAX_HP), (2 * 39 + 31 + 252 / 4) * 100 / 100 + 100 + 10);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_ATK), (2 * 52 + 31 + 252 / 4) * 100 / 100 + 5);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_DEF), (2 * 43 + 31) * 100 / 100 + 5);
}

static void Test_StatNature(void)
{
    struct Pokemon mon;
    u32 atk, def;

    MakeCharmander(&mon, 50);
    atk = GetMonData(&mon, MON_DATA_ATK);
    def = GetMonData(&mon, MON_DATA_DEF);
    // Adamant (+Atk, -SpAtk) is nature 3
    CreateMon(&mon, SPECIES_CHARMANDER, 50, 31, TRUE, 3, OT_ID_PRESET, 0x00010002);
    ASSERT_EQ(GetNature(&mon), NATURE_ADAMANT);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_ATK), atk * 110 / 100);
    ASSERT_EQ(GetMonData(&mon, MON_DATA_DEF), def);
}

static void Test_ExpTables(void)
{
    u32 growth, level;

    for (growth = 0; growth <= GROWTH_SLOW; growth++)
    {
        ASSERT_EQ(gExperienceTables[growth][0], 0);
        for (level = 2; level <= MAX_LEVEL; level++)
        {
            if (gExperienceTables[growth][level] <= gExperienceTables[growth][level - 1])
                ASSERT_EQ(growth * 1000 + level, 0);
        }
    }
    ASSERT_EQ(gExperienceTables[GROWTH_MEDIUM_FAST][100], 1000000);
    ASSERT_EQ(gExperienceTables[GROWTH_FAST][100], 800000);
    ASSERT_EQ(gExperienceTables[GROWTH_SLOW][100], 1250000);
    ASSERT_EQ(gExperienceTables[GROWTH_MEDIUM_SLOW][100], 1059860);
    ASSERT_EQ(gExperienceTables[GROWTH_ERRATIC][100], 600000);
    ASSERT_EQ(gExperienceTables[GROWTH_FLUCTUATING][100], 1640000);
}

static void Test_ExpLevelRoundTrip(void)
{
    struct Pokemon mon;
    u32 level;
    u8 growth;

    MakeCharmander(&mon, 5);
    growth = gSpeciesInfo[SPECIES_CHARMANDER].growthRate;
    for (level = 2; level <= MAX_LEVEL; level += 7)
    {
        u32 exp = gExperienceTables[growth][level];

        SetMonData(&mon, MON_DATA_EXP, &exp);
        ASSERT_EQ(GetLevelFromMonExp(&mon), level);
        exp--;
        SetMonData(&mon, MON_DATA_EXP, &exp);
        ASSERT_EQ(GetLevelFromMonExp(&mon), level - 1);
    }
}

static void Test_ExpYieldFormula(void)
{
    // Gen 3 wild exp: baseExp * level / 7 (checked end to end by battle_exp_gain)
    ASSERT_EQ(gSpeciesInfo[SPECIES_RATTATA].expYield * 5 / 7, 57 * 5 / 7);
    ASSERT_TRUE(gSpeciesInfo[SPECIES_MEWTWO].expYield > gSpeciesInfo[SPECIES_PIDGEY].expYield);
}

static void Test_CatchRates(void)
{
    u32 s;

    ASSERT_EQ(gSpeciesInfo[SPECIES_PIDGEY].catchRate, 255);
    ASSERT_EQ(gSpeciesInfo[SPECIES_MEWTWO].catchRate, 3);
    ASSERT_EQ(gSpeciesInfo[SPECIES_MEW].catchRate, 45);
    ASSERT_EQ(gSpeciesInfo[SPECIES_SNORLAX].catchRate, 25);
    for (s = SPECIES_BULBASAUR; s <= SPECIES_MEW; s++)
    {
        if (gSpeciesInfo[s].catchRate == 0)
            ASSERT_EQ(s, 0);
    }
}

static void Test_EvolutionLevel(void)
{
    struct Pokemon mon;

    MakeCharmander(&mon, 15);
    ASSERT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, 0), SPECIES_NONE);
    MakeCharmander(&mon, 16);
    ASSERT_EQ(GetEvolutionTargetSpecies(&mon, EVO_MODE_NORMAL, 0), SPECIES_CHARMELEON);
}

// --- RNG --------------------------------------------------------------------------

static void Test_RngMulberryVectors(void)
{
    // Reference values from a Python Mulberry32 with the same constants, seed 0x1234 (runner seeds every test)
    ASSERT_EQ(gRngValue, 0x1234);
    ASSERT_EQ(Random32(), (s32)0x8B691C86);
    ASSERT_EQ(gRngValue, 0x6D2B8C29);   // state is plain Weyl: seed + 0x6D2B79F5
    ASSERT_EQ(Random(), 0xD0F1);
    ASSERT_EQ(Random(), 0x1082);
    ASSERT_EQ(Random(), 0xF13A);
    ASSERT_EQ(gRngValue, (s32)0xB4ADFA08);
}

static void Test_RngDeterministic(void)
{
    u16 a[8];
    u32 i;

    SeedRng(777);
    for (i = 0; i < 8; i++)
        a[i] = Random();
    SeedRng(777);
    for (i = 0; i < 8; i++)
        ASSERT_EQ(Random(), a[i]);
}

static void Test_RngTwoStreams(void)
{
    u16 first;

    SeedRng(5);
    SeedRng2(5);
    first = Random();
    ASSERT_EQ(Random2(), first);        // same algorithm, separate state
    Random();
    ASSERT_TRUE(gRngValue != gRng2Value);
    ASSERT_EQ(gRng2Value, 5 + 0x6D2B79F5);
}

static void Test_RngSpread(void)
{
    u32 i, ones = 0, high = 0;

    SeedRng(1);
    for (i = 0; i < 1000; i++)
    {
        u16 r = Random();

        ones += r & 1;
        high += r >> 15;
    }
    // Every output bit is usable (vanilla's low bit alternates), so both halves should be near 500
    ASSERT_RANGE(ones, 440, 560);
    ASSERT_RANGE(high, 440, 560);
}

// --- mon data, encryption --------------------------------------------------------

static void Test_BoxMonRoundTrip(void)
{
    struct BoxPokemon box;
    u8 name[POKEMON_NAME_LENGTH + 1];
    u8 iv;
    u32 v;
    u16 move = MOVE_FLAMETHROWER;

    CreateBoxMon(&box, SPECIES_CHARMANDER, 30, 31, TRUE, 0xABCD1234, OT_ID_PRESET, 0x55AA6699);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_SPECIES), SPECIES_CHARMANDER);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_PERSONALITY), (s32)0xABCD1234);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_OT_ID), 0x55AA6699);
    SetBoxMonData(&box, MON_DATA_MOVE2, &move);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_MOVE2), MOVE_FLAMETHROWER);
    iv = 17;
    SetBoxMonData(&box, MON_DATA_SPEED_IV, &iv);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_SPEED_IV), 17);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_HP_IV), 31); // neighbours untouched
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_ATK_IV), 31);
    v = gExperienceTables[gSpeciesInfo[SPECIES_CHARMANDER].growthRate][31];
    SetBoxMonData(&box, MON_DATA_EXP, &v);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_EXP), v);
    GetBoxMonData(&box, MON_DATA_NICKNAME, name);
    ASSERT_EQ(StringLength(name), 10); // "CHARMANDER"
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_SANITY_IS_BAD_EGG), 0);
}

static void Test_BoxMonEncryptedAtRest(void)
{
    struct BoxPokemon box;
    u32 key, i;
    u16 sum = 0;

    CreateBoxMon(&box, SPECIES_PIKACHU, 20, 31, TRUE, 0x0BADF00D, OT_ID_PRESET, 0x13572468);
    key = box.otId ^ box.personality;
    // Independent checksum: decrypt the 12 words by hand and add the 24 halfwords
    for (i = 0; i < 12; i++)
    {
        u32 w = box.secure.raw[i] ^ key;

        sum += (u16)w + (u16)(w >> 16);
    }
    ASSERT_EQ(box.checksum, sum);
    // The species is not readable from the raw bytes
    for (i = 0; i < 12; i++)
        if (box.secure.raw[i] == SPECIES_PIKACHU)
            ASSERT_EQ(i, 99);
}

static void Test_BoxMonCorruptionDetected(void)
{
    struct BoxPokemon box;

    CreateBoxMon(&box, SPECIES_PIKACHU, 20, 31, TRUE, 0x0BADF00D, OT_ID_PRESET, 0x13572468);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_SPECIES), SPECIES_PIKACHU);
    box.secure.raw[5] ^= 0x100;
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_SPECIES), SPECIES_EGG);
    ASSERT_EQ(GetBoxMonData(&box, MON_DATA_SANITY_IS_BAD_EGG), 1);
}

static void Test_PartyMonCopyKeepsData(void)
{
    struct Pokemon a, b;
    u16 move = MOVE_SLASH;
    u8 level;

    MakeCharmander(&a, 42);
    SetMonData(&a, MON_DATA_MOVE3, &move);
    b = a;
    ASSERT_EQ(GetMonData(&b, MON_DATA_MOVE3), MOVE_SLASH);
    ASSERT_EQ(GetMonData(&b, MON_DATA_LEVEL), 42);
    level = 43;
    SetMonData(&b, MON_DATA_LEVEL, &level);
    ASSERT_EQ(GetMonData(&a, MON_DATA_LEVEL), 42);
}

static void Test_MonGenderAndNature(void)
{
    struct Pokemon mon;

    CreateMon(&mon, SPECIES_CHARMANDER, 5, 0, TRUE, 24, OT_ID_PRESET, 1);
    ASSERT_EQ(GetNatureFromPersonality(24), 24);
    ASSERT_EQ(GetNatureFromPersonality(25), 0);
    // Nidoran F is always female, Nidoran M always male, Magnemite genderless
    CreateMon(&mon, SPECIES_NIDORAN_F, 5, 0, TRUE, 0x1234, OT_ID_PRESET, 1);
    ASSERT_EQ(GetMonGender(&mon), MON_FEMALE);
    CreateMon(&mon, SPECIES_NIDORAN_M, 5, 0, TRUE, 0x1234, OT_ID_PRESET, 1);
    ASSERT_EQ(GetMonGender(&mon), MON_MALE);
    CreateMon(&mon, SPECIES_MAGNEMITE, 5, 0, TRUE, 0x1234, OT_ID_PRESET, 1);
    ASSERT_EQ(GetMonGender(&mon), MON_GENDERLESS);
}

// --- strings ---------------------------------------------------------------------

static void Test_StringUtil(void)
{
    u8 buf[16];
    static const u8 abc[] = _("ABC");

    ASSERT_EQ(StringLength(abc), 3);
    StringCopy(buf, abc);
    ASSERT_EQ(StringLength(buf), 3);
    ASSERT_EQ(buf[0], abc[0]);
    ConvertIntToDecimalStringN(buf, 1234, STR_CONV_MODE_LEFT_ALIGN, 6);
    ASSERT_EQ(StringLength(buf), 4);
    ConvertIntToDecimalStringN(buf, 42, STR_CONV_MODE_LEADING_ZEROS, 5);
    ASSERT_EQ(StringLength(buf), 5);
    ASSERT_EQ(buf[0], 0xA1); // '0'
    ASSERT_EQ(buf[3], 0xA5); // '4'
    ASSERT_EQ(buf[4], 0xA3); // '2'
}

const struct TestCase gPureTests[] =
{
    {"tp_cost_table_in_range",       Test_TpCostTable},
    {"tp_cost_samples",              Test_TpCostSamples},
    {"tp_cost_follows_power",        Test_TpCostFollowsPower},
    {"tp_init_full",                 Test_TpInit},
    {"tp_regen_formula",             Test_TpRegenFormula},
    {"tp_regen_scales_with_int",     Test_TpRegenScalesWithInt},
    {"tp_regen_capped",              Test_TpRegenCap},
    {"tp_spend_floor_zero",          Test_TpSpend},
    {"tp_can_afford_slot",           Test_TpAffordMove},
    {"tp_unaffordable_moves_masked", Test_TpUnusableMoves},
    {"damage_base",                  Test_DamageBase},
    {"damage_stat_stages",           Test_DamageStages},
    {"damage_burn_reflect",          Test_DamageBurnAndReflect},
    {"damage_special_and_minimum",   Test_DamageSpecialAndMinimum},
    {"damage_scales_with_level",     Test_DamageScalesWithLevel},
    {"type_immunities",              Test_TypeImmunities},
    {"type_super_effective",         Test_TypeSuperEffective},
    {"type_not_effective",           Test_TypeNotEffective},
    {"type_table_well_formed",       Test_TypeTableWellFormed},
    {"stat_calc_level50",            Test_StatCalc},
    {"stat_calc_evs_level100",       Test_StatCalcEvs},
    {"stat_nature_modifier",         Test_StatNature},
    {"exp_tables_monotonic",         Test_ExpTables},
    {"exp_level_round_trip",         Test_ExpLevelRoundTrip},
    {"exp_yield_formula",            Test_ExpYieldFormula},
    {"catch_rates",                  Test_CatchRates},
    {"evolution_by_level",           Test_EvolutionLevel},
    {"rng_mulberry32_vectors",       Test_RngMulberryVectors},
    {"rng_deterministic",            Test_RngDeterministic},
    {"rng_two_streams",              Test_RngTwoStreams},
    {"rng_bit_spread",               Test_RngSpread},
    {"boxmon_get_set_round_trip",    Test_BoxMonRoundTrip},
    {"boxmon_encrypted_checksum",    Test_BoxMonEncryptedAtRest},
    {"boxmon_corruption_detected",   Test_BoxMonCorruptionDetected},
    {"mon_copy_independent",         Test_PartyMonCopyKeepsData},
    {"mon_gender_nature",            Test_MonGenderAndNature},
    {"string_util",                  Test_StringUtil},
};

const u32 gPureTestCount = ARRAY_COUNT(gPureTests);

#endif // TEST_BUILD
