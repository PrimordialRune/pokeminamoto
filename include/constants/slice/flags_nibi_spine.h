// Studio scaffold (world M1): event flags for workstream "nibi-spine".
// Nibi PMC, Akagi-ya inn, Route 3.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_FLAGS_NIBI_SPINE_H
#define GUARD_CONSTANTS_SLICE_FLAGS_NIBI_SPINE_H

// Story flags 0x4A9-0x4B8 (16)
#define FLAG_HIDE_NIBI_INN_MORI                  0x4A9 // Akagi-ya: Mori at the entrance, derived (NITE only)
#define FLAG_HIDE_NIBI_INN_HIDA                  0x4AA // Akagi-ya: Hida in the common room, derived (after the first rest here, until delivered)
#define FLAG_HIDE_NIBI_INN_KEN                   0x4AB // Akagi-ya 2F: Ken in the Ume room, derived (NITE, until FLAG_Q_KEN_LEFT_NIBI)
#define FLAG_HIDE_R03_GEODUDE                    0x4AC // Route 3: the displaced Geodude, derived (after B2, until it is gone)
#define FLAG_R03_GEODUDE_GONE                    0x4AD // R03-301: the Geodude was battled or caught; it stays gone, the observations stay
#define FLAG_P_FUMIE_FIRST                       0x4AE // NIB-103: Fumie's first-entry welcome has played
#define FLAG_P_FUMIE_HIDA                        0x4AF // NIB-301: Fumie's one-shot signpost about Hida has played
#define FLAG_NIBI_INN_RESTED                     0x4B0 // Akagi-ya: the player has rested here at least once (NIB-111 appears)
#define FLAG_Q_HIDA_REFUSED                      0x4B1 // NIB-301/131: the player refused Hida (he never asks again)
#define FLAG_Q_TM34_OWED                         0x4B2 // SYS-004: TM34 not handed over yet (full bag); Takeshi pays at the next talk
#define FLAG_P_TAKESHI_GO                        0x4B3 // NIB-112: Takeshi's "Fine. Go on." has played
#define FLAG_P_TAKESHI_TUNNELS                   0x4B4 // NIB-112: Takeshi's tunnels line has played
#define FLAG_P_TAKESHI_RELAYED                   0x4B5 // NIB-502: Fumie relayed Takeshi's loss line after the whiteout (cleared at each fight)
#define FLAG_SLICE_NIBI_SPINE_13                 0x4B6
#define FLAG_SLICE_NIBI_SPINE_14                 0x4B7
#define FLAG_SLICE_NIBI_SPINE_15                 0x4B8

// Item balls 0x418-0x41F (dead Hoenn FLAG_ITEM_* ids; use as the object's "flag")
#define FLAG_ITEM_SLICE_NIBI_SPINE_00            0x418
#define FLAG_ITEM_SLICE_NIBI_SPINE_01            0x419
#define FLAG_ITEM_SLICE_NIBI_SPINE_02            0x41A
#define FLAG_ITEM_SLICE_NIBI_SPINE_03            0x41B
#define FLAG_ITEM_SLICE_NIBI_SPINE_04            0x41C
#define FLAG_ITEM_SLICE_NIBI_SPINE_05            0x41D
#define FLAG_ITEM_SLICE_NIBI_SPINE_06            0x41E
#define FLAG_ITEM_SLICE_NIBI_SPINE_07            0x41F

// Hidden items 0x21C-0x223 (dead Hoenn FLAG_HIDDEN_ITEM_* ids; bg_hidden_item_event "flag")
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_00     (FLAG_HIDDEN_ITEMS_START + 0x28)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_01     (FLAG_HIDDEN_ITEMS_START + 0x29)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_02     (FLAG_HIDDEN_ITEMS_START + 0x2A)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_03     (FLAG_HIDDEN_ITEMS_START + 0x2B)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_04     (FLAG_HIDDEN_ITEMS_START + 0x2C)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_05     (FLAG_HIDDEN_ITEMS_START + 0x2D)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_06     (FLAG_HIDDEN_ITEMS_START + 0x2E)
#define FLAG_HIDDEN_ITEM_SLICE_NIBI_SPINE_07     (FLAG_HIDDEN_ITEMS_START + 0x2F)

#endif // GUARD_CONSTANTS_SLICE_FLAGS_NIBI_SPINE_H
