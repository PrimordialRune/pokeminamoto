#include "global.h"
#include "test_runner.h"

// Scripted single battles. Each scenario creates a 1v1 wild battle with fixed mons and a fixed RNG
// seed, starts the real battle engine (CB2_InitBattle) and lets TestBattle_Tick, called from
// BattleMainCB1 every frame, press A and pick the move slot the scenario wants. The tick also logs the
// TP of both mons at the start of every turn. TestBattle_Check then asserts on the log, the winner and
// the party. TP assertions are written against the TP_* constants and the move table, so only the
// scenario data needs touching when the TP numbers change.

#ifdef TEST_BUILD

#include "arena_ecology.h"
#include "battle.h"
#include "battle_util.h"
#include "event_data.h"
#include "main.h"
#include "pokemon.h"
#include "random.h"
#include "special_move.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/species.h"
#include "constants/vars.h"

#define MAX_LOG_TURNS 12

struct TurnLog
{
    bool8 valid;
    u8 playerTp;
    u8 enemyTp;
    u16 playerHp;
    u16 enemyHp;
};

struct Scenario
{
    const char *name;
    void (*setup)(void);
    void (*tweak)(void);                       // once, on the first frame the battle structs exist
    u8 (*pick)(u32 turn, u32 frame);           // move slot to select
    void (*check)(void);
    u8 reduceEnemyAtTurn;                      // 0 = never; else the enemy drops to 1 HP when this turn starts
};

extern void CB2_InitBattle(void);

static EWRAM_DATA const struct Scenario *sScenario = NULL;
static EWRAM_DATA struct TurnLog sLog[MAX_LOG_TURNS] = {0};
static EWRAM_DATA u32 sFrame = 0;
static EWRAM_DATA bool8 sTweaked = FALSE;
static EWRAM_DATA bool8 sStruggleSeen = FALSE;
static EWRAM_DATA u8 sLastTurn = 0;
static EWRAM_DATA struct TurnLog sSnap = {0};     // taken at frame 399
static EWRAM_DATA u8 sSnapTurn = 0;
static EWRAM_DATA u32 sExpBefore = 0;
static EWRAM_DATA u16 sHpAfterRepeat[3] = {0};    // final player HP of the two seed-repeat runs
static EWRAM_DATA u32 sRngAtFirstTick[3] = {0};   // gRngValue when the battle structs first exist, per repeat run
static EWRAM_DATA u8 sRepeatRun = 0;
static EWRAM_DATA u32 sRngAtStart = 0;
EWRAM_DATA bool8 gTestFreezeRngStir = FALSE;
extern u8 gTestArenaMsgPrinted[ARENA_MSG_COUNT]; // src/arena_ecology.c: how often each first-fire message script ran
static EWRAM_DATA u8 sArenaTerrain = 0;          // last arena state seen in the battle (the struct is gone when the checks run)
static EWRAM_DATA u8 sArenaShown = 0;
static EWRAM_DATA u8 sArenaId = 0;

static void MakeMon(struct Pokemon *mon, u16 species, u8 level, const u16 *moves)
{
    u32 i;

    CreateMon(mon, species, level, 31, TRUE, 0, OT_ID_PLAYER_ID, 0);
    for (i = 0; i < MAX_MON_MOVES; i++)
    {
        SetMonData(mon, MON_DATA_MOVE1 + i, &moves[i]);
    }
}

static void SetupParties(u16 pSpecies, u8 pLevel, const u16 *pMoves, u16 eSpecies, u8 eLevel, const u16 *eMoves)
{
    ZeroPlayerPartyMons();
    ZeroEnemyPartyMons();
    MakeMon(&gPlayerParty[0], pSpecies, pLevel, pMoves);
    MakeMon(&gEnemyParty[0], eSpecies, eLevel, eMoves);
    CalculatePlayerPartyCount();
    CalculateEnemyPartyCount();
    sExpBefore = GetMonData(&gPlayerParty[0], MON_DATA_EXP);
}

static const u16 sCharizardMoves[MAX_MON_MOVES] = {MOVE_EMBER, MOVE_FLAMETHROWER, MOVE_GROWL, MOVE_SCRATCH};
static const u16 sSplashOnly[MAX_MON_MOVES] = {MOVE_SPLASH, MOVE_NONE, MOVE_NONE, MOVE_NONE};
static const u16 sTackleOnly[MAX_MON_MOVES] = {MOVE_TACKLE, MOVE_NONE, MOVE_NONE, MOVE_NONE};

