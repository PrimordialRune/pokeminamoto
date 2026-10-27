// Each species' Special and how it is unsealed (struct SpecialMoveInfo in include/special_move.h).
// Species left out have no Special yet: they stay sealed, and their condition is the Bond
// fallback (friendship >= SPECIAL_BOND_FRIENDSHIP) so nothing changes once a move is named.
// Awakening is per mon and survives evolution; the move comes from the current species.
//
// v1 (2026-10-26): only the Charmander line has a real Special. Bulbasaur and Squirtle use the
// Bond fallback until their Specials (GREEDY PLANT, SKULL BASH in the design notes) are made.

#define CHARMANDER_LINE_SPECIAL                         \
{                                                       \
    .move = MOVE_FLAME_CLAWS,                           \
    .condition = SPECIAL_COND_DEED,                     \
    .threshold = 3,                                     \
    .deed = SPECIAL_DEED_WIN_VS_TYPES,                  \
    .deedArgs = {TYPE_ROCK, TYPE_WATER},                \
}

#if STARTER_SIGNATURES_PROPOSAL
// PLACEHOLDER (Q97): Omar's future names GREEDY PLANT and SKULL BASH with invented numbers, off by default.
// Thread "Research TCG attacks as lost moves" owns the real design.
#define BULBASAUR_LINE_SPECIAL                          \
{                                                       \
    .move = MOVE_GREEDY_PLANT,                          \
    .condition = SPECIAL_COND_DEED,                     \
    .threshold = 3,                                     \
    .deed = SPECIAL_DEED_WIN_VS_TYPES,                  \
    .deedArgs = {TYPE_ROCK, TYPE_GROUND},               \
}

#define SQUIRTLE_LINE_SPECIAL                           \
{                                                       \
    .move = MOVE_SKULL_BASH_SIGNATURE,                  \
    .condition = SPECIAL_COND_DEED,                     \
    .threshold = 4,                                     \
    .deed = SPECIAL_DEED_WIN_VS_TYPES,                  \
    .deedArgs = {TYPE_BUG, TYPE_POISON},                \
}
#endif

const struct SpecialMoveInfo gSpecialMoves[NUM_SPECIES] =
{
#if STARTER_SIGNATURES_PROPOSAL
    [SPECIES_BULBASAUR]  = BULBASAUR_LINE_SPECIAL,
    [SPECIES_IVYSAUR]    = BULBASAUR_LINE_SPECIAL,
    [SPECIES_VENUSAUR]   = BULBASAUR_LINE_SPECIAL,
    [SPECIES_SQUIRTLE]   = SQUIRTLE_LINE_SPECIAL,
    [SPECIES_WARTORTLE]  = SQUIRTLE_LINE_SPECIAL,
    [SPECIES_BLASTOISE]  = SQUIRTLE_LINE_SPECIAL,
#endif

    // The tail flame burns hottest when it faces what it fears: win 3 battles in which a Rock
    // or Water foe fainted while it was on the field.
    [SPECIES_CHARMANDER] = CHARMANDER_LINE_SPECIAL,
    [SPECIES_CHARMELEON] = CHARMANDER_LINE_SPECIAL,
    [SPECIES_CHARIZARD]  = CHARMANDER_LINE_SPECIAL,

    // No unlock condition designed yet (Bond fallback). A gym leader's ace has it unlocked.
    [SPECIES_FARFETCHD] = {.move = MOVE_LEEK_SLAP, .condition = SPECIAL_COND_BOND},
};
