/* Appended by gen.py ctrl to a copy of src/battle_controller_opponent.c, so it can reach the
   statics of that file. Turns every battler into a headless one: graphics/sound/text commands
   complete at once, the data commands (GETMONDATA, SETMONDATA, ...) stay the real ones, and the
   decision commands (action, move, item, party, exp) go to the policy code in hb_api.c.
   A foe with policy "rom" keeps the real opponent handlers, i.e. the ROM's AI scripts. */
#include "special_move.h"
#include "battle_util.h"
#include "battle_talk.h"
#include "party_menu.h"
#include "constants/party_menu.h"

extern int HbIsRom(u8 battler);
extern int HbDecideAction(u8 battler, u32 *arg);
extern int HbApplyItem(u8 battler, u32 arg, u16 *itemOut);
extern int HbDecideSwitch(u8 battler, int partyAction);
extern void HbExpConsumed(u8 monId, u32 amount);
extern void HbNoteFailed(u8 battler, int what);
extern struct Pokemon *HbPartyOf(int side);

/* The ROM passes NULL as the party order (reads BIOS memory); on the host NULL would crash */
#undef BtlController_EmitChosenMonReturnValue
void HbEmitChosen(u8 bufferId, u8 partyId, u8 *order)
{
    static u8 zero[PARTY_SIZE / 2];
    BtlController_EmitChosenMonReturnValue(bufferId, partyId, order ? order : zero);
}

static u8 sHbKind[MAX_BATTLERS_COUNT];
static u32 sHbArg[MAX_BATTLERS_COUNT];

static void HbDone(void) { OpponentBufferExecCompleted(); }

static void HbSwitchIn(void)
{
    *(gBattleStruct->monToSwitchIntoId + gActiveBattler) = PARTY_SIZE;
    gBattlerPartyIndexes[gActiveBattler] = gBattleBufferA[gActiveBattler][1];
    OpponentBufferExecCompleted();
}

static void HbChooseAction(void)
{
    u32 arg = 0;
    int kind;

    if (HbIsRom(gActiveBattler)) { OpponentHandleChooseAction(); return; }
    kind = HbDecideAction(gActiveBattler, &arg);
    sHbKind[gActiveBattler] = kind;
    sHbArg[gActiveBattler] = arg;
    BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, kind, 0);
    OpponentBufferExecCompleted();
}

static void HbChooseMove(void)
{
    u8 b = gActiveBattler;
    u8 slot, target;
    u16 move;

    if (HbIsRom(b)) { OpponentHandleChooseMove(); return; }
    slot = (sHbKind[b] == B_ACTION_USE_MOVE) ? sHbArg[b] : 0;
    move = GetBattlerMoveAtPos(b, slot);
    if (move != MOVE_NONE && (gMovesInfo[move].target & MOVE_TARGET_USER))
        target = b;
    else
        target = GetBattlerAtPosition(BATTLE_OPPOSITE(GET_BATTLER_SIDE(b)));
    HbNoteFailed(b, 0); /* counts re-asks of the same turn: a policy that picks an unusable move loops */
    BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, 10, slot | (target << 8));
    OpponentBufferExecCompleted();
}

static void HbChooseItem(void)
{
    u16 item = 0;
    u8 b = gActiveBattler;

    if (HbIsRom(b)) { OpponentHandleChooseItem(); return; }
    if (sHbKind[b] != B_ACTION_USE_ITEM || !HbApplyItem(b, sHbArg[b], &item))
    {
        item = 0; /* cancel: the engine asks for a new action */
        HbNoteFailed(b, 1);
    }
    BtlController_EmitOneReturnValue(B_COMM_TO_ENGINE, item);
    OpponentBufferExecCompleted();
}

static void HbChoosePokemon(void)
{
    u8 b = gActiveBattler;
    int i, slot, action;

    if (HbIsRom(b)) { OpponentHandleChoosePokemon(); return; }
    action = gBattleBufferA[b][1] & 0xF;
    for (i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); i++)
        gBattlePartyCurrentOrder[i] = gBattleBufferA[b][4 + i];
    if (action == PARTY_ACTION_CANT_SWITCH || action == PARTY_ACTION_ABILITY_PREVENTS)
    {
        slot = PARTY_SIZE; /* trapped: back to the action menu */
        HbNoteFailed(b, 2);
    }
    else
    {
        slot = HbDecideSwitch(b, action);
    }
    BtlController_EmitChosenMonReturnValue(B_COMM_TO_ENGINE, slot, gBattlePartyCurrentOrder);
    OpponentBufferExecCompleted();
}

/* The player controller's Task_GiveExpToMon without the exp bar */
static void HbExpUpdate(void)
{
    u8 monId = gBattleBufferA[gActiveBattler][1];
    struct Pokemon *mon;
    s16 gainedExp;
    u16 species;
    u8 level;
    u32 currExp, nextLvlExp;

    if (GetBattlerSide(gActiveBattler) != B_SIDE_PLAYER) { OpponentHandleExpUpdate(); return; }
    mon = HbPartyOf(B_SIDE_PLAYER) + monId;
    if (GetMonData(mon, MON_DATA_LEVEL) >= MAX_LEVEL) { OpponentBufferExecCompleted(); return; }
    gainedExp = T1_READ_16(&gBattleBufferA[gActiveBattler][2]);
    species = GetMonData(mon, MON_DATA_SPECIES);
    level = GetMonData(mon, MON_DATA_LEVEL);
    currExp = GetMonData(mon, MON_DATA_EXP);
    nextLvlExp = gExperienceTables[gSpeciesInfo[species].growthRate][level + 1];
    if (currExp + gainedExp >= nextLvlExp)
    {
        HbExpConsumed(monId, nextLvlExp - currExp);
        SetMonData(mon, MON_DATA_EXP, &nextLvlExp);
        CalculateMonStats(mon);
        gainedExp -= nextLvlExp - currExp;
        BtlController_EmitTwoReturnValues(B_COMM_TO_ENGINE, RET_VALUE_LEVELED_UP, gainedExp);
    }
    else
    {
        HbExpConsumed(monId, gainedExp);
        currExp += gainedExp;
        SetMonData(mon, MON_DATA_EXP, &currExp);
    }
    OpponentBufferExecCompleted();
}

void HbInstallControllers(void)
{
    s32 i;

    for (i = 0; i < CONTROLLER_CMDS_COUNT; i++)
    {
        switch (i)
        {
        case CONTROLLER_GETMONDATA: case CONTROLLER_GETRAWMONDATA: case CONTROLLER_SETMONDATA: case CONTROLLER_SETRAWMONDATA:
        case CONTROLLER_TERMINATOR_NOP: case CONTROLLER_YESNOBOX:
            break; /* the real handlers */
        case CONTROLLER_CHOOSEACTION: sOpponentBufferCommands[i] = HbChooseAction; break;
        case CONTROLLER_CHOOSEMOVE: sOpponentBufferCommands[i] = HbChooseMove; break;
        case CONTROLLER_OPENBAG: sOpponentBufferCommands[i] = HbChooseItem; break;
        case CONTROLLER_CHOOSEPOKEMON: sOpponentBufferCommands[i] = HbChoosePokemon; break;
        case CONTROLLER_EXPUPDATE: sOpponentBufferCommands[i] = HbExpUpdate; break;
        case CONTROLLER_SWITCHINANIM: sOpponentBufferCommands[i] = HbSwitchIn; break;
        default: sOpponentBufferCommands[i] = HbDone; break;
        }
    }
}
