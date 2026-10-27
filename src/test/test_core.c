#include "global.h"
#include "test_runner.h"

// Core slice tests (studio, workstream core): the no-whiteout story battle, VAR_BATTLE_ARENA
// housekeeping, the party hand-over specials, the Trainer Card gate and the Charisma line.
// They run with the pure tests; each test is a void function using ASSERT_*.

#ifdef TEST_BUILD

#include "arena_ecology.h"
#include "battle.h"
#include "battle_setup.h"
#include "event_data.h"
#include "field_specials.h"
#include "main.h"
#include "money.h"
#include "overworld.h"
#include "pokemon.h"
#include "script_pokemon_util.h"
#include "start_menu.h"
#include "trainer_card.h"
#include "constants/battle.h"
#include "constants/battle_setup.h"
#include "constants/event_objects.h"
#include "constants/heal_locations.h"
#include "constants/opponents.h"
#include "constants/region_map_sections.h"
#include "constants/species.h"
#include "constants/tp_model.h"
#include "constants/vars.h"

extern void BattleSetup_TestEndTrainerBattle(void);
extern bool32 StartMenu_TestOffersTrainerCard(void);

#define TEST_MONEY 5000
#define TEST_TP_LEFT 30

// A two-mon party: slot 0 Charmander, slot 1 Pidgey, both fainted and with TP spent.
static void MakeFaintedParty(void)
{
    u32 i;
    u16 hp = 0;
    u8 tp = TEST_TP_LEFT;

    ZeroPlayerPartyMons();
    CreateMon(&gPlayerParty[0], SPECIES_CHARMANDER, 8, 31, TRUE, 0, OT_ID_PRESET, 0x00010002);
    CreateMon(&gPlayerParty[1], SPECIES_PIDGEY, 6, 31, TRUE, 0, OT_ID_PRESET, 0x00010003);
    CalculatePlayerPartyCount();
    for (i = 0; i < 2; i++)
    {
        SetMonData(&gPlayerParty[i], MON_DATA_HP, &hp);
        SetMonData(&gPlayerParty[i], MON_DATA_SAVED_TP, &tp);
    }
}

// Script bytes after the opcode, as BattleSetup_ConfigureTrainerBattle reads them: mode, trainer,
// local id, then four 32-bit pointers (texts and the after-battle script; all NULL here).
static void ConfigureTrainerBattle(u8 mode)
{
    static u8 sData[1 + 2 + 2 + 16];
    u32 i;

    for (i = 0; i < sizeof(sData); i++)
        sData[i] = 0;
    sData[0] = mode;
    sData[1] = TRAINER_ROXANNE_1 & 0xFF;
    sData[2] = TRAINER_ROXANNE_1 >> 8;
    sData[3] = LOCALID_NONE & 0xFF;
    sData[4] = LOCALID_NONE >> 8;
    BattleSetup_ConfigureTrainerBattle(sData);
}

// Runs the trainer-battle end callback for a lost battle and leaves the callback it picked in
// *picked; gMain.callback2 is put back so the test runner carries on.
static void EndLostTrainerBattle(u8 mode, MainCallback *picked)
{
    MainCallback saved = gMain.callback2;

    ConfigureTrainerBattle(mode);
    gBattleOutcome = B_OUTCOME_LOST;
    BattleSetup_TestEndTrainerBattle();
    *picked = gMain.callback2;
    SetMainCallback2(saved);
}

static void Test_EarlyRivalLossKeepsMoneyMapAndSpendsTp(void)
{
    MainCallback picked;
    struct WarpData locationBefore = gSaveBlock1Ptr->location;
    u16 tp;

    SetMoney(&gSaveBlock1Ptr->money, TEST_MONEY);
    MakeFaintedParty();
    EndLostTrainerBattle(TRAINER_BATTLE_EARLY_RIVAL, &picked);

    ASSERT_TRUE(picked == CB2_ReturnToFieldContinueScriptPlayMapMusic); // not CB2_WhiteOut
    ASSERT_EQ(GetMoney(&gSaveBlock1Ptr->money), TEST_MONEY);
    ASSERT_EQ(gSaveBlock1Ptr->location.mapGroup, locationBefore.mapGroup);
    ASSERT_EQ(gSaveBlock1Ptr->location.mapNum, locationBefore.mapNum);
    ASSERT_EQ(gSaveBlock1Ptr->location.x, locationBefore.x);
    // HP and status restored for the whole party ...
    ASSERT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), GetMonData(&gPlayerParty[0], MON_DATA_MAX_HP));
    ASSERT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_HP), GetMonData(&gPlayerParty[1], MON_DATA_MAX_HP));
    ASSERT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_STATUS), 0);
    // ... but TP is not: only an inn rest refills it
    tp = GetMonData(&gPlayerParty[0], MON_DATA_SAVED_TP);
    ASSERT_EQ(tp, TEST_TP_LEFT);
    ASSERT_TRUE(tp < TP_MAX);
    tp = GetMonData(&gPlayerParty[1], MON_DATA_SAVED_TP);
    ASSERT_EQ(tp, TEST_TP_LEFT);
}

