#ifndef GUARD_TEST_RUNNER_H
#define GUARD_TEST_RUNNER_H

// In-ROM rule tests. Only built with `make TEST=1` (src/test/); every other build
// compiles the hook below to nothing. The ROM boots straight into CB2_TestRunner,
// runs the tests in src/test/ and leaves the outcome in gTestResults (EWRAM) for
// tools/test/run_tests.py, which reads it through the emulator. Details: src/test/test_runner.c

#ifdef TEST_BUILD

#define TEST_MAGIC      0x54534554 // "TEST"
#define TEST_MAX        96

enum
{
    TEST_STATE_BOOT,
    TEST_STATE_PURE,    // pure-function tests running
    TEST_STATE_BATTLE,  // scripted battles running
    TEST_STATE_DONE,
};

enum
{
    TEST_NOT_RUN,
    TEST_PASS,
    TEST_FAIL,
};

struct TestEntry
{
    const char *name;
    u8 status;       // TEST_*
    u8 kind;         // 0 pure function, 1 scripted battle
    u16 line;        // line of the first failed assertion
    s32 got;
    s32 want;
};

// Layout read by tools/test/run_tests.py: header of 5 u32, then the entries (16 bytes each).
struct TestResults
{
    u32 magic;
    u32 state;
    u32 count;
    u32 passed;
    u32 failed;
    struct TestEntry entries[TEST_MAX];
};

extern struct TestResults gTestResults;

struct TestCase
{
    const char *name;
    void (*func)(void);
};

// Records the first failure of the running test and leaves it.
void Test_Fail(u32 line, s32 got, s32 want);

#define ASSERT_EQ(got, want) do { s32 _g = (s32)(got); s32 _w = (s32)(want); if (_g != _w) { Test_Fail(__LINE__, _g, _w); return; } } while (0)
#define ASSERT_TRUE(cond)    do { if (!(cond)) { Test_Fail(__LINE__, 0, 1); return; } } while (0)
#define ASSERT_RANGE(got, lo, hi) do { s32 _g = (s32)(got); if (_g < (s32)(lo) || _g > (s32)(hi)) { Test_Fail(__LINE__, _g, (s32)(lo)); return; } } while (0)

extern const struct TestCase gPureTests[];
extern const u32 gPureTestCount;

// Scripted battles (src/test/test_battle.c)
u32 TestBattle_Count(void);
const char *TestBattle_Name(u32 index);
void TestBattle_Start(u32 index);   // sets up and starts battle `index`; savedCallback reports back
void TestBattle_Check(u32 index);   // asserts on the outcome once the battle is over
void TestBattle_Tick(void);

#define TEST_BATTLE_TICK() TestBattle_Tick()

// The game stirs the RNG once or twice per vblank outside link and Frontier battles (vanilla
// behaviour), so a scripted wild battle's RNG stream depends on how many vblanks fell before and
// after VBlankCB_Battle was installed, which varies with the code run before it. The tests switch
// the stirring off to be repeatable.
extern bool8 gTestFreezeRngStir;
#define TEST_RNG_STIR_ALLOWED() (!gTestFreezeRngStir)

void CB2_TestRunner(void);

#else

#define TEST_BATTLE_TICK()
#define TEST_RNG_STIR_ALLOWED() TRUE

#endif // TEST_BUILD

#endif // GUARD_TEST_RUNNER_H
