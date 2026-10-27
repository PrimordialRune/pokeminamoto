#include "global.h"
#include "battle.h"
#include "battle_anim.h"
#include "pokemon.h"
#include "special_move.h"
#include "util.h"
#include "battle_script_commands.h"
#include "battle_util.h"
#include "battle_setup.h"
#include "data.h"
#include "constants/trainers.h"
#include "constants/moves.h"
#include "constants/species.h"

#include "data/pokemon/special_moves.h"

static const struct SpecialMoveInfo *GetMonSpecialInfo(struct Pokemon *mon)
{
    return &gSpecialMoves[GetMonData(mon, MON_DATA_SPECIES)];
}

u16 GetMonSpecialMove(struct Pokemon *mon)
{
    if (GetMonData(mon, MON_DATA_IS_EGG))
        return MOVE_NONE;
    return GetMonSpecialInfo(mon)->move;
}

// A species with no Signature yet stays sealed even if the mon's bit is set
bool8 IsMonSpecialAwakened(struct Pokemon *mon)
{
    return GetMonSpecialMove(mon) != MOVE_NONE && GetMonData(mon, MON_DATA_SPECIAL_AWAKENED);
}

// A gym leader's ace (the last mon of the party) has its Signature unlocked (Omar, 2026-10-27)
static bool8 IsLeaderAce(u8 battler)
{
    u32 last = 0, i;

    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER) || (gBattleTypeFlags & BATTLE_TYPE_LINK)
     || gTrainers[gTrainerBattleOpponent_A].trainerClass != TRAINER_CLASS_LEADER)
        return FALSE;
    for (i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gEnemyParty[i], MON_DATA_SPECIES) != SPECIES_NONE)
            last = i;
    return gBattlerPartyIndexes[battler] == last;
}

// The player's mons once awakened; on the foe side only a gym leader's ace
u16 GetBattlerSpecialMove(u8 battler)
{
    struct Pokemon *mon;

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        return MOVE_NONE;
    if (GetBattlerSide(battler) != B_SIDE_PLAYER)
        return IsLeaderAce(battler) ? GetMonSpecialMove(&gEnemyParty[gBattlerPartyIndexes[battler]]) : MOVE_NONE;
    mon = &gPlayerParty[gBattlerPartyIndexes[battler]];
    if (!IsMonSpecialAwakened(mon))
        return MOVE_NONE;
    return GetMonSpecialMove(mon);
}

static bool8 IsMonSpecialConditionMet(struct Pokemon *mon)
{
    const struct SpecialMoveInfo *info = GetMonSpecialInfo(mon);

    switch (info->condition)
    {
    case SPECIAL_COND_BOND:
        return GetMonData(mon, MON_DATA_FRIENDSHIP) >= (info->threshold ? info->threshold : SPECIAL_BOND_FRIENDSHIP);
    case SPECIAL_COND_DEED:
        return GetMonData(mon, MON_DATA_SPECIAL_PROGRESS) >= info->threshold;
    }
    return FALSE;
}

// Called when an opponent's mon faints: the player's mons on the field note their deed.
// Only party slot bits in the heap gBattleStruct, counted when the battle is won.
void SpecialMove_OnFoeFainted(u8 foeBattler)
{
    u8 battler;
    u8 foeType1 = gBattleMons[foeBattler].types[0];
    u8 foeType2 = gBattleMons[foeBattler].types[1];

    if (gBattleTypeFlags & BATTLE_TYPE_LINK)
        return;

    for (battler = 0; battler < gBattlersCount; battler++)
    {
        const struct SpecialMoveInfo *info;
        u8 partyId = gBattlerPartyIndexes[battler];

        if (GetBattlerSide(battler) != B_SIDE_PLAYER || gBattleMons[battler].hp == 0
         || (gAbsentBattlerFlags & gBitTable[battler]))
            continue;
        info = GetMonSpecialInfo(&gPlayerParty[partyId]);
        if (info->condition != SPECIAL_COND_DEED || info->deed != SPECIAL_DEED_WIN_VS_TYPES)
            continue;
        if (foeType1 == info->deedArgs[0] || foeType1 == info->deedArgs[1]
         || foeType2 == info->deedArgs[0] || foeType2 == info->deedArgs[1])
            gBattleStruct->specialDeedMons |= 1 << partyId;
    }
}

// A won battle counts once for each mon that did its deed in it
void SpecialMove_OnBattleWon(void)
{
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u8 progress;

        if (!(gBattleStruct->specialDeedMons & (1 << i)))
            continue;
        progress = GetMonData(&gPlayerParty[i], MON_DATA_SPECIAL_PROGRESS);
        if (progress < 255)
            progress++;
        SetMonData(&gPlayerParty[i], MON_DATA_SPECIAL_PROGRESS, &progress);
    }
    gBattleStruct->specialDeedMons = 0;
}

// Unseals the first party mon whose condition is met. Returns its party slot, or -1.
// It reads only saved data, so a condition met anywhere (friendship on a walk, say) is
// picked up at the end of the next won battle.
s32 SpecialMove_TryAwakenNext(void)
{
    s32 i;
    u8 awakened = TRUE;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gPlayerParty[i];

        if (GetMonData(mon, MON_DATA_SPECIES) == SPECIES_NONE || GetMonSpecialMove(mon) == MOVE_NONE
         || GetMonData(mon, MON_DATA_SPECIAL_AWAKENED))
            continue;
        if (IsMonSpecialConditionMet(mon))
        {
            SetMonData(mon, MON_DATA_SPECIAL_AWAKENED, &awakened);
            return i;
        }
    }
    return -1;
}

#ifdef DEBUG_MENUS
// Debug menu: one more deed for every party mon (a won battle still has to awaken it)
void SpecialMove_DebugAddProgress(void)
{
    s32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        u8 progress = GetMonData(&gPlayerParty[i], MON_DATA_SPECIAL_PROGRESS);

        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        if (progress < 255)
            progress++;
        SetMonData(&gPlayerParty[i], MON_DATA_SPECIAL_PROGRESS, &progress);
    }
}

void SpecialMove_DebugSealParty(void)
{
    s32 i;
    u8 zero = 0;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) == SPECIES_NONE)
            continue;
        SetMonData(&gPlayerParty[i], MON_DATA_SPECIAL_AWAKENED, &zero);
        SetMonData(&gPlayerParty[i], MON_DATA_SPECIAL_PROGRESS, &zero);
    }
}
#endif

// Trainer AI: the ace swaps the move its AI picked for its Signature when it can pay for it, the
// target isn't immune and the Signature hits at least as hard (trainer AI proper comes after the beta)
bool8 ShouldTrainerUseSpecial(u8 battler, u16 aiMove)
{
    u16 move = GetBattlerSpecialMove(battler);

    if (move == MOVE_NONE || !CanBattlerAffordMoveSlot(battler, SPECIAL_MOVE_POS))
        return FALSE;
    if (AI_TypeCalc(move, gBattleMons[gBattlerTarget].species, gBattleMons[gBattlerTarget].ability) & MOVE_RESULT_NO_EFFECT)
        return FALSE;
    return gMovesInfo[move].power >= gMovesInfo[aiMove].power;
}
