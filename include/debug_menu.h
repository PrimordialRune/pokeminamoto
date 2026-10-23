#ifndef GUARD_DEBUG_MENU_H
#define GUARD_DEBUG_MENU_H

// Debug menus, built into every ROM except `make release` (see DEBUG_MENUS in config.h).
// Overworld: hold R and press START. Battle: hold L and press SELECT on the action menu.

#ifdef DEBUG_MENUS

#define DEBUG_TOGGLE_NO_COLLISION     (1 << 0) // the player walks through walls and NPCs
#define DEBUG_TOGGLE_NO_ENCOUNTERS    (1 << 1) // no wild encounters from walking
#define DEBUG_TOGGLE_NO_TRAINER_SIGHT (1 << 2) // trainers don't spot the player

extern u8 gDebugToggles;

#define DebugToggleOn(toggle) ((gDebugToggles & (toggle)) != 0)

void Debug_ShowMainMenu(void);
void CB2_BattleDebugMenu(void);

#else

#define DebugToggleOn(toggle) FALSE

#endif // DEBUG_MENUS

#endif // GUARD_DEBUG_MENU_H
