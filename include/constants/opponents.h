#ifndef GUARD_CONSTANTS_OPPONENTS_H
#define GUARD_CONSTANTS_OPPONENTS_H

#define TRAINER_NONE                                 0
#define TRAINER_DAISY                                1
#define TRAINER_GABBY_AND_TY_1                       2
#define TRAINER_ROXANNE_1                            3
#define TRAINER_MIGUEL_1                             4
#define TRAINER_MIGUEL_2                             5
#define TRAINER_MIGUEL_3                             6
#define TRAINER_MIGUEL_4                             7
#define TRAINER_MIGUEL_5                             8
#define TRAINER_ANDREW                               9
#define TRAINER_AMY_AND_LIV_1                       10
#define TRAINER_AMY_AND_LIV_2                       11
#define TRAINER_AMY_AND_LIV_4                       12
#define TRAINER_AMY_AND_LIV_5                       13
#define TRAINER_AMY_AND_LIV_6                       14
#define TRAINER_BRENDAN_ROUTE_103_MUDKIP            15
#define TRAINER_BRENDAN_ROUTE_103_TREECKO           16
#define TRAINER_BRENDAN_ROUTE_103_TORCHIC           17
#define TRAINER_MAY_ROUTE_103_MUDKIP                18
#define TRAINER_MAY_ROUTE_103_TREECKO               19
#define TRAINER_MAY_ROUTE_103_TORCHIC               20
#define TRAINER_MARCOS                              21
#define TRAINER_RHETT                               22
#define TRAINER_PETE                                23
#define TRAINER_ISABELLE                            24
#define TRAINER_STEVEN                              25
#define TRAINER_ANABEL                              26
#define TRAINER_TUCKER                              27
#define TRAINER_SPENSER                             28
#define TRAINER_GRETA                               29
#define TRAINER_NOLAND                              30
#define TRAINER_LUCY                                31
#define TRAINER_BRANDON                             32

// Trainer flags are TRAINER_FLAGS_START + trainer id, for ids below MAX_TRAINERS_COUNT
// (constants/flags.h); MAX_TRAINERS_COUNT also sizes the save's trainer flags, so it
// stays at Emerald's 864 and new trainers fit below it.

// Masara-to-Nibi slice (studio scaffold, world M1; slice-plan 5.3). Each block
// belongs to the workstream named above it; its party lives between the
// matching BEGIN/END lines in src/data/trainers.party. Ids never move.
// masara
#define TRAINER_SHIGERU_LAB_1                        33
#define TRAINER_SHIGERU_LAB_2                        34
#define TRAINER_SHIGERU_LAB_3                        35
// tokiwa-spine
#define TRAINER_YUJIROU                              36
#define TRAINER_CIRCUIT_TOKIWA_1                     37
#define TRAINER_CIRCUIT_TOKIWA_2                     38
#define TRAINER_SLICE_TOKIWA_SPINE_SPARE_1           39
// tokiwa-town
#define TRAINER_SLICE_TOKIWA_TOWN_SPARE_1            40
#define TRAINER_SLICE_TOKIWA_TOWN_SPARE_2            41
// north
#define TRAINER_SHIGERU_ROUTE22_1                    42
#define TRAINER_SHIGERU_ROUTE22_2                    43
#define TRAINER_SHIGERU_ROUTE22_3                    44
#define TRAINER_KEN_FOREST_1                         45
#define TRAINER_KEN_FOREST_2                         46
#define TRAINER_KEN_FOREST_3                         47
#define TRAINER_BUG_CATCHER_FOREST_1                 48
#define TRAINER_BUG_CATCHER_FOREST_2                 49
#define TRAINER_BUG_CATCHER_FOREST_3                 50
// takeshi
#define TRAINER_TAKESHI                              51
// nibi-spine
#define TRAINER_CAMPER_NIBI_PMC                      52
#define TRAINER_BUG_CATCHER_ROUTE3_1                 53
#define TRAINER_SLICE_NIBI_SPINE_SPARE_1             54
// nibi-town
#define TRAINER_SLICE_NIBI_TOWN_SPARE_1              55
#define TRAINER_SLICE_NIBI_TOWN_SPARE_2              56
// director
#define TRAINER_SLICE_DIRECTOR_RESERVE_1             57
#define TRAINER_SLICE_DIRECTOR_RESERVE_2             58
#define TRAINER_SLICE_DIRECTOR_RESERVE_3             59
#define TRAINER_SLICE_DIRECTOR_RESERVE_4             60

#define TRAINERS_COUNT                               61
#define MAX_TRAINERS_COUNT                          864

// The deleted Hoenn trainers, still named by dead systems
#include "constants/removed_trainers.h"

#endif  // GUARD_CONSTANTS_OPPONENTS_H
