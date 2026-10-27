#include "global.h"
#include "test_runner.h"

// Boot target of a `make TEST=1` ROM (see include/test_runner.h). The pure-function tests
// run in the first frame, then the scripted battles one after the other (test_battle.c).
// Results go to gTestResults; the driver (tools/test/run_tests.py) polls `state` and reads it.

#ifdef TEST_BUILD

#include "battle.h"
#include "load_save.h"
#include "main.h"
#include "pokemon.h"
#include "random.h"
#include "constants/songs.h"

EWRAM_DATA struct TestResults gTestResults = {0};
static EWRAM_DATA u32 sCurrent = 0;
static EWRAM_DATA u32 sBattleIndex = 0;
static EWRAM_DATA bool8 sBattleRunning = FALSE;

extern const struct TestCase gCoreTests[];
extern const u32 gCoreTestCount;

static void CB2_TestRunBattles(void);
static void CB2_TestHalt(void);

void Test_Fail(u32 line, s32 got, s32 want)
{
    struct TestEntry *e = &gTestResults.entries[sCurrent];

    // Keep the first failure only
    if (e->status != TEST_FAIL)
    {
        e->status = TEST_FAIL;
        e->line = line;
        e->got = got;
        e->want = want;
    }
}

static void Finish(u32 index)
{
    struct TestEntry *e = &gTestResults.entries[index];

    if (e->status == TEST_FAIL)
        gTestResults.failed++;
    else
    {
        e->status = TEST_PASS;
        gTestResults.passed++;
    }
}

void CB2_TestRunner(void)
{
    u32 i;

    // The save block pointers are normally set while loading a save
    gSaveBlock1Ptr = &gSaveblock1;
    gSaveBlock2Ptr = &gSaveblock2;
    gSaveBlock2Ptr->playerName[0] = 0xBB; // "A"
    gSaveBlock2Ptr->playerName[1] = 0xFF;
    gSaveBlock2Ptr->optionsBattleSceneOff = TRUE;
    gSaveBlock2Ptr->optionsBattleStyle = 1;
    gSaveBlock2Ptr->optionsTextSpeed = 2;
    gTestResults.magic = TEST_MAGIC;
    gTestResults.state = TEST_STATE_PURE;

    for (i = 0; i < gPureTestCount; i++)
    {
        sCurrent = gTestResults.count++;
        gTestResults.entries[sCurrent].name = gPureTests[i].name;
        gTestResults.entries[sCurrent].kind = 0;
        SeedRng(0x1234);
        gPureTests[i].func();
        Finish(sCurrent);
    }

    // Studio slice tests (test_core.c)
    for (i = 0; i < gCoreTestCount; i++)
    {
        sCurrent = gTestResults.count++;
        gTestResults.entries[sCurrent].name = gCoreTests[i].name;
        gTestResults.entries[sCurrent].kind = 0;
        SeedRng(0x1234);
        gCoreTests[i].func();
        Finish(sCurrent);
    }

    sBattleIndex = 0;
    gTestResults.state = TEST_STATE_BATTLE;
    SetMainCallback2(CB2_TestRunBattles);
}

// Starts battle sBattleIndex, or halts when all ran. TestBattle_Report runs before this is called again.
static void CB2_TestRunBattles(void)
{
    if (sBattleRunning)
    {
        TestBattle_Check(sBattleIndex - 1);
        Finish(sCurrent);
        sBattleRunning = FALSE;
    }
    if (sBattleIndex >= TestBattle_Count())
    {
        gTestResults.state = TEST_STATE_DONE;
        SetMainCallback2(CB2_TestHalt);
        return;
    }

    sCurrent = gTestResults.count++;
    gTestResults.entries[sCurrent].name = TestBattle_Name(sBattleIndex);
    gTestResults.entries[sCurrent].kind = 1;
    SeedRng(0x1234);
    gMain.savedCallback = CB2_TestRunBattles;
    sBattleIndex++;
    sBattleRunning = TRUE;
    TestBattle_Start(sBattleIndex - 1);
}

static void CB2_TestHalt(void)
{
}

#endif // TEST_BUILD
