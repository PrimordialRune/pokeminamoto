// Studio scaffold (world M1): event flags for workstream "tokiwa-spine".
// Route 1, Tokiwa PMC, Ryokan Tokaido-ya, Tokiwa CVS, Yujirou's flat.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_FLAGS_TOKIWA_SPINE_H
#define GUARD_CONSTANTS_SLICE_FLAGS_TOKIWA_SPINE_H

// Story flags 0x028-0x03B (20)
#define FLAG_HIDE_TOKIWA_PMC_BARRIER            0x028 // PMC: closed-arena barrier and sign hidden (derived from FLAG_Q_YUJI_SLEPT each ON_TRANSITION)
#define FLAG_SEEN_ARENA_CLOSED                  0x029 // TOK-004: read the Closed. sign (TOK-006 needs it)
#define FLAG_P_FUMIKO_SANDSHREW                 0x02A // TOK-103: Fumiko's one-shot Omura signpost played
#define FLAG_Q_YUJI_PHOTOS                      0x02B // TOK-304: waited and heard Yujirou on the photos
#define FLAG_Q_PARCEL_GIVEN                     0x02C // TOK-002: the CVS clerk handed over Okido's parcel
#define FLAG_Q_OMURA_RESOLVED                   0x02D // TOK-108: Omura's resolution line (given/kept/fainted/missed) has played
#define FLAG_OMURA_CHECKED_OUT                  0x02E // TOK-108/309: Omura left the Tokaido-ya (set by the next rest after the resolution line)
#define FLAG_HIDE_OMURA_COMMON                  0x02F // Inn: Omura in the common room, derived (18-22, not checked out)
#define FLAG_HIDE_OMURA_CORRIDOR                0x030 // Inn: Omura in the Take corridor, derived (08-10, not checked out)
#define FLAG_HIDE_INN_CAGE                      0x031 // Inn: Omura's cage (TOK-227), derived (var 4 or 7, not checked out)
#define FLAG_HIDE_INN_HIROSHI_ENTRANCE          0x032 // Inn: Hiroshi at the entrance, derived (07-09)
#define FLAG_HIDE_INN_HIROSHI_COMMON            0x033 // Inn: Hiroshi in the common room, derived (18-21)
#define FLAG_HIDE_INN_KEN                       0x034 // Inn: Ken carrying futons, derived (TOK-105)
#define FLAG_HIDE_CVS_TANABE                    0x035 // CVS: Tanabe's shift, derived (not NITE)
#define FLAG_HIDE_CVS_KATO                      0x036 // CVS: Kato's shift, derived (NITE)
#define FLAG_HIDE_CVS_HIROSHI                   0x037 // CVS: Hiroshi buying onigiri, derived (12-14)
#define FLAG_HIDE_CVS_KIDS                      0x038 // CVS: kids counting coins, derived (14-17)
#define FLAG_HIDE_CVS_SILENT_MAN                0x039 // CVS: the silent man, derived (18-20)
#define FLAG_HIDE_CVS_KIMURA                    0x03A // CVS: Kimura's pachinko magazine, derived (22-23)
#define FLAG_HIDE_R01_RATTATA                   0x03B // Route 1: the tagged Rattata, derived (C1)

// Story flags 0x2A4-0x2AF (12)
#define FLAG_HIDE_R01_KIMURA_21                 0x2A4 // Route 1: Kimura facing north, derived (hour 21)
#define FLAG_HIDE_R01_KIMURA_23                 0x2A5 // Route 1: Kimura facing south, derived (hour 23)
#define FLAG_HIDE_YUJI_FLAT                     0x2A6 // Flat: Yujirou, derived (in the arena between the rest and B1, gone after B2)
#define FLAG_HIDE_YUJI_ARENA                    0x2A7 // PMC arena: Yujirou, derived (only from FLAG_Q_YUJI_SLEPT until B1)
#define FLAG_P_FUMIKO_B2                        0x2A8 // TOK-410: Fumiko's 'paid to the end of the month' played
#define FLAG_P_HIROSHI_PIDGEY                   0x2A9 // TOK-104: Hiroshi's 'The bird. Good.' played
#define FLAG_P_FUMIKO_TAKE                      0x2AA // TOK-309: Fumiko offered the Take room
#define FLAG_Q_TM04_OWED                        0x2AB // SYS-004: TM04 not handed over yet (full bag); Yujirou pays at the next talk
#define FLAG_SLICE_TOKIWA_SPINE_28               0x2AC
#define FLAG_SLICE_TOKIWA_SPINE_29               0x2AD
#define FLAG_SLICE_TOKIWA_SPINE_30               0x2AE
#define FLAG_SLICE_TOKIWA_SPINE_31               0x2AF

// Item balls 0x3F0-0x3F7 (dead Hoenn FLAG_ITEM_* ids; use as the object's "flag")
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_00          0x3F0
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_01          0x3F1
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_02          0x3F2
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_03          0x3F3
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_04          0x3F4
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_05          0x3F5
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_06          0x3F6
#define FLAG_ITEM_SLICE_TOKIWA_SPINE_07          0x3F7

// Hidden items 0x1FC-0x203 (dead Hoenn FLAG_HIDDEN_ITEM_* ids; bg_hidden_item_event "flag")
#define FLAG_HIDDEN_ITEM_R01_HERB               (FLAG_HIDDEN_ITEMS_START + 0x08) // R01-201 Nanami's herb picked
#define FLAG_HIDDEN_ITEM_R01_TREASURE           (FLAG_HIDDEN_ITEMS_START + 0x09) // R01-202 treasure spot 3 opened
#define FLAG_HIDDEN_ITEM_R01_TAG                (FLAG_HIDDEN_ITEMS_START + 0x0A) // R01-301 tag picked up from the grass after a knock-out
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_SPINE_03   (FLAG_HIDDEN_ITEMS_START + 0x0B)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_SPINE_04   (FLAG_HIDDEN_ITEMS_START + 0x0C)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_SPINE_05   (FLAG_HIDDEN_ITEMS_START + 0x0D)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_SPINE_06   (FLAG_HIDDEN_ITEMS_START + 0x0E)
#define FLAG_HIDDEN_ITEM_SLICE_TOKIWA_SPINE_07   (FLAG_HIDDEN_ITEMS_START + 0x0F)

#endif // GUARD_CONSTANTS_SLICE_FLAGS_TOKIWA_SPINE_H