static void Test_EarlyRivalResultVar(void)
{
    gBattleOutcome = B_OUTCOME_LOST;
    SetEarlyRivalBattleResult();
    ASSERT_EQ(gSpecialVar_Result, 0);
    gBattleOutcome = B_OUTCOME_DREW;
    SetEarlyRivalBattleResult();
    ASSERT_EQ(gSpecialVar_Result, 0);
    gBattleOutcome = B_OUTCOME_WON;
    SetEarlyRivalBattleResult();
    ASSERT_EQ(gSpecialVar_Result, 1);
}

static void Test_NormalTrainerLossStillWhitesOut(void)
{
    MainCallback picked;

    MakeFaintedParty();
    EndLostTrainerBattle(TRAINER_BATTLE_SINGLE, &picked);
    ASSERT_TRUE(picked == CB2_WhiteOut);
    // a whiteout heals nothing by itself: DoWhiteOut rests the party after the fade
    ASSERT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_HP), 0);
}

static void Test_ArenaClearedAfterNoWhiteoutLoss(void)
{
    MainCallback picked;

    MakeFaintedParty();
    VarSet(VAR_BATTLE_ARENA, ARENA_LUCKY_STRIKE);
    EndLostTrainerBattle(TRAINER_BATTLE_EARLY_RIVAL, &picked);
    ASSERT_EQ(VarGet(VAR_BATTLE_ARENA), ARENA_NONE);
}

static void Test_ArenaClearedAfterWin(void)
{
    MainCallback saved = gMain.callback2;

    MakeFaintedParty();
    VarSet(VAR_BATTLE_ARENA, ARENA_GRUNGE_ROCK);
    ConfigureTrainerBattle(TRAINER_BATTLE_SINGLE);
    gBattleOutcome = B_OUTCOME_WON;
    BattleSetup_TestEndTrainerBattle();
    SetMainCallback2(saved);
    ASSERT_EQ(VarGet(VAR_BATTLE_ARENA), ARENA_NONE);
}

static void Test_ArenaClearedAfterTrainerWhiteoutLoss(void)
{
    MainCallback picked;

    MakeFaintedParty();
    VarSet(VAR_BATTLE_ARENA, ARENA_LUCKY_STRIKE);
    EndLostTrainerBattle(TRAINER_BATTLE_SINGLE, &picked); // the whiteout itself runs after the fade
    ASSERT_EQ(VarGet(VAR_BATTLE_ARENA), ARENA_NONE);
}

// DoWhiteOut clears it too (a wild loss goes straight to the whiteout).
static void Test_ArenaClearedByDoWhiteOut(void)
{
    SetLastHealLocationWarp(HEAL_LOCATION_MASARA_HOME);
    MakeFaintedParty();
    VarSet(VAR_BATTLE_ARENA, ARENA_LUCKY_STRIKE);
    DoWhiteOut();
    ASSERT_EQ(VarGet(VAR_BATTLE_ARENA), ARENA_NONE);
    // and it rests the party, TP included
    ASSERT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SAVED_TP), TP_MAX);
}

// --- CheckPartyMonForHandover / RemovePartyMonAtSlot ------------------------------------------

#define MAPSEC_TEST_ROUTE_22 MAPSEC_ROUTE_22
#define MAPSEC_TEST_OTHER MAPSEC_ROUTE_1

static void MakeHandoverMon(u8 slot, u16 species, u8 level, u8 metLocation)
{
    CreateMon(&gPlayerParty[slot], species, level, 31, TRUE, 0, OT_ID_PRESET, 0x00010000 + slot);
    SetMonData(&gPlayerParty[slot], MON_DATA_MET_LOCATION, &metLocation);
}