// --- helpers for the checks ---------------------------------------------------

static u8 Regen(u16 species)
{
    return TP_REGEN_BASE + GetSpeciesIntelligence(species) / TP_REGEN_INT_DIVISOR;
}

static u8 AfterTurn(u8 tp, u8 cost, u16 species)
{
    u32 v = (tp > cost ? tp - cost : 0) + Regen(species);

    return v > TP_MAX ? TP_MAX : v;
}

#define CHECK_TRUE(cond) do { if (!(cond)) { Test_Fail(__LINE__, 0, 1); return; } } while (0)
#define CHECK_EQ(got, want) do { s32 _g = (s32)(got); s32 _w = (s32)(want); if (_g != _w) { Test_Fail(__LINE__, _g, _w); return; } } while (0)

// --- scenario 0: TP is spent and regenerated each turn --------------------------
// Charizard Lv50 starts with 50 TP and Growls twice, then kills a Lv60 Magikarp (that only Splashes)
// with Flamethrower. Both TP series are checked at the start of every turn.

static void Setup_TpSeries(void)
{
    SetupParties(SPECIES_CHARIZARD, 50, sCharizardMoves, SPECIES_MAGIKARP, 60, sSplashOnly);
}

static void Tweak_Tp50(void)
{
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 50;
}

static u8 Pick_GrowlTwiceThenFlamethrower(u32 turn, u32 frame)
{
    return turn < 2 ? 2 : 1;
}

static void Check_TpSeries(void)
{
    u8 p = 50, e = TP_MAX;
    u32 i;

    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_EQ(sLog[0].playerTp, p);
    CHECK_EQ(sLog[0].enemyTp, e);
    CHECK_TRUE(sLog[2].valid);
    p = AfterTurn(p, GetMoveTPCost(MOVE_GROWL), SPECIES_CHARIZARD);
    e = AfterTurn(e, GetMoveTPCost(MOVE_SPLASH), SPECIES_MAGIKARP);
    CHECK_EQ(sLog[1].playerTp, p);
    CHECK_EQ(sLog[1].enemyTp, e);
    p = AfterTurn(p, GetMoveTPCost(MOVE_GROWL), SPECIES_CHARIZARD);
    e = AfterTurn(e, GetMoveTPCost(MOVE_SPLASH), SPECIES_MAGIKARP);
    CHECK_EQ(sLog[2].playerTp, p);
    CHECK_EQ(sLog[2].enemyTp, e);
    CHECK_EQ(GetMonData(&gEnemyParty[0], MON_DATA_HP), 0); // a Flamethrower from the third turn on killed it
    // TP persists: the party keeps what the last Flamethrower left (the battle ends before the regen)
    for (i = 3; i < MAX_LOG_TURNS && sLog[i].valid; i++)
        ;
    CHECK_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SAVED_TP), sLog[i - 1].playerTp - GetMoveTPCost(MOVE_FLAMETHROWER));
}

// --- scenario 1: no affordable move means Struggle -------------------------------

static void Setup_Struggle(void)
{
    SetupParties(SPECIES_CHARIZARD, 50, sCharizardMoves, SPECIES_MAGIKARP, 40, sSplashOnly);
}

static void Tweak_ZeroTp(void)
{
    gBattleStruct->tp[B_SIDE_PLAYER][0] = 0;
}

static u8 Pick_Slot0(u32 turn, u32 frame)
{
    return 0;
}

// The checks only read the TP the first turn leaves. After it, top the player's TP up so
// Ember stays affordable (regen alone is below its cost) and the battle ends.
static u8 Pick_Slot0ThenRefill(u32 turn, u32 frame)
{
    if (turn >= 1)
        gBattleStruct->tp[B_SIDE_PLAYER][0] = TP_MAX;
    return 0;
}

static void Check_Struggle(void)
{
    CHECK_TRUE(sStruggleSeen);
    CHECK_EQ(sLog[0].playerTp, 0);
    // Struggle costs nothing, so the turn ends with just the regeneration
    CHECK_TRUE(sLog[1].valid);
    CHECK_EQ(sLog[1].playerTp, AfterTurn(0, GetMoveTPCost(MOVE_STRUGGLE), SPECIES_CHARIZARD));
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
}

