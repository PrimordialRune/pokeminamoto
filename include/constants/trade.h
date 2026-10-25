#ifndef GUARD_CONSTANTS_TRADE_H
#define GUARD_CONSTANTS_TRADE_H

#define TRADE_PLAYER  0
#define TRADE_PARTNER 1

// In-game Trade IDs
#define INGAME_TRADE_SEEDOT 0
#define INGAME_TRADE_PLUSLE 1
#define INGAME_TRADE_HORSEA 2
#define INGAME_TRADE_MEOWTH 3

// Return values for CanTradeSelectedMon and CanSpinTradeMon
#define CAN_TRADE_MON              0
#define CANT_TRADE_LAST_MON        1
#define CANT_TRADE_NATIONAL        2
#define CANT_TRADE_EGG_YET         3
#define CANT_TRADE_INVALID_MON     4
#define CANT_TRADE_PARTNER_EGG_YET 5

// Return values for CheckValidityOfTradeMons
#define PLAYER_MON_INVALID   0
#define BOTH_MONS_VALID      1
#define PARTNER_MON_INVALID  2

// Return values for GetGameProgressForLinkTrade
#define TRADE_BOTH_PLAYERS_READY      0
#define TRADE_PLAYER_NOT_READY        1
#define TRADE_PARTNER_NOT_READY       2

#endif //GUARD_CONSTANTS_TRADE_H
