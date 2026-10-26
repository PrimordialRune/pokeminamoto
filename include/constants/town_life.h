#ifndef GUARD_CONSTANTS_TOWN_LIFE_H
#define GUARD_CONSTANTS_TOWN_LIFE_H

// Town-life toolkit: Charisma tiers, inn rooms, clock windows (src/town_life.c,
// data/scripts/town_life.pory, data/scripts/inn.pory).

// checkcharisma result in VAR_RESULT
#define CHARISMA_TIER_NONE  0
#define CHARISMA_TIER_LOW   1
#define CHARISMA_TIER_MID   2
#define CHARISMA_TIER_HIGH  3

// Threshold tables for checkcharisma, one per place (sCharismaTiers in src/town_life.c)
#define CHARISMA_TIERS_TOKIWA  0 // low 2, mid 5, high 9 (tokiwa_city_v5 section 15)
#define NUM_CHARISMA_TIER_TABLES 1

// Inn rooms
#define INN_ROOM_UME    0 // free: HP, status and TP
#define INN_ROOM_TAKE   1 // as Ume, then a healing item turns up in the room after INN_TAKE_ITEM_STEPS
#define INN_ROOM_MATSU  2 // as Take, and the party's friendship rises by INN_MATSU_FRIENDSHIP

#define INN_TAKE_CHARISMA     5   // Take opens at this Charisma...
#define INN_TAKE_BADGES       2   // ...or with this many badges, whichever comes first
#define INN_TAKE_ITEM_STEPS   100
#define INN_MATSU_FRIENDSHIP  5

#endif // GUARD_CONSTANTS_TOWN_LIFE_H
