#ifndef GUARD_SPECIAL_MOVE_H
#define GUARD_SPECIAL_MOVE_H

// The Signature: each species line's own move, outside the 4 move slots. It is sealed
// ("???") until the mon meets its species' condition, then it can be used from the Signature page
// of the FIGHT menu (SELECT). Per mon it costs no new save data: MON_DATA_SPECIAL_AWAKENED and
// MON_DATA_SPECIAL_PROGRESS (docs/engineering/per-mon-data.md). Design:
// /mnt/project-files/research/special-move-research-v1.md sections 2.3-2.7.

// How a species' Signature is unsealed
enum {
    SPECIAL_COND_BOND,    // friendship >= threshold (0 means SPECIAL_BOND_FRIENDSHIP)
    SPECIAL_COND_DEED,    // the deed done threshold times, counted in MON_DATA_SPECIAL_PROGRESS
    SPECIAL_COND_PLACE,   // not wired yet
    SPECIAL_COND_WITNESS, // not wired yet
};

// What counts as one deed for SPECIAL_COND_DEED
enum {
    SPECIAL_DEED_NONE,
    SPECIAL_DEED_WIN_VS_TYPES, // win a battle where it was on the field when a foe of type deedArgs[0] or [1] fainted
};

// Friendship that unseals a species with no condition of its own
#define SPECIAL_BOND_FRIENDSHIP 220

// The move position the FIGHT menu sends when the Signature is chosen (slots are 0-3)
#define SPECIAL_MOVE_POS MAX_MON_MOVES

struct SpecialMoveInfo
{
    u16 move;       // MOVE_NONE: the species has no Signature yet and stays sealed
    u8 condition;   // SPECIAL_COND_*
    u8 threshold;   // friendship for Bond, deed count for Deed
    u8 deed;        // SPECIAL_DEED_*
    u8 deedArgs[2];
};

extern const struct SpecialMoveInfo gSpecialMoves[];

u16 GetMonSpecialMove(struct Pokemon *mon);
bool8 IsMonSpecialAwakened(struct Pokemon *mon);
u16 GetBattlerSpecialMove(u8 battler);
bool8 ShouldTrainerUseSpecial(u8 battler, u16 aiMove);
void SpecialMove_OnFoeFainted(u8 foeBattler);
void SpecialMove_OnBattleWon(void);
s32 SpecialMove_TryAwakenNext(void);
void SpecialMove_DebugAddProgress(void);
void SpecialMove_DebugSealParty(void);

#endif // GUARD_SPECIAL_MOVE_H