// --- scenario 2: choosing a move that costs too much does nothing ---------------
// TP starts at the cost of Ember, so Flamethrower is refused. The player keeps selecting it for 399
// frames: no turn may pass and no TP may be spent. Then Ember is selected and the battle goes on.

static void Setup_StruggleTanky(void)
{
    u32 hp = 500;   // survives the first Ember

    Setup_Struggle();
    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &hp);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &hp);
    SetMonData(&gPlayerParty[0], MON_DATA_MAX_HP, &hp);   // the player outlasts it too: the battle is lost to nobody
    SetMonData(&gPlayerParty[0], MON_DATA_HP, &hp);
}

static void Tweak_EmberTp(void)
{
    gBattleStruct->tp[B_SIDE_PLAYER][0] = GetMoveTPCost(MOVE_EMBER);
}

static u8 Pick_BlockedThenAffordable(u32 turn, u32 frame)
{
    if (frame >= 400)
        return Pick_Slot0ThenRefill(turn, frame);
    return 1;
}

static void Check_Blocked(void)
{
    CHECK_TRUE(GetMoveTPCost(MOVE_FLAMETHROWER) > GetMoveTPCost(MOVE_EMBER));
    CHECK_EQ(sSnapTurn, 0);
    CHECK_EQ(sSnap.playerTp, GetMoveTPCost(MOVE_EMBER));
    CHECK_EQ(sSnap.enemyTp, TP_MAX);
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_TRUE(sLog[1].valid);          // the turn went ahead once an affordable move was chosen
    CHECK_EQ(sLog[1].playerTp, AfterTurn(GetMoveTPCost(MOVE_EMBER), GetMoveTPCost(MOVE_EMBER), SPECIES_CHARIZARD));
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
}

// --- scenario 3: exp gain of a won wild battle -----------------------------------

static void Setup_Exp(void)
{
    SetupParties(SPECIES_CHARIZARD, 50, sCharizardMoves, SPECIES_RATTATA, 5, sTackleOnly);
}

static void Check_Exp(void)
{
    u32 gained = GetMonData(&gPlayerParty[0], MON_DATA_EXP) - sExpBefore;

    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_EQ(GetMonData(&gEnemyParty[0], MON_DATA_HP), 0);
    CHECK_EQ(gained, gSpeciesInfo[SPECIES_RATTATA].expYield * 5 / 7);
    CHECK_EQ(GetMonData(&gPlayerParty[0], MON_DATA_LEVEL), 50);
    CHECK_TRUE(GetMonData(&gPlayerParty[0], MON_DATA_HP) > 0);
}

// --- scenarios 4 and 5: the same battle twice under the same seed ----------------

static void Setup_Repeat(void)
{
    SetupParties(SPECIES_CHARMANDER, 12, sCharizardMoves, SPECIES_PIDGEY, 12, sTackleOnly);
}

static void Check_Repeat(void)
{
    sHpAfterRepeat[sRepeatRun] = GetMonData(&gPlayerParty[0], MON_DATA_HP);
    sRngAtFirstTick[sRepeatRun] = sRngAtStart;
    sRepeatRun++;
    if (sRepeatRun >= 2)
    {
        // Same seed, same parties, same inputs: the engine must reach the same RNG state and the same end,
        // whatever battle ran before (the earlier scenarios in the table differ from these)
        CHECK_EQ(sRngAtFirstTick[sRepeatRun - 1], sRngAtFirstTick[0]);
        CHECK_EQ(sHpAfterRepeat[sRepeatRun - 1], sHpAfterRepeat[0]);
    }
    CHECK_TRUE(gBattleOutcome == B_OUTCOME_WON || gBattleOutcome == B_OUTCOME_LOST);
}

// --- scenarios 6-9: the Special (include/special_move.h) ----------------------------
// Charmander's Special, FLAME CLAWS, awakens after 3 won battles in which a Rock or Water foe fainted
// while it was on the field. Progress starts at 2, so one such win awakens it.

static const u16 sCharmanderMoves[MAX_MON_MOVES] = {MOVE_EMBER, MOVE_SCRATCH, MOVE_GROWL, MOVE_NONE};

static void SetSpecial(u8 progress, u8 awakened)
{
    SetMonData(&gPlayerParty[0], MON_DATA_SPECIAL_PROGRESS, &progress);
    SetMonData(&gPlayerParty[0], MON_DATA_SPECIAL_AWAKENED, &awakened);
}

