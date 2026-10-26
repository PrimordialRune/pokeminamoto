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

#define TRAINERS_COUNT                               33
#define MAX_TRAINERS_COUNT                          864

// The deleted Hoenn trainers, still named by dead systems
#include "constants/removed_trainers.h"

#endif  // GUARD_CONSTANTS_OPPONENTS_H
