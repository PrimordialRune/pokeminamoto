// Studio scaffold (world M1): event flags for workstream "north".
// Route 22 and its gate, Route 2 and its gates, Tokiwa Forest.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_FLAGS_NORTH_H
#define GUARD_CONSTANTS_SLICE_FLAGS_NORTH_H

// Story flags 0x28C-0x2A3 (24)
#define FLAG_Q_R22_DONE                          0x28C
#define FLAG_Q_R22_SKIPPED                       0x28D
#define FLAG_R22_TAG_GONE                        0x28E
#define FLAG_HIDE_R22_RIVAL                      0x28F
#define FLAG_HIDE_R22_SANDSHREW                  0x290
#define FLAG_HIDE_R22_TAG                        0x291
#define FLAG_HIDE_FOREST_KEN                     0x292
#define FLAG_Q_KEN1_WIN                          0x293
#define FLAG_FOREST_BC1_TALKED                   0x294
#define FLAG_FOREST_BC2_TALKED                   0x295
#define FLAG_R22_RIVAL_ENGAGED                   0x296
#define FLAG_SLICE_NORTH_11                      0x297
#define FLAG_SLICE_NORTH_12                      0x298
#define FLAG_SLICE_NORTH_13                      0x299
#define FLAG_SLICE_NORTH_14                      0x29A
#define FLAG_SLICE_NORTH_15                      0x29B
#define FLAG_SLICE_NORTH_16                      0x29C
#define FLAG_SLICE_NORTH_17                      0x29D
#define FLAG_SLICE_NORTH_18                      0x29E
#define FLAG_SLICE_NORTH_19                      0x29F
#define FLAG_SLICE_NORTH_20                      0x2A0
#define FLAG_SLICE_NORTH_21                      0x2A1
#define FLAG_SLICE_NORTH_22                      0x2A2
#define FLAG_SLICE_NORTH_23                      0x2A3

// Story flags 0x499-0x4A8 (16)
#define FLAG_SLICE_NORTH_24                      0x499
#define FLAG_SLICE_NORTH_25                      0x49A
#define FLAG_SLICE_NORTH_26                      0x49B
#define FLAG_SLICE_NORTH_27                      0x49C
#define FLAG_SLICE_NORTH_28                      0x49D
#define FLAG_SLICE_NORTH_29                      0x49E
#define FLAG_SLICE_NORTH_30                      0x49F
#define FLAG_SLICE_NORTH_31                      0x4A0
#define FLAG_SLICE_NORTH_32                      0x4A1
#define FLAG_SLICE_NORTH_33                      0x4A2
#define FLAG_SLICE_NORTH_34                      0x4A3
#define FLAG_SLICE_NORTH_35                      0x4A4
#define FLAG_SLICE_NORTH_36                      0x4A5
#define FLAG_SLICE_NORTH_37                      0x4A6
#define FLAG_SLICE_NORTH_38                      0x4A7
#define FLAG_SLICE_NORTH_39                      0x4A8

// Item balls 0x400-0x417 (dead Hoenn FLAG_ITEM_* ids; use as the object's "flag")
#define FLAG_ITEM_R02_MOON_STONE                 0x400
#define FLAG_ITEM_R02_HP_UP                      0x401
#define FLAG_ITEM_FOREST_ANTIDOTE                0x402
#define FLAG_ITEM_FOREST_POTION                  0x403
#define FLAG_ITEM_FOREST_POKE_BALL               0x404
#define FLAG_ITEM_SLICE_NORTH_05                 0x405
#define FLAG_ITEM_SLICE_NORTH_06                 0x406
#define FLAG_ITEM_SLICE_NORTH_07                 0x407
#define FLAG_ITEM_SLICE_NORTH_08                 0x408
#define FLAG_ITEM_SLICE_NORTH_09                 0x409
#define FLAG_ITEM_SLICE_NORTH_10                 0x40A
#define FLAG_ITEM_SLICE_NORTH_11                 0x40B
#define FLAG_ITEM_SLICE_NORTH_12                 0x40C
#define FLAG_ITEM_SLICE_NORTH_13                 0x40D
#define FLAG_ITEM_SLICE_NORTH_14                 0x40E
#define FLAG_ITEM_SLICE_NORTH_15                 0x40F
#define FLAG_ITEM_SLICE_NORTH_16                 0x410
#define FLAG_ITEM_SLICE_NORTH_17                 0x411
#define FLAG_ITEM_SLICE_NORTH_18                 0x412
#define FLAG_ITEM_SLICE_NORTH_19                 0x413
#define FLAG_ITEM_SLICE_NORTH_20                 0x414
#define FLAG_ITEM_SLICE_NORTH_21                 0x415
#define FLAG_ITEM_SLICE_NORTH_22                 0x416
#define FLAG_ITEM_SLICE_NORTH_23                 0x417

// Hidden items 0x20C-0x21B (dead Hoenn FLAG_HIDDEN_ITEM_* ids; bg_hidden_item_event "flag")
#define FLAG_HIDDEN_ITEM_R22_TAG                 (FLAG_HIDDEN_ITEMS_START + 0x18)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_01          (FLAG_HIDDEN_ITEMS_START + 0x19)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_02          (FLAG_HIDDEN_ITEMS_START + 0x1A)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_03          (FLAG_HIDDEN_ITEMS_START + 0x1B)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_04          (FLAG_HIDDEN_ITEMS_START + 0x1C)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_05          (FLAG_HIDDEN_ITEMS_START + 0x1D)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_06          (FLAG_HIDDEN_ITEMS_START + 0x1E)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_07          (FLAG_HIDDEN_ITEMS_START + 0x39) // 0x213 swapped: a dead special in field_specials.c still names it
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_08          (FLAG_HIDDEN_ITEMS_START + 0x3A) // 0x214 swapped: a dead special in field_specials.c still names it
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_09          (FLAG_HIDDEN_ITEMS_START + 0x3B) // 0x215 swapped: a dead special in field_specials.c still names it
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_10          (FLAG_HIDDEN_ITEMS_START + 0x3C) // 0x216 swapped: a dead special in field_specials.c still names it
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_11          (FLAG_HIDDEN_ITEMS_START + 0x23)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_12          (FLAG_HIDDEN_ITEMS_START + 0x24)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_13          (FLAG_HIDDEN_ITEMS_START + 0x25)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_14          (FLAG_HIDDEN_ITEMS_START + 0x26)
#define FLAG_HIDDEN_ITEM_SLICE_NORTH_15          (FLAG_HIDDEN_ITEMS_START + 0x27)

#endif // GUARD_CONSTANTS_SLICE_FLAGS_NORTH_H