static void Setup_SpecialDeed(void)
{
    SetupParties(SPECIES_CHARMANDER, 20, sCharmanderMoves, SPECIES_GEODUDE, 5, sTackleOnly);
    SetSpecial(2, FALSE);
}

static void Check_SpecialDeed(void)
{
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIAL_PROGRESS), 3);
    CHECK_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIAL_AWAKENED), TRUE);
    CHECK_TRUE(IsMonSpecialAwakened(&gPlayerParty[0]));
    CHECK_EQ(GetMonSpecialMove(&gPlayerParty[0]), MOVE_FLAME_CLAWS);
}

// A win against a foe of another type is no deed
static void Setup_SpecialNoDeed(void)
{
    SetupParties(SPECIES_CHARMANDER, 20, sCharmanderMoves, SPECIES_PIDGEY, 5, sTackleOnly);
    SetSpecial(2, FALSE);
}

static void Check_SpecialNoDeed(void)
{
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIAL_PROGRESS), 2);
    CHECK_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIAL_AWAKENED), FALSE);
}

// An awakened Special is chosen from the Special page (SPECIAL_MOVE_POS) and spends its own TP cost
static void Setup_SpecialUse(void)
{
    u32 hp = 500;   // survives FLAME CLAWS

    SetupParties(SPECIES_CHARMANDER, 20, sCharmanderMoves, SPECIES_MAGIKARP, 40, sSplashOnly);
    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &hp);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &hp);
    SetSpecial(3, TRUE);
}

static u8 Pick_SpecialThenSlot0(u32 turn, u32 frame)
{
    return turn == 0 ? SPECIAL_MOVE_POS : 0;
}

static void Check_SpecialUse(void)
{
    CHECK_TRUE(sLog[1].valid);
    CHECK_TRUE(GetMoveTPCost(MOVE_FLAME_CLAWS) >= 40);
    CHECK_EQ(sLog[1].playerTp, AfterTurn(sLog[0].playerTp, GetMoveTPCost(MOVE_FLAME_CLAWS), SPECIES_CHARMANDER));
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
}

// A sealed Special can't be chosen: 399 frames of picking it pass no turn and spend no TP
static void Setup_SpecialSealed(void)
{
    Setup_SpecialUse();
    SetSpecial(0, FALSE);
}

static u8 Pick_SpecialThenSlot0ByFrame(u32 turn, u32 frame)
{
    return frame < 400 ? SPECIAL_MOVE_POS : 0;
}

static void Check_SpecialSealed(void)
{
    CHECK_EQ(sSnapTurn, 0);
    CHECK_EQ(sSnap.playerTp, sLog[0].playerTp);
    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
}

// --- scenarios 10 and 11: arena first-fire messages (src/arena_ecology.c) ---------------
// Each rule says its line once per battle, however often it fires. PLACEHOLDER numbers: only the once-per-battle
// behaviour and the order of events are asserted, not the balance.

#define MSG_BIT(m) (1 << (m))

static void SetArenaScenario(u8 arena)
{
    u32 i;

    VarSet(VAR_BATTLE_ARENA, arena);
    for (i = 0; i < ARENA_MSG_COUNT; i++)
        gTestArenaMsgPrinted[i] = 0;
    sArenaId = sArenaTerrain = sArenaShown = 0;
}

static void TankyMons(void)
{
    u32 hp = 3000;   // outlasts 20 turns; the scenario knocks it to 1 HP at turn 20

    SetMonData(&gEnemyParty[0], MON_DATA_MAX_HP, &hp);
    SetMonData(&gEnemyParty[0], MON_DATA_HP, &hp);
    SetMonData(&gPlayerParty[0], MON_DATA_MAX_HP, &hp);   // the player outlasts it too: the battle is lost to nobody
    SetMonData(&gPlayerParty[0], MON_DATA_HP, &hp);
}

// Fault Line, 20 turns: Water Gun (mud), Gust (dries it), Gust (dust storm), then Tackles until the floor splits.
static const u16 sBlastoiseFaultMoves[MAX_MON_MOVES] = {MOVE_WATER_GUN, MOVE_GUST, MOVE_TACKLE, MOVE_NONE};

static void Setup_FaultLine20(void)
{
    SetupParties(SPECIES_BLASTOISE, 60, sBlastoiseFaultMoves, SPECIES_ONIX, 50, sTackleOnly);
    TankyMons();
    SetArenaScenario(ARENA_FAULT_LINE);
}

