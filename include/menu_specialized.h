#ifndef GUARD_MENU_SPECIALIZED_H
#define GUARD_MENU_SPECIALIZED_H

#include "sprite.h"
#include "player_pc.h"
#include "list_menu.h"
#include "pokemon.h"
#include "constants/berry.h"

// Window IDs for the Player PC Mailbox
enum {
    MAILBOXWIN_TITLE,
    MAILBOXWIN_LIST,
    MAILBOXWIN_OPTIONS,
    MAILBOXWIN_COUNT
};

// Window IDs for the move relearner
enum {
    RELEARNERWIN_DESC_BATTLE,
    RELEARNERWIN_DESC_CONTEST,
    RELEARNERWIN_MOVE_LIST,
    RELEARNERWIN_MSG,
    RELEARNERWIN_YESNO,
};

// Mailbox menu
bool8 MailboxMenu_Alloc(u8 count);
u8 MailboxMenu_AddWindow(u8 windowIdx);
u8 MailboxMenu_CreateList(struct PlayerPCItemPageStruct *page);
void MailboxMenu_AddScrollArrows(struct PlayerPCItemPageStruct *page);
void MailboxMenu_Free(void);
void MailboxMenu_RemoveWindow(u8 windowIdx);

// Move relearner
void MoveRelearnerPrintMessage(u8 *str);
bool16 MoveRelearnerRunTextPrinters(void);
void MoveRelearnerCreateYesNoMenu(void);
u8 LoadMoveRelearnerMovesList(const struct ListMenuItem *items, u16 numChoices);
void InitMoveRelearnerWindows(bool8 useContestWindow);

// Level up window
void DrawLevelUpWindowPg1(u16 windowId, u16 *statsBefore, u16 *statsAfter, u8 bgClr, u8 fgClr, u8 shadowClr);
void DrawLevelUpWindowPg2(u16 windowId, u16 *currStats, u8 bgClr, u8 fgClr, u8 shadowClr);
void GetMonLevelUpWindowStats(struct Pokemon *mon, u16 *currStats);

#endif // GUARD_MENU_SPECIALIZED_H
