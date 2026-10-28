// Studio scaffold (world M1): event flags for workstream "nibi-town".
// Nibi City streets, museum, market, houses, Nibi CVS, Aizawa house.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_FLAGS_NIBI_TOWN_H
#define GUARD_CONSTANTS_SLICE_FLAGS_NIBI_TOWN_H

// Story flags 0x4B9-0x4CC (20)
#define FLAG_NIBI_MUSEUM_TICKET                  0x4B9 // NIB-126: the Museum ticket is paid for this visit (cleared on every entry to Nibi's streets)
#define FLAG_HIDE_NIBI_FUJITA_OUT                0x4BA // NIB-401: Fujita outside the Museum, derived (08-10)
#define FLAG_HIDE_NIBI_FUJITA_IN                 0x4BB // Museum 1F: Fujita inside, derived (hidden 08-10 while she is outside)
#define FLAG_HIDE_NIBI_KEN_MUSEUM                0x4BC // NIB-123: Ken drawing the Aerodactyl, derived (DAY and not FLAG_Q_KEN_LEFT_NIBI)
#define FLAG_HIDE_NIBI_CVS_TAKESHI               0x4BD // NIB-402: Takeshi at the CVS, derived (17-19)
#define FLAG_HIDE_NIBI_CVS_SIBLING_1             0x4BE // NIB-402: first younger sibling at the CVS, derived (17-19)
#define FLAG_HIDE_NIBI_CVS_SIBLING_2             0x4BF // NIB-402: second younger sibling at the CVS, derived (17-19)
#define FLAG_HIDE_NIBI_CVS_WORKER                0x4C0 // NIB-404: the night worker at the CVS, derived (22-04)
#define FLAG_NIBI_CRATE_ASKED                    0x4C1 // S_NIB_CRATE_ASKED: someone answered about the Silph crate (K2, one witness per pass)
#define FLAG_Q_GREY_STONE                        0x4C2 // NIB-113: Takeshi's brother handed over a grey stone (a flag, not an item; PLACEHOLDER)
#define FLAG_Q_POET_A                            0x4C3 // NIB-305 step 2: the player pointed to the poet's first line
#define FLAG_Q_POET_B                            0x4C4 // NIB-305 step 2: the player pointed to the poet's second line
#define FLAG_Q_AIZAWA_DIG                        0x4C5 // NIB-306: Aizawa told the second-dig story (T3)
#define FLAG_Q_MUSEUM_NIGHT_RUMOUR               0x4C6 // NIB-307: Casa B's mother told what she saw at the Museum at night (T3)
#define FLAG_SLICE_NIBI_TOWN_14                  0x4C7
#define FLAG_SLICE_NIBI_TOWN_15                  0x4C8
#define FLAG_SLICE_NIBI_TOWN_16                  0x4C9
#define FLAG_SLICE_NIBI_TOWN_17                  0x4CA
#define FLAG_SLICE_NIBI_TOWN_18                  0x4CB
#define FLAG_SLICE_NIBI_TOWN_19                  0x4CC

// Item balls 0x420-0x427 (dead Hoenn FLAG_ITEM_* ids; use as the object's "flag")
#define FLAG_ITEM_SLICE_NIBI_TOWN_00             0x420
#define FLAG_ITEM_SLICE_NIBI_TOWN_01             0x421
#define FLAG_ITEM_SLICE_NIBI_TOWN_02             0x422
#define FLAG_ITEM_SLICE_NIBI_TOWN_03             0x423
#define FLAG_ITEM_SLICE_NIBI_TOWN_04             0x424
#define FLAG_ITEM_SLICE_NIBI_TOWN_05             0x425
#define FLAG_ITEM_SLICE_NIBI_TOWN_06             0x426
#define FLAG_ITEM_SLICE_NIBI_TOWN_07             0x427

// Hidden items 0x224-0x22B (dead Hoenn FLAG_HIDDEN_ITEM_* ids; bg_hidden_item_event "flag")
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_00      (FLAG_HIDDEN_ITEMS_START + 0x30)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_01      (FLAG_HIDDEN_ITEMS_START + 0x31)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_02      (FLAG_HIDDEN_ITEMS_START + 0x32)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_03      (FLAG_HIDDEN_ITEMS_START + 0x33)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_04      (FLAG_HIDDEN_ITEMS_START + 0x34)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_05      (FLAG_HIDDEN_ITEMS_START + 0x35)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_06      (FLAG_HIDDEN_ITEMS_START + 0x36)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_TOWN_07      (FLAG_HIDDEN_ITEMS_START + 0x37)

#endif // GUARD_CONSTANTS_SLICE_FLAGS_NIBI_TOWN_H
