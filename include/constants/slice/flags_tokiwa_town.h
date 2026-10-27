// Studio scaffold (world M1): event flags for workstream "tokiwa-town".
// Tokiwa City streets, Kurosawa house, Inoue house.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_FLAGS_TOKIWA_TOWN_H
#define GUARD_CONSTANTS_SLICE_FLAGS_TOKIWA_TOWN_H

// Story flags 0x03C-0x04F (20)
#define FLAG_HIDE_TOKIWA_HAMADA                  0x03C // hide flag: Hamada on the street (hidden before the north-block scene and at NITE)
#define FLAG_HIDE_TOKIWA_HIROSHI                 0x03D // hide flag: Hiroshi on the canal bench (shown hours 14-18, C8)
#define FLAG_HIDE_TOKIWA_YUKI                    0x03E // hide flag: Yuki and Biscotto at the canal (after Q_CARD, not at NITE unless VAR_Q_PIDGEY is 2)
#define FLAG_HIDE_INOUE_YUKI                     0x03F // hide flag: Yuki and Biscotto in the Inoue house (when they are not at the canal)
#define FLAG_HIDE_INOUE_GRANDMA                  0x040 // H_TKW_GRANDMA: Grandma Inoue, shown from the second entry after Q_CARD
#define FLAG_HIDE_KUROSAWA_PUPIL                 0x041 // hide flag: the pupil at the low table (hours 09-12, TOK-407)
#define FLAG_Q_TKW_NORTH_SEEN                    0x042 // TOK-010: the north-block scene has played
#define FLAG_Q_INOUE_VISIT1                      0x043 // TOK-113: first entry to the Inoue house after Q_CARD happened
#define FLAG_Q_FUJIWARA_NIGHT                    0x044 // TOK-307/408: heard Fujiwara at night (pays off at Otsukimi)
#define FLAG_P_HIROSHI_CANAL_PIDGEY              0x045 // TOK-104: the canal Hiroshi said "The bird. Good."
#define FLAG_P_HIROSHI_CANAL_SEASONS             0x046 // TOK-104: the canal Hiroshi said the Genbei water line (next: the seasons line)
#define FLAG_P_YUKI_NAMES                        0x047 // TOK-112: Yuki said "He is mine" (next: the names line)
#define FLAG_P_MASAKO_GUARD                      0x048 // TOK-111: toggle between the town line and the Biscotto line
#define FLAG_P_INOUE_LETTER_ASKED                0x049 // TOK-305: Grandma asked once about the undelivered letter
#define FLAG_P_INOUE_LETTER                      0x04A // TOK-125: Grandma asked whether the letter was read
#define FLAG_P_KUROSAWA_WHY                      0x04B // TOK-109: toggle between the tutor line and the pupils line
#define FLAG_P_GRANDMA_WAR                       0x04C // TOK-113: toggle between the hatago line and the war line
#define FLAG_P_HAMADA_YUJI                       0x04D // TOK-115: toggle between the case line and the Yujirou line
#define FLAG_P_KUROSAWA_HIDA                     0x04E // TOK-303: Kurosawa said the researcher line (once, on a return after FLAG_Q_HIDA_ACCEPTED)
#define FLAG_P_KUROSAWA_T3                       0x04F // TOK-109: Kurosawa said the quitting-competition line (once)

// Story flags 0x2B0-0x2BB (12)
#define FLAG_P_MASAKO_LIGHT                      0x2B0 // TOK-111: Masako said the Yujirou's-light line (second step of her rotation)
#define FLAG_SLICE_TOKIWA_TOWN_21                0x2B1
#define FLAG_SLICE_TOKIWA_TOWN_22                0x2B2
#define FLAG_SLICE_TOKIWA_TOWN_23                0x2B3
#define FLAG_SLICE_TOKIWA_TOWN_24                0x2B4
#define FLAG_SLICE_TOKIWA_TOWN_25                0x2B5
#define FLAG_SLICE_TOKIWA_TOWN_26                0x2B6
#define FLAG_SLICE_TOKIWA_TOWN_27                0x2B7
#define FLAG_SLICE_TOKIWA_TOWN_28                0x2B8
#define FLAG_SLICE_TOKIWA_TOWN_29                0x2B9
#define FLAG_SLICE_TOKIWA_TOWN_30                0x2BA
#define FLAG_SLICE_TOKIWA_TOWN_31                0x2BB

// Item balls 0x3F8-0x3FF (dead Hoenn FLAG_ITEM_* ids; use as the object's "flag")
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_00           0x3F8
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_01           0x3F9
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_02           0x3FA
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_03           0x3FB
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_04           0x3FC
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_05           0x3FD
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_06           0x3FE
#define FLAG_ITEM_SLICE_TOKIWA_TOWN_07           0x3FF

// Hidden items 0x204-0x20B (dead Hoenn FLAG_HIDDEN_ITEM_* ids; bg_hidden_item_event "flag")
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_00    (FLAG_HIDDEN_ITEMS_START + 0x10)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_01    (FLAG_HIDDEN_ITEMS_START + 0x11)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_02    (FLAG_HIDDEN_ITEMS_START + 0x12)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_03    (FLAG_HIDDEN_ITEMS_START + 0x13)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_04    (FLAG_HIDDEN_ITEMS_START + 0x14)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_05    (FLAG_HIDDEN_ITEMS_START + 0x15)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_06    (FLAG_HIDDEN_ITEMS_START + 0x16)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_TOWN_07    (FLAG_HIDDEN_ITEMS_START + 0x17)

#endif // GUARD_CONSTANTS_SLICE_FLAGS_TOKIWA_TOWN_H
