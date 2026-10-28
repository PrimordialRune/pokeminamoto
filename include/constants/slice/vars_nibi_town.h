// Studio scaffold (world M1): saved vars for workstream "nibi-town".
// Nibi City streets, museum, market, houses, Nibi CVS, Aizawa house.
// Rules (slice-plan section 5): rename the placeholders in place, never
// reorder, never change an id. Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_VARS_NIBI_TOWN_H
#define GUARD_CONSTANTS_SLICE_VARS_NIBI_TOWN_H

#define VAR_NIBI_STATE                           0x405A // east exit: 0 not escorted yet, 1 escorted to the gym (the boy blocks), 2 open (B2); was VAR_RUSTBORO_CITY_STATE
#define VAR_Q_FOSSILS                            0x40E5 // NIB-302: 0 none, 1 the fossil notice was read (sign or inn board), 2 Fujita answered it; was VAR_UNUSED_0x40E5
#define VAR_Q_POET                               0x40FB // NIB-305: 0 none, 1 stone shown, 2 Onix told, 3 both; step 3 (Otsukimi, later) will use 4+; was VAR_UNUSED_0x40FB

#endif // GUARD_CONSTANTS_SLICE_VARS_NIBI_TOWN_H
