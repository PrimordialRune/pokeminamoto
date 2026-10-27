// Studio scaffold (world M1): cross-area names for the Masara-to-Nibi slice.
// Owner: director. Frozen after the scaffold; ask for new names in studio/requests.md.
// Everyone may read these. The comment names who writes each one (slice-plan 5.1, 5.2, 5.8).
// Ids don't move, so the save layout is unchanged.

#ifndef GUARD_CONSTANTS_SLICE_SHARED_H
#define GUARD_CONSTANTS_SLICE_SHARED_H

// Flags 0x4CD-0x4EF
#define FLAG_Q_TAG_CAUGHT                        0x4CD // writer: tokiwa-spine (R01-301), north (R22-302)
#define FLAG_Q_TAG_KO                            0x4CE // writer: tokiwa-spine (R01-301), north (R22-302)
#define FLAG_Q_TAG_FLED                          0x4CF // writer: tokiwa-spine
#define FLAG_Q_TAG_KEN                           0x4D0 // writer: north (FOR-003)
#define FLAG_Q_SANDSHREW_GIVEN                   0x4D1 // writer: tokiwa-spine
#define FLAG_Q_SANDSHREW_KEPT                    0x4D2 // writer: tokiwa-spine
#define FLAG_Q_SANDSHREW_R22                     0x4D3 // writer: north (R22-301 capture)
#define FLAG_SEEN_OMURA_NOTICE                   0x4D4 // writer: tokiwa-town (TOK-212)
#define FLAG_Q_FIORA_MET                         0x4D5 // writer: masara
#define FLAG_Q_RIVAL1_WIN                        0x4D6 // writer: masara
#define FLAG_Q_FORM_SEALED                       0x4D7 // writer: masara
#define FLAG_Q_CARD                              0x4D8 // writer: tokiwa-spine
#define FLAG_Q_PIDGEY_KEPT                       0x4D9 // writer: tokiwa-town
#define FLAG_Q_PIDGEY_RELEASED                   0x4DA // writer: tokiwa-town
#define FLAG_Q_LOST_YUJI                         0x4DB // writer: tokiwa-spine
#define FLAG_Q_YUJI_MET                          0x4DC // writer: tokiwa-spine
#define FLAG_Q_YUJI_SLEPT                        0x4DD // writer: tokiwa-spine (after its inn rest)
#define FLAG_Q_LOST_TAKESHI                      0x4DE // writer: nibi-spine
#define FLAG_Q_HIDA_ACCEPTED                     0x4DF // writer: nibi-spine
#define FLAG_Q_HIDA_DELIVERED                    0x4E0 // writer: nibi-spine
#define FLAG_Q_GEODUDE_DAY                       0x4E1 // writer: nibi-spine (R03-301)
#define FLAG_Q_GEODUDE_NITE                      0x4E2 // writer: nibi-spine
#define FLAG_SEEN_NIB_C7                         0x4E3 // writer: nibi-town (NIB-209)
#define FLAG_Q_C7_KNOWN                          0x4E4 // writer: nibi-spine (Mori)
#define FLAG_Q_KEN1_DONE                         0x4E5 // writer: north
#define FLAG_Q_KEN_LEFT_NIBI                     0x4E6 // writer: nibi-spine (first rest with B2)
#define FLAG_VISITED_TOKIWA                      0x4E7 // writer: tokiwa-town
#define FLAG_VISITED_NIBI                        0x4E8 // writer: nibi-town
#define FLAG_SLICE_SHARED_SPARE_1                0x4E9 // director only
#define FLAG_SLICE_SHARED_SPARE_2                0x4EA // director only
#define FLAG_SLICE_SHARED_SPARE_3                0x4EB // director only
#define FLAG_SLICE_SHARED_SPARE_4                0x4EC // director only
#define FLAG_SLICE_SHARED_SPARE_5                0x4ED // director only
#define FLAG_SLICE_SHARED_SPARE_6                0x4EE // director only
#define FLAG_SLICE_SHARED_SPARE_7                0x4EF // director only

// Vars (cross-map contracts C1-C5, slice-plan 5.8; reserves are director only)
#define VAR_Q_TAG                                0x4091 // C1 Okido's tag: 0 none, 1 asked, 2 found, 3 reported; was VAR_UNUSED_0x4091
#define VAR_Q_SANDSHREW                          0x409D // C2 Omura's Sandshrew: 0 none, 1 notice read, 2 Omura met, 3 caught, 4 given, 5 kept, 6 fainted, 7 missed; was VAR_UNUSED_0x409D
#define VAR_Q_LETTER                             0x40B8 // C3 Grandma Inoue's letter: 0, 1 given, 2 delivered; was VAR_UNUSED_0x40B8
#define VAR_Q_PIDGEY                             0x40A1 // C4 injured Pidgey: 0, 1 given, 2 recovered, 3 released, 4 kept; was VAR_UNUSED_0x40A1
#define VAR_Q_HERB                               0x409B // C5 Nanami's herb: 0, 1 asked, 2 picked, 3 handed in; was VAR_UNUSED_0x409B
#define VAR_SLICE_RESERVE_1                      0x40FE // director reserve; was VAR_UNUSED_0x40FE
#define VAR_SLICE_RESERVE_2                      0x40FF // director reserve; was VAR_UNUSED_0x40FF
#define VAR_SLICE_RESERVE_3                      0x4055 // director reserve; was VAR_VERDANTURF_TOWN_STATE
#define VAR_SLICE_RESERVE_4                      0x4059 // director reserve; was VAR_MAUVILLE_CITY_STATE
#define VAR_SLICE_RESERVE_5                      0x4062 // director reserve; was VAR_ROUTE103_STATE

#endif // GUARD_CONSTANTS_SLICE_SHARED_H