static u8 Pick_FaultLine20(u32 turn, u32 frame)
{
    gBattleStruct->tp[B_SIDE_PLAYER][0] = TP_MAX;
    gBattleStruct->tp[B_SIDE_OPPONENT][0] = TP_MAX;
    if (turn == 0)
        return 0;       // Water Gun: the floor turns to mud
    if (turn <= 2)
        return 1;       // Gust: dries the mud, then raises dust
    return 2;           // Tackle
}

static void Check_FaultLine20(void)
{
    u32 i;

    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_TRUE(sLastTurn >= 20);                       // 20 turns of the Fault Line, no crash
    CHECK_EQ(sArenaId, ARENA_FAULT_LINE);
    CHECK_EQ(sArenaTerrain, ARENA_TERRAIN_FRACTURED);  // the blows landed and the floor never heals
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_GROUND_CRACKS], 1);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_FLOOR_MUD], 1);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_FLOOR_FRACTURED], 1);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_DUST_STORM], 1);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_VENTS_ROAR], 0);     // Lucky Strike lines stay out of the Fault Line
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_VENTS_CLOGGED], 0);
    CHECK_EQ(sArenaShown, MSG_BIT(ARENA_MSG_GROUND_CRACKS) | MSG_BIT(ARENA_MSG_FLOOR_MUD)
                        | MSG_BIT(ARENA_MSG_FLOOR_FRACTURED) | MSG_BIT(ARENA_MSG_DUST_STORM));
    for (i = 0; i < ARENA_MSG_COUNT; i++)
        CHECK_TRUE(gTestArenaMsgPrinted[i] <= 1);
    VarSet(VAR_BATTLE_ARENA, ARENA_NONE);
}

// Lucky Strike: the vents roar for the first bird move, String Shot clogs them; each says its line once.
static const u16 sPidgeotMoves[MAX_MON_MOVES] = {MOVE_TACKLE, MOVE_STRING_SHOT, MOVE_NONE, MOVE_NONE};

static void Setup_LuckyStrikeVents(void)
{
    SetupParties(SPECIES_PIDGEOT, 50, sPidgeotMoves, SPECIES_ONIX, 40, sTackleOnly);
    TankyMons();
    SetArenaScenario(ARENA_LUCKY_STRIKE);
}

static u8 Pick_LuckyStrikeVents(u32 turn, u32 frame)
{
    gBattleStruct->tp[B_SIDE_PLAYER][0] = TP_MAX;
    gBattleStruct->tp[B_SIDE_OPPONENT][0] = TP_MAX;
    return turn == 1 ? 1 : 0;   // String Shot on the second turn
}

static void Check_LuckyStrikeVents(void)
{
    u32 i;

    CHECK_EQ(gBattleOutcome, B_OUTCOME_WON);
    CHECK_TRUE(sLastTurn >= 5);
    CHECK_EQ(sArenaId, ARENA_LUCKY_STRIKE);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_VENTS_ROAR], 1);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_VENTS_CLOGGED], 1);
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_GROUND_CRACKS], 0);   // no Fault Line line in the vents
    CHECK_EQ(gTestArenaMsgPrinted[ARENA_MSG_FLOOR_MUD], 0);
    CHECK_EQ(sArenaTerrain, ARENA_TERRAIN_INTACT);
    for (i = 0; i < ARENA_MSG_COUNT; i++)
        CHECK_TRUE(gTestArenaMsgPrinted[i] <= 1);
    VarSet(VAR_BATTLE_ARENA, ARENA_NONE);
}

// --- table -------------------------------------------------------------------

