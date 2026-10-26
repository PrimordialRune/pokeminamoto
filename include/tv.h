#ifndef GUARD_TV_H
#define GUARD_TV_H

#include "constants/tv.h"

void TurnOffTVScreen(void);
void TurnOnTVScreen(void);
u8 CheckForPlayersHouseNews(void);
void GetMomOrDadStringForTVMessage(void);
void UpdateTVScreensOnMap(int width, int height);

#endif //GUARD_TV_H
