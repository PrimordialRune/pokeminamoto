#ifndef GUARD_CONSTANTS_BATTLE_SETUP_H
#define GUARD_CONSTANTS_BATTLE_SETUP_H

#define TRAINER_BATTLE_SINGLE                           0
#define TRAINER_BATTLE_CONTINUE_SCRIPT_NO_MUSIC         1
#define TRAINER_BATTLE_CONTINUE_SCRIPT                  2
#define TRAINER_BATTLE_SINGLE_NO_INTRO_TEXT             3
#define TRAINER_BATTLE_DOUBLE                           4
#define TRAINER_BATTLE_REMATCH                          5
#define TRAINER_BATTLE_CONTINUE_SCRIPT_DOUBLE           6
#define TRAINER_BATTLE_REMATCH_DOUBLE                   7
#define TRAINER_BATTLE_CONTINUE_SCRIPT_DOUBLE_NO_MUSIC  8
#define TRAINER_BATTLE_PYRAMID                          9
#define TRAINER_BATTLE_SET_TRAINER_A                    10
#define TRAINER_BATTLE_SET_TRAINER_B                    11
// Story rival fight that cannot be lost for good (studio slice, core). Continue-script
// mode with one more text (the foe's win text). On a loss: no money lost, no warp, no
// whiteout, the foe's win text is shown, the party is healed (HP and status, not TP) and the
// script runs with VAR_RESULT = 0 (1 when the player won). A win sets the trainer flag as usual.
#define TRAINER_BATTLE_EARLY_RIVAL                      12

// gBattleTypeFlags bit set for a TRAINER_BATTLE_EARLY_RIVAL fight (bit 11 was unused in battle.h).
#define BATTLE_TYPE_EARLY_RIVAL                         (1 << 11)

// Other core constants shared with scripts (this header is one the script build includes).

// CheckPartyMonForHandover (src/script_pokemon_util.c): VAR_RESULT, and the VAR_0x8006 wildcard
#define HANDOVER_OK            0
#define HANDOVER_NONE          1 // no party member matches species, level and met location
#define HANDOVER_NOT_FULL_HP   2 // a match exists but is hurt (only when full HP was asked for)
#define HANDOVER_LAST_MON      3 // the match is the only party member
#define HANDOVER_ANY_LOCATION  0xFFFF

// GetWhiteoutArrivalPlace (src/overworld.c): where a whiteout woke the player
#define WHITEOUT_ARRIVAL_NONE        0
#define WHITEOUT_ARRIVAL_TOKIWA_INN  1
#define WHITEOUT_ARRIVAL_NIBI_INN    2
#define WHITEOUT_ARRIVAL_MASARA_HOME 3

#endif // GUARD_CONSTANTS_BATTLE_SETUP_H
