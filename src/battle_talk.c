#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_talk.h"
#include "data.h"
#include "event_data.h"
#include "random.h"
#include "constants/battle.h"
#include "constants/trainers.h"
#include "constants/vars.h"

// Talk (R on the action menu) and the trainer states it sets.
//
// Talk doesn't use up the turn, but works once per turn. Against a trainer the
// player picks one of two lines:
//   PROVOKE: likely to work; the trainer turns Resolute (moves first, double TP).
//   UNNERVE: harder; the trainer Panics (random moves). If it fails the trainer
//            turns Resolute instead.
// The odds rise with Charisma and are lower against gym leaders, the Elite
// Four, the Champion and the rival. A state lasts 2-6 turns, longer with more
// Charisma. Everything lives in gBattleStruct (heap), so it costs no RAM.

// Chance in percent: base + Charisma / divisor, minus the boss malus, kept in 5-95
#define PROVOKE_BASE_CHANCE        60
#define PROVOKE_CHARISMA_DIV       4
#define UNNERVE_BASE_CHANCE        30
#define UNNERVE_CHARISMA_DIV       2
#define BOSS_TALK_MALUS            30
#define TALK_MIN_CHANCE            5
#define TALK_MAX_CHANCE            95

// The action menu's LCD fits two lines of about 20 characters
static const u8 sText_TalkMenu[] = _("{RIGHT_ARROW}PROVOKE{CLEAR_TO 50}fire up\n UNNERVE{CLEAR_TO 50}rattle");
static const u8 sText_TalkMenu2[] = _(" PROVOKE{CLEAR_TO 50}fire up\n{RIGHT_ARROW}UNNERVE{CLEAR_TO 50}rattle");
static const u8 sText_TalkProvoked[] = _("{B_TRAINER1_NAME} is fired up!\nMoves first, 2x TP.");
static const u8 sText_TalkIgnored[] = _("{B_TRAINER1_NAME} ignores you.\nNothing changes.");
static const u8 sText_TalkPanic[] = _("{B_TRAINER1_NAME} panics!\nRandom moves now.");
static const u8 sText_TalkBackfired[] = _("{B_TRAINER1_NAME} won't budge!\nMoves first, 2x TP.");
static const u8 sText_TalkAlready[] = _("You already talked\nthis turn.");
static const u8 sText_TalkWild[] = _("The wild {B_OPPONENT_MON1_NAME}\njust stares at you.");
static const u8 sText_TalkLink[] = _("No one answers\nover the link.");

static u32 GetCharismaForTalk(void)
{
    u32 charisma = VarGet(VAR_CHARISMA);
    return (charisma > CHARISMA_MAX) ? CHARISMA_MAX : charisma;
}

static bool32 IsBossTrainer(u16 trainerId)
{
    switch (gTrainers[trainerId].trainerClass)
    {
    case TRAINER_CLASS_LEADER:
    case TRAINER_CLASS_ELITE_FOUR:
    case TRAINER_CLASS_CHAMPION:
    case TRAINER_CLASS_RIVAL:
        return TRUE;
    }
    return FALSE;
}

static u32 GetTalkChance(u32 base, u32 charismaDiv)
{
    s32 chance = base + GetCharismaForTalk() / charismaDiv;

    if (IsBossTrainer(gTrainerBattleOpponent_A))
        chance -= BOSS_TALK_MALUS;
    if (chance < TALK_MIN_CHANCE)
        chance = TALK_MIN_CHANCE;
    if (chance > TALK_MAX_CHANCE)
        chance = TALK_MAX_CHANCE;
    return chance;
}

// The state of the trainer commanding this battler: always NONE for the
// player's side and outside trainer battles
u8 GetBattlerTrainerState(u8 battler)
{
    if (gBattleStruct == NULL
     || !(gBattleTypeFlags & BATTLE_TYPE_TRAINER)
     || GetBattlerSide(battler) != B_SIDE_OPPONENT)
        return TRAINER_STATE_NONE;
    return gBattleStruct->trainerState;
}

static void SetTrainerState(u8 state)
{
    gBattleStruct->trainerState = state;
    gBattleStruct->trainerStateTurns = TRAINER_STATE_TURNS_BASE + GetCharismaForTalk() / TRAINER_STATE_TURNS_CHARISMA_DIV;
}

// End of turn: counts the state down. TRUE when it just ran out.
bool8 TrainerStateEndTurn(void)
{
    gBattleStruct->talkedThisTurn = FALSE;
    if (gBattleStruct->trainerState == TRAINER_STATE_NONE)
        return FALSE;
    if (gBattleStruct->trainerStateTurns > 1)
    {
        gBattleStruct->trainerStateTurns--;
        return FALSE;
    }
    gBattleStruct->trainerState = TRAINER_STATE_NONE;
    gBattleStruct->trainerStateTurns = 0;
    return TRUE;
}

// What R does right now
u8 GetTalkAvailability(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        return TALK_UNAVAILABLE_LINK;
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return TALK_UNAVAILABLE_WILD;
    if (gBattleStruct->talkedThisTurn)
        return TALK_UNAVAILABLE_ALREADY;
    return TALK_AVAILABLE;
}

const u8 *GetTalkUnavailableText(u8 availability)
{
    switch (availability)
    {
    case TALK_UNAVAILABLE_LINK:
        return sText_TalkLink;
    case TALK_UNAVAILABLE_WILD:
        return sText_TalkWild;
    default:
        return sText_TalkAlready;
    }
}

const u8 *GetTalkMenuText(u8 cursor)
{
    return (cursor == TALK_PROVOKE) ? sText_TalkMenu : sText_TalkMenu2;
}

// Says the chosen line: rolls the odds, sets the trainer state and returns the
// trainer's reaction (with battle placeholders still in it)
const u8 *DoBattleTalk(u8 line)
{
    gBattleStruct->talkedThisTurn = TRUE;

    if (line == TALK_PROVOKE)
    {
        if (Random() % 100 < GetTalkChance(PROVOKE_BASE_CHANCE, PROVOKE_CHARISMA_DIV))
        {
            SetTrainerState(TRAINER_STATE_RESOLUTE);
            return sText_TalkProvoked;
        }
        return sText_TalkIgnored;
    }
    else
    {
        if (Random() % 100 < GetTalkChance(UNNERVE_BASE_CHANCE, UNNERVE_CHARISMA_DIV))
        {
            SetTrainerState(TRAINER_STATE_PANIC);
            return sText_TalkPanic;
        }
        SetTrainerState(TRAINER_STATE_RESOLUTE);
        return sText_TalkBackfired;
    }
}