static u16 Handover(u16 species, u16 minLevel, u16 metMapsec, u16 fullHp)
{
    gSpecialVar_0x8004 = species;
    gSpecialVar_0x8005 = minLevel;
    gSpecialVar_0x8006 = metMapsec;
    gSpecialVar_0x8007 = fullHp;
    gSpecialVar_Result = 0xFFFF;
    CheckPartyMonForHandover();
    return gSpecialVar_Result;
}

static void SetupHandoverParty(void)
{
    ZeroPlayerPartyMons();
    MakeHandoverMon(0, SPECIES_CHARMANDER, 10, MAPSEC_TEST_OTHER);
    MakeHandoverMon(1, SPECIES_SANDSHREW, 8, MAPSEC_TEST_ROUTE_22);
    MakeHandoverMon(2, SPECIES_GRAVELER, 24, MAPSEC_TEST_OTHER);
    CalculatePlayerPartyCount();
}

static void Test_HandoverOk(void)
{
    SetupHandoverParty();
    ASSERT_EQ(Handover(SPECIES_SANDSHREW, 1, MAPSEC_TEST_ROUTE_22, TRUE), HANDOVER_OK);
    ASSERT_EQ(gSpecialVar_0x8008, 1);
    // Hida's Graveler: level 20 or more, any met location, no HP requirement
    ASSERT_EQ(Handover(SPECIES_GRAVELER, 20, HANDOVER_ANY_LOCATION, FALSE), HANDOVER_OK);
    ASSERT_EQ(gSpecialVar_0x8008, 2);
}

static void Test_HandoverNoneMatching(void)
{
    SetupHandoverParty();
    ASSERT_EQ(Handover(SPECIES_PIKACHU, 1, HANDOVER_ANY_LOCATION, FALSE), HANDOVER_NONE);  // species
    ASSERT_EQ(Handover(SPECIES_GRAVELER, 25, HANDOVER_ANY_LOCATION, FALSE), HANDOVER_NONE); // level
    ASSERT_EQ(Handover(SPECIES_SANDSHREW, 1, MAPSEC_TEST_OTHER, TRUE), HANDOVER_NONE);      // met location
}

static void Test_HandoverRefusesHurtMon(void)
{
    u16 hp;

    SetupHandoverParty();
    hp = GetMonData(&gPlayerParty[1], MON_DATA_HP) - 1;
    SetMonData(&gPlayerParty[1], MON_DATA_HP, &hp);
    ASSERT_EQ(Handover(SPECIES_SANDSHREW, 1, MAPSEC_TEST_ROUTE_22, TRUE), HANDOVER_NOT_FULL_HP);
    ASSERT_EQ(gSpecialVar_0x8008, 1);
    // full HP not asked for: a hurt mon may go
    ASSERT_EQ(Handover(SPECIES_SANDSHREW, 1, MAPSEC_TEST_ROUTE_22, FALSE), HANDOVER_OK);
}

static void Test_HandoverPrefersTheHealthyMatch(void)
{
    u16 hp;

    SetupHandoverParty();
    MakeHandoverMon(3, SPECIES_SANDSHREW, 9, MAPSEC_TEST_ROUTE_22);
    CalculatePlayerPartyCount();
    hp = 1;
    SetMonData(&gPlayerParty[1], MON_DATA_HP, &hp); // first Sandshrew hurt, second whole
    ASSERT_EQ(Handover(SPECIES_SANDSHREW, 1, MAPSEC_TEST_ROUTE_22, TRUE), HANDOVER_OK);
    ASSERT_EQ(gSpecialVar_0x8008, 3);
}

static void Test_HandoverRefusesLastMember(void)
{
    ZeroPlayerPartyMons();
    MakeHandoverMon(0, SPECIES_SANDSHREW, 8, MAPSEC_TEST_ROUTE_22);
    CalculatePlayerPartyCount();
    ASSERT_EQ(Handover(SPECIES_SANDSHREW, 1, MAPSEC_TEST_ROUTE_22, TRUE), HANDOVER_LAST_MON);
    // and the remove special refuses it as well
    gSpecialVar_0x8008 = 0;
    RemovePartyMonAtSlot();
    ASSERT_EQ(gSpecialVar_Result, FALSE);
    ASSERT_EQ(CalculatePlayerPartyCount(), 1);
}

