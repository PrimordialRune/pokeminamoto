#ifndef GUARD_BATTLE_TALK_H
#define GUARD_BATTLE_TALK_H

// Talk lines, in menu order
enum
{
    TALK_PROVOKE,
    TALK_UNNERVE,
    TALK_LINE_COUNT
};

enum
{
    TALK_AVAILABLE,
    TALK_UNAVAILABLE_LINK,
    TALK_UNAVAILABLE_WILD,
    TALK_UNAVAILABLE_ALREADY,
};

u8 GetBattlerTrainerState(u8 battler);
bool8 TrainerStateEndTurn(void);
u8 GetTalkAvailability(void);
const u8 *GetTalkUnavailableText(u8 availability);
const u8 *GetTalkMenuText(u8 cursor);
const u8 *DoBattleTalk(u8 line);

#endif // GUARD_BATTLE_TALK_H
