#ifndef GUARD_TRADE_H
#define GUARD_TRADE_H

#include "constants/trade.h"

extern struct Mail gTradeMail[PARTY_SIZE];
extern u8 gSelectedTradeMonPositions[2];

extern const u16 gTradePlatform_Tilemap[];
extern const struct WindowTemplate gTradeEvolutionSceneYesNoWindowTemplate;

s32 GetGameProgressForLinkTrade(void);
void CB2_StartCreateTradeMenu(void);
void CB2_LinkTrade(void);
int CanSpinTradeMon(struct Pokemon *mon, u16 monIdx);
void InitTradeSequenceBgGpuRegs(void);
void LinkTradeDrawWindow(void);
void LoadTradeAnimGfx(void);
void DrawTextOnTradeWindow(u8 windowId, const u8 *str, u8 speed);

#endif //GUARD_TRADE_H