// The repeat runs come after the other scenarios on purpose: they must not depend on what ran before.
static const struct Scenario sScenarios[] =
{
    {"battle_tp_spend_and_regen",   Setup_TpSeries, Tweak_Tp50,   Pick_GrowlTwiceThenFlamethrower, Check_TpSeries},
    {"battle_struggle_at_zero_tp",  Setup_Struggle, Tweak_ZeroTp, Pick_Slot0ThenRefill,         Check_Struggle},
    {"battle_unaffordable_blocked", Setup_StruggleTanky, Tweak_EmberTp, Pick_BlockedThenAffordable,  Check_Blocked, 2},
    {"battle_exp_gain",             Setup_Exp,      NULL,         Pick_Slot0,                   Check_Exp},
    {"battle_special_deed_awakens", Setup_SpecialDeed, NULL,      Pick_Slot0,                   Check_SpecialDeed, 1},
    {"battle_special_other_type",   Setup_SpecialNoDeed, NULL,    Pick_Slot0,                   Check_SpecialNoDeed, 1},
    {"battle_special_used",         Setup_SpecialUse, NULL,       Pick_SpecialThenSlot0,        Check_SpecialUse, 2},
    {"battle_special_sealed",       Setup_SpecialSealed, NULL,    Pick_SpecialThenSlot0ByFrame, Check_SpecialSealed, 2},
    {"battle_arena_fault_line_20_turns", Setup_FaultLine20, NULL, Pick_FaultLine20,             Check_FaultLine20, 20},
    {"battle_arena_lucky_strike_vents",  Setup_LuckyStrikeVents, NULL, Pick_LuckyStrikeVents,   Check_LuckyStrikeVents, 5},
    {"battle_seeded_run_a",         Setup_Repeat,   NULL,         Pick_Slot0,                   Check_Repeat},
    {"battle_seeded_run_b_same_hp", Setup_Repeat,   NULL,         Pick_Slot0,                   Check_Repeat},
    {"battle_seeded_run_c_same_hp", Setup_Repeat,   NULL,         Pick_Slot0,                   Check_Repeat},
};

u32 TestBattle_Count(void)
{
    return ARRAY_COUNT(sScenarios);
}

const char *TestBattle_Name(u32 index)
{
    return sScenarios[index].name;
}

void TestBattle_Start(u32 index)
{
    u32 i;

    sScenario = &sScenarios[index];
    for (i = 0; i < MAX_LOG_TURNS; i++)
        sLog[i].valid = FALSE;
    sFrame = 0;
    sTweaked = FALSE;
    sStruggleSeen = FALSE;
    sLastTurn = 0;
    sSnapTurn = 0;
    sSnap.valid = FALSE;
    gBattleTypeFlags = 0;
    gBattleOutcome = 0;
    gBattleResults.battleTurnCounter = 0;
    gTestFreezeRngStir = TRUE;
    sScenario->setup();
    SetMainCallback2(CB2_InitBattle);
}

void TestBattle_Check(u32 index)
{
    sScenarios[index].check();
}

// Called every frame from BattleMainCB1, before the controllers run
void TestBattle_Tick(void)
{
    u8 turn = gBattleResults.battleTurnCounter;

    sFrame++;
    if (sScenario == NULL)
        return;

    if (gBattleResources != NULL && gBattleStruct != NULL && gBattlersCount == 2)
    {
        if (!sTweaked)
        {
            sTweaked = TRUE;
            sRngAtStart = gRngValue;
            if (sScenario->tweak != NULL)
                sScenario->tweak();
        }
        // Only from above: a KO in that turn must stay a KO
        if (sScenario->reduceEnemyAtTurn != 0 && turn == sScenario->reduceEnemyAtTurn && gBattleMons[1].hp > 1)
            gBattleMons[1].hp = 1;
        if (turn < MAX_LOG_TURNS && !sLog[turn].valid)
        {
            sLog[turn].valid = TRUE;
            sLog[turn].playerTp = gBattleStruct->tp[B_SIDE_PLAYER][0];
            sLog[turn].enemyTp = gBattleStruct->tp[B_SIDE_OPPONENT][0];
        }
        if (gCurrentMove == MOVE_STRUGGLE)
            sStruggleSeen = TRUE;
        sLastTurn = turn;
        sArenaId = gBattleStruct->arenaId;
        sArenaTerrain = gBattleStruct->arenaTerrain;
        sArenaShown = gBattleStruct->arenaMsgShown;
        if (sFrame == 399)
        {
            sSnap.valid = TRUE;
            sSnap.playerTp = gBattleStruct->tp[B_SIDE_PLAYER][0];
            sSnap.enemyTp = gBattleStruct->tp[B_SIDE_OPPONENT][0];
            sSnapTurn = turn;
        }
    }

    gMoveSelectionCursor[0] = sScenario->pick(turn, sFrame);
    if ((sFrame & 3) == 0)
    {
        gMain.newKeys |= A_BUTTON;
        gMain.newAndRepeatedKeys |= A_BUTTON;
        gMain.heldKeys |= A_BUTTON;
    }
}

#endif // TEST_BUILD
