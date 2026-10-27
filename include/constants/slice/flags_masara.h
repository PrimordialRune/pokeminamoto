// Studio scaffold (world M1): event flags for workstream "masara".
// Masara Town, Satoshi's house, the Okido institute and house, Okada shop.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_FLAGS_MASARA_H
#define GUARD_CONSTANTS_SLICE_FLAGS_MASARA_H

// Story flags 0x264-0x28B (40)
#define FLAG_SLICE_MASARA_00                     0x264
#define FLAG_SLICE_MASARA_01                     0x265
#define FLAG_SLICE_MASARA_02                     0x266
#define FLAG_SLICE_MASARA_03                     0x267
#define FLAG_SLICE_MASARA_04                     0x268
#define FLAG_SLICE_MASARA_05                     0x269
#define FLAG_SLICE_MASARA_06                     0x26A
#define FLAG_SLICE_MASARA_07                     0x26B
#define FLAG_SLICE_MASARA_08                     0x26C
#define FLAG_SLICE_MASARA_09                     0x26D
#define FLAG_SLICE_MASARA_10                     0x26E
#define FLAG_SLICE_MASARA_11                     0x26F
#define FLAG_SLICE_MASARA_12                     0x270
#define FLAG_SLICE_MASARA_13                     0x271
#define FLAG_SLICE_MASARA_14                     0x272
#define FLAG_SLICE_MASARA_15                     0x273
#define FLAG_SLICE_MASARA_16                     0x274
#define FLAG_SLICE_MASARA_17                     0x275
#define FLAG_SLICE_MASARA_18                     0x276
#define FLAG_SLICE_MASARA_19                     0x277
#define FLAG_SLICE_MASARA_20                     0x278
#define FLAG_SLICE_MASARA_21                     0x279
#define FLAG_SLICE_MASARA_22                     0x27A
#define FLAG_SLICE_MASARA_23                     0x27B
#define FLAG_SLICE_MASARA_24                     0x27C
#define FLAG_SLICE_MASARA_25                     0x27D
#define FLAG_SLICE_MASARA_26                     0x27E
#define FLAG_SLICE_MASARA_27                     0x27F
#define FLAG_SLICE_MASARA_28                     0x280
#define FLAG_SLICE_MASARA_29                     0x281
#define FLAG_SLICE_MASARA_30                     0x282
#define FLAG_SLICE_MASARA_31                     0x283
#define FLAG_SLICE_MASARA_32                     0x284
#define FLAG_SLICE_MASARA_33                     0x285
#define FLAG_SLICE_MASARA_34                     0x286
#define FLAG_SLICE_MASARA_35                     0x287
#define FLAG_SLICE_MASARA_36                     0x288
#define FLAG_SLICE_MASARA_37                     0x289
#define FLAG_SLICE_MASARA_38                     0x28A
#define FLAG_SLICE_MASARA_39                     0x28B

// Item balls 0x3E8-0x3EF (dead Hoenn FLAG_ITEM_* ids; use as the object's "flag")
#define FLAG_ITEM_SLICE_MASARA_00                0x3E8
#define FLAG_ITEM_SLICE_MASARA_01                0x3E9
#define FLAG_ITEM_SLICE_MASARA_02                0x3EA
#define FLAG_ITEM_SLICE_MASARA_03                0x3EB
#define FLAG_ITEM_SLICE_MASARA_04                0x3EC
#define FLAG_ITEM_SLICE_MASARA_05                0x3ED
#define FLAG_ITEM_SLICE_MASARA_06                0x3EE
#define FLAG_ITEM_SLICE_MASARA_07                0x3EF

// Hidden items 0x1F4-0x1FB (dead Hoenn FLAG_HIDDEN_ITEM_* ids; bg_hidden_item_event "flag")
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_00         (FLAG_HIDDEN_ITEMS_START + 0x00)
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_01         (FLAG_HIDDEN_ITEMS_START + 0x38) // 0x1F5 swapped: a dead special in field_specials.c still names it
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_02         (FLAG_HIDDEN_ITEMS_START + 0x02)
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_03         (FLAG_HIDDEN_ITEMS_START + 0x03)
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_04         (FLAG_HIDDEN_ITEMS_START + 0x04)
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_05         (FLAG_HIDDEN_ITEMS_START + 0x05)
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_06         (FLAG_HIDDEN_ITEMS_START + 0x06)
#define FLAG_HIDDEN_ITEM_SLICE_MASARA_07         (FLAG_HIDDEN_ITEMS_START + 0x07)

#endif // GUARD_CONSTANTS_SLICE_FLAGS_MASARA_H