static void Test_RemovePartyMonCompacts(void)
{
    SetupHandoverParty();
    gSpecialVar_0x8008 = 1; // the Sandshrew
    RemovePartyMonAtSlot();
    ASSERT_EQ(gSpecialVar_Result, TRUE);
    ASSERT_EQ(CalculatePlayerPartyCount(), 2);
    ASSERT_EQ(GetMonData(&gPlayerParty[0], MON_DATA_SPECIES), SPECIES_CHARMANDER);
    ASSERT_EQ(GetMonData(&gPlayerParty[1], MON_DATA_SPECIES), SPECIES_GRAVELER);
    ASSERT_EQ(GetMonData(&gPlayerParty[2], MON_DATA_SPECIES), SPECIES_NONE);
}

// --- Trainer Card ----------------------------------------------------------------------------

static void Test_FlagQCardGatesTheTrainerCard(void)
{
    FlagClear(FLAG_SYS_SAFARI_MODE);
    FlagClear(FLAG_Q_CARD);
    ASSERT_EQ(IsTrainerCardUnlocked(), FALSE);
    ASSERT_EQ(StartMenu_TestOffersTrainerCard(), FALSE);
    FlagSet(FLAG_Q_CARD);
    ASSERT_EQ(IsTrainerCardUnlocked(), TRUE);
    ASSERT_EQ(StartMenu_TestOffersTrainerCard(), TRUE);
    FlagClear(FLAG_Q_CARD);
}

static void Test_CardCharismaLine(void)
{
    const u8 *noticed = GetCharismaCardWord(2);
    const u8 *known = GetCharismaCardWord(5);
    const u8 *trusted = GetCharismaCardWord(9);

    ASSERT_TRUE(GetCharismaCardWord(0) == NULL); // blank below 2
    ASSERT_TRUE(GetCharismaCardWord(1) == NULL);
    ASSERT_TRUE(noticed != NULL && known != NULL && trusted != NULL);
    ASSERT_TRUE(noticed != known && known != trusted);
    ASSERT_TRUE(GetCharismaCardWord(4) == noticed);
    ASSERT_TRUE(GetCharismaCardWord(8) == known);
    ASSERT_TRUE(GetCharismaCardWord(CHARISMA_MAX) == trusted);
}

static void Test_GetGameHourInRange(void)
{
    gSpecialVar_Result = 0xFFFF;
    GetGameHour();
    ASSERT_TRUE(gSpecialVar_Result < 24);
}

const struct TestCase gCoreTests[] =
{
    {"core_early_rival_loss_keeps_money_map_tp", Test_EarlyRivalLossKeepsMoneyMapAndSpendsTp},
    {"core_early_rival_result_var",              Test_EarlyRivalResultVar},
    {"core_normal_trainer_loss_whites_out",      Test_NormalTrainerLossStillWhitesOut},
    {"core_arena_cleared_no_whiteout_loss",      Test_ArenaClearedAfterNoWhiteoutLoss},
    {"core_arena_cleared_after_win",             Test_ArenaClearedAfterWin},
    {"core_arena_cleared_trainer_loss",          Test_ArenaClearedAfterTrainerWhiteoutLoss},
    {"core_arena_cleared_by_do_whiteout",        Test_ArenaClearedByDoWhiteOut},
    {"core_handover_ok",                         Test_HandoverOk},
    {"core_handover_none_matching",              Test_HandoverNoneMatching},
    {"core_handover_refuses_hurt_mon",           Test_HandoverRefusesHurtMon},
    {"core_handover_prefers_healthy_match",      Test_HandoverPrefersTheHealthyMatch},
    {"core_handover_refuses_last_member",        Test_HandoverRefusesLastMember},
    {"core_remove_party_mon_compacts",           Test_RemovePartyMonCompacts},
    {"core_flag_q_card_gates_trainer_card",      Test_FlagQCardGatesTheTrainerCard},
    {"core_card_charisma_line_tiers",            Test_CardCharismaLine},
    {"core_get_game_hour_in_range",              Test_GetGameHourInRange},
};

const u32 gCoreTestCount = ARRAY_COUNT(gCoreTests);

#endif // TEST_BUILD
