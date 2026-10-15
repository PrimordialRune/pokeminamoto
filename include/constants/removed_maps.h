#ifndef GUARD_CONSTANTS_REMOVED_MAPS_H
#define GUARD_CONSTANTS_REMOVED_MAPS_H

// Constants for Hoenn maps and layouts that were deleted from data/maps and
// data/layouts. Only Emerald systems that can no longer be reached still use
// them (Battle Frontier, Trainer Hill, contests, Secret Bases, roamers,
// rematches, region map, TV...). Every MAP_ value here sits in one of map
// groups 0x7B to 0x7E, which do not exist, so comparing one with the current
// map is always false. Map numbers start at 0x40 so that the few Emerald checks
// that compare a map number alone (assuming group 0) don't match a real map
// either, as long as no map group grows past 64 maps. Never warp to one of
// these. When a system is deleted, drop its entries here; when nothing includes
// this file any more, delete it.

#define MAP_GROUP_REMOVED   0x7E
#define MAP_GROUP_REMOVED_2 0x7D
#define MAP_GROUP_REMOVED_3 0x7C
#define MAP_GROUP_REMOVED_4 0x7B

// Maps
enum
{
    MAP_ABANDONED_SHIP_ROOMS2_1F                        = (0x40 | (MAP_GROUP_REMOVED << 8)),
    MAP_ABANDONED_SHIP_ROOMS_1F                         = (0x41 | (MAP_GROUP_REMOVED << 8)),
    MAP_ALTERING_CAVE                                   = (0x42 | (MAP_GROUP_REMOVED << 8)),
    MAP_ANCIENT_TOMB                                    = (0x43 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_ARENA_BATTLE_ROOM        = (0x44 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_ARENA_CORRIDOR           = (0x45 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_ARENA_LOBBY              = (0x46 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_DOME_BATTLE_ROOM         = (0x47 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_DOME_CORRIDOR            = (0x48 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_DOME_LOBBY               = (0x49 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_DOME_PRE_BATTLE_ROOM     = (0x4A | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM      = (0x4B | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_FACTORY_LOBBY            = (0x4C | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM  = (0x4D | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PALACE_BATTLE_ROOM       = (0x4E | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PALACE_CORRIDOR          = (0x4F | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PALACE_LOBBY             = (0x50 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_CORRIDOR            = (0x51 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_LOBBY               = (0x52 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_FINAL          = (0x53 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_NORMAL         = (0x54 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS      = (0x55 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_THREE_PATH_ROOM     = (0x56 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR            = (0x57 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY            = (0x58 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_TOP              = (0x59 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_BATTLE_ROOM        = (0x5A | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_LOBBY              = (0x5B | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_BATTLE_ROOM  = (0x5C | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_CORRIDOR     = (0x5D | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_PARTNER_ROOM = (0x5E | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_MART                            = (0x5F | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_OUTSIDE_EAST                    = (0x60 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_OUTSIDE_WEST                    = (0x61 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_POKEMON_CENTER_1F               = (0x62 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_POKEMON_CENTER_2F               = (0x63 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_PYRAMID_SQUARE01                         = (0x64 | (MAP_GROUP_REMOVED << 8)),
    MAP_BIRTH_ISLAND_EXTERIOR                           = (0x65 | (MAP_GROUP_REMOVED << 8)),
    MAP_DESERT_RUINS                                    = (0x66 | (MAP_GROUP_REMOVED << 8)),
    MAP_DEWFORD_TOWN                                    = (0x67 | (MAP_GROUP_REMOVED << 8)),
    MAP_DEWFORD_TOWN_POKEMON_CENTER_1F                  = (0x68 | (MAP_GROUP_REMOVED << 8)),
    MAP_DEWFORD_TOWN_POKEMON_CENTER_2F                  = (0x69 | (MAP_GROUP_REMOVED << 8)),
    MAP_EVER_GRANDE_CITY                                = (0x6A | (MAP_GROUP_REMOVED << 8)),
    MAP_EVER_GRANDE_CITY_POKEMON_CENTER_1F              = (0x6B | (MAP_GROUP_REMOVED << 8)),
    MAP_EVER_GRANDE_CITY_POKEMON_CENTER_2F              = (0x6C | (MAP_GROUP_REMOVED << 8)),
    MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_1F              = (0x6D | (MAP_GROUP_REMOVED << 8)),
    MAP_EVER_GRANDE_CITY_POKEMON_LEAGUE_2F              = (0x6E | (MAP_GROUP_REMOVED << 8)),
    MAP_FALLARBOR_TOWN                                  = (0x6F | (MAP_GROUP_REMOVED << 8)),
    MAP_FALLARBOR_TOWN_MART                             = (0x70 | (MAP_GROUP_REMOVED << 8)),
    MAP_FALLARBOR_TOWN_POKEMON_CENTER_1F                = (0x71 | (MAP_GROUP_REMOVED << 8)),
    MAP_FALLARBOR_TOWN_POKEMON_CENTER_2F                = (0x72 | (MAP_GROUP_REMOVED << 8)),
    MAP_FARAWAY_ISLAND_INTERIOR                         = (0x73 | (MAP_GROUP_REMOVED << 8)),
    MAP_FORTREE_CITY                                    = (0x74 | (MAP_GROUP_REMOVED << 8)),
    MAP_FORTREE_CITY_GYM                                = (0x75 | (MAP_GROUP_REMOVED << 8)),
    MAP_FORTREE_CITY_MART                               = (0x76 | (MAP_GROUP_REMOVED << 8)),
    MAP_FORTREE_CITY_POKEMON_CENTER_1F                  = (0x77 | (MAP_GROUP_REMOVED << 8)),
    MAP_FORTREE_CITY_POKEMON_CENTER_2F                  = (0x78 | (MAP_GROUP_REMOVED << 8)),
    MAP_ISLAND_CAVE                                     = (0x79 | (MAP_GROUP_REMOVED << 8)),
    MAP_JAGGED_PASS                                     = (0x7A | (MAP_GROUP_REMOVED << 8)),
    MAP_LAVARIDGE_TOWN                                  = (0x7B | (MAP_GROUP_REMOVED << 8)),
    MAP_LAVARIDGE_TOWN_MART                             = (0x7C | (MAP_GROUP_REMOVED << 8)),
    MAP_LAVARIDGE_TOWN_POKEMON_CENTER_1F                = (0x7D | (MAP_GROUP_REMOVED << 8)),
    MAP_LAVARIDGE_TOWN_POKEMON_CENTER_2F                = (0x7E | (MAP_GROUP_REMOVED << 8)),
    MAP_LILYCOVE_CITY                                   = (0x7F | (MAP_GROUP_REMOVED << 8)),
    MAP_LILYCOVE_CITY_COVE_LILY_MOTEL_1F                = (0x40 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_DEPARTMENT_STORE_1F               = (0x41 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_DEPARTMENT_STORE_2F               = (0x42 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_DEPARTMENT_STORE_3F               = (0x43 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_DEPARTMENT_STORE_4F               = (0x44 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_DEPARTMENT_STORE_5F               = (0x45 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_DEPARTMENT_STORE_ROOFTOP          = (0x46 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_POKEMON_CENTER_1F                 = (0x47 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_LILYCOVE_CITY_POKEMON_CENTER_2F                 = (0x48 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MARINE_CAVE_END                                 = (0x49 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MARINE_CAVE_ENTRANCE                            = (0x4A | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MAUVILLE_CITY                                   = (0x4B | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MAUVILLE_CITY_MART                              = (0x4C | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MAUVILLE_CITY_POKEMON_CENTER_1F                 = (0x4D | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MAUVILLE_CITY_POKEMON_CENTER_2F                 = (0x4E | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_METEOR_FALLS_1F_2R                              = (0x50 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MOSSDEEP_CITY                                   = (0x51 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MOSSDEEP_CITY_MART                              = (0x52 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MOSSDEEP_CITY_POKEMON_CENTER_1F                 = (0x53 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MOSSDEEP_CITY_POKEMON_CENTER_2F                 = (0x54 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MT_CHIMNEY                                      = (0x57 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MT_CHIMNEY_CABLE_CAR_STATION                    = (0x58 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MT_PYRE_3F                                      = (0x59 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_MT_PYRE_6F                                      = (0x5A | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PACIFIDLOG_TOWN                                 = (0x5B | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PACIFIDLOG_TOWN_POKEMON_CENTER_1F               = (0x5C | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PACIFIDLOG_TOWN_POKEMON_CENTER_2F               = (0x5D | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PETALBURG_CITY                                  = (0x5E | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PETALBURG_CITY_MART                             = (0x5F | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PETALBURG_CITY_POKEMON_CENTER_1F                = (0x60 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PETALBURG_CITY_POKEMON_CENTER_2F                = (0x61 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_PETALBURG_WOODS                                 = (0x62 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE102                                        = (0x63 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE104                                        = (0x64 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE105                                        = (0x65 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE106                                        = (0x66 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE107                                        = (0x67 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE108                                        = (0x68 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE109                                        = (0x69 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE110                                        = (0x6A | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE110_SEASIDE_CYCLING_ROAD_NORTH_ENTRANCE    = (0x6B | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE110_TRICK_HOUSE_PUZZLE6                    = (0x6C | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE111                                        = (0x6D | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE112                                        = (0x6E | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE112_CABLE_CAR_STATION                      = (0x6F | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE113                                        = (0x70 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE114                                        = (0x71 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE115                                        = (0x72 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE116                                        = (0x73 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE117                                        = (0x74 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE118                                        = (0x75 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE119                                        = (0x76 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE120                                        = (0x79 | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE121                                        = (0x7A | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE121_SAFARI_ZONE_ENTRANCE                   = (0x7B | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE122                                        = (0x7C | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE123                                        = (0x7D | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE124                                        = (0x7E | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE125                                        = (0x7F | (MAP_GROUP_REMOVED_2 << 8)),
    MAP_ROUTE126                                        = (0x40 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE127                                        = (0x41 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE128                                        = (0x42 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE129                                        = (0x43 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE130                                        = (0x44 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE131                                        = (0x45 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE132                                        = (0x46 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE133                                        = (0x47 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_ROUTE134                                        = (0x48 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_RUSTBORO_CITY                                   = (0x49 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_RUSTBORO_CITY_MART                              = (0x4A | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_RUSTBORO_CITY_POKEMON_CENTER_1F                 = (0x4B | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_RUSTBORO_CITY_POKEMON_CENTER_2F                 = (0x4C | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_RUSTURF_TUNNEL                                  = (0x4D | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SEALED_CHAMBER_OUTER_ROOM                       = (0x4F | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BLUE_CAVE1                          = (0x50 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BLUE_CAVE2                          = (0x51 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BLUE_CAVE3                          = (0x52 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BLUE_CAVE4                          = (0x53 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BROWN_CAVE1                         = (0x54 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BROWN_CAVE2                         = (0x55 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BROWN_CAVE3                         = (0x56 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_BROWN_CAVE4                         = (0x57 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_RED_CAVE1                           = (0x58 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_RED_CAVE2                           = (0x59 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_RED_CAVE3                           = (0x5A | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_RED_CAVE4                           = (0x5B | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_SHRUB1                              = (0x5C | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_SHRUB2                              = (0x5D | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_SHRUB3                              = (0x5E | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_SHRUB4                              = (0x5F | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_TREE1                               = (0x60 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_TREE2                               = (0x61 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_TREE3                               = (0x62 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_TREE4                               = (0x63 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_YELLOW_CAVE1                        = (0x64 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_YELLOW_CAVE2                        = (0x65 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_YELLOW_CAVE3                        = (0x66 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SECRET_BASE_YELLOW_CAVE4                        = (0x67 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SLATEPORT_CITY                                  = (0x68 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SLATEPORT_CITY_MART                             = (0x69 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SLATEPORT_CITY_POKEMON_CENTER_1F                = (0x6A | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SLATEPORT_CITY_POKEMON_CENTER_2F                = (0x6B | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SOOTOPOLIS_CITY                                 = (0x6C | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SOOTOPOLIS_CITY_MART                            = (0x6D | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SOOTOPOLIS_CITY_POKEMON_CENTER_1F               = (0x6E | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SOOTOPOLIS_CITY_POKEMON_CENTER_2F               = (0x6F | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SS_TIDAL_CORRIDOR                               = (0x70 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SS_TIDAL_LOWER_DECK                             = (0x71 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_SS_TIDAL_ROOMS                                  = (0x72 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TERRA_CAVE_END                                  = (0x73 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TERRA_CAVE_ENTRANCE                             = (0x74 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TRAINER_HILL_2F                                 = (0x75 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TRAINER_HILL_3F                                 = (0x76 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TRAINER_HILL_4F                                 = (0x77 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TRAINER_HILL_ENTRANCE                           = (0x78 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_TRAINER_HILL_ROOF                               = (0x79 | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_UNDERWATER_MARINE_CAVE                          = (0x7A | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_UNDERWATER_ROUTE105                             = (0x7B | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_UNDERWATER_ROUTE125                             = (0x7C | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_UNDERWATER_ROUTE127                             = (0x7D | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_UNDERWATER_ROUTE129                             = (0x7E | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_VERDANTURF_TOWN                                 = (0x7F | (MAP_GROUP_REMOVED_3 << 8)),
    MAP_VERDANTURF_TOWN_MART                            = (0x40 | (MAP_GROUP_REMOVED_4 << 8)),
    MAP_VERDANTURF_TOWN_POKEMON_CENTER_1F               = (0x41 | (MAP_GROUP_REMOVED_4 << 8)),
    MAP_VERDANTURF_TOWN_POKEMON_CENTER_2F               = (0x42 | (MAP_GROUP_REMOVED_4 << 8)),
    MAP_VICTORY_ROAD_1F                                 = (0x43 | (MAP_GROUP_REMOVED_4 << 8)),
};

// Layouts
enum
{
    LAYOUT_ALTERING_CAVE                                  = (0x40 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_FACTORY_BATTLE_ROOM     = (0x41 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_FACTORY_PRE_BATTLE_ROOM = (0x42 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_NORMAL        = (0x43 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_UNUSED        = (0x44 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS     = (0x45 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_PIKE_THREE_PATH_ROOM    = (0x46 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_FLOOR           = (0x47 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_FRONTIER_BATTLE_PYRAMID_TOP             = (0x48 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_TENT_BATTLE_ROOM                        = (0x49 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_BATTLE_TENT_CORRIDOR                           = (0x4A | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_SS_TIDAL_CORRIDOR                              = (0x4B | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_SS_TIDAL_LOWER_DECK                            = (0x4C | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_SS_TIDAL_ROOMS                                 = (0x4D | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_TRAINER_HILL_1F                                = (0x4E | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_TRAINER_HILL_2F                                = (0x4F | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_TRAINER_HILL_3F                                = (0x50 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_TRAINER_HILL_4F                                = (0x51 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_TRAINER_HILL_ENTRANCE                          = (0x52 | (MAP_GROUP_REMOVED << 8)),
    LAYOUT_TRAINER_HILL_ROOF                              = (0x53 | (MAP_GROUP_REMOVED << 8)),
};

// Heal locations (none of these exist any more, so Fly and respawn skip them)
#define HEAL_LOCATION_BATTLE_FRONTIER_OUTSIDE_EAST    HEAL_LOCATION_NONE
#define HEAL_LOCATION_DEWFORD_TOWN                    HEAL_LOCATION_NONE
#define HEAL_LOCATION_EVER_GRANDE_CITY                HEAL_LOCATION_NONE
#define HEAL_LOCATION_EVER_GRANDE_CITY_POKEMON_LEAGUE HEAL_LOCATION_NONE
#define HEAL_LOCATION_FALLARBOR_TOWN                  HEAL_LOCATION_NONE
#define HEAL_LOCATION_FORTREE_CITY                    HEAL_LOCATION_NONE
#define HEAL_LOCATION_LILYCOVE_CITY                   HEAL_LOCATION_NONE
#define HEAL_LOCATION_MAUVILLE_CITY                   HEAL_LOCATION_NONE
#define HEAL_LOCATION_MOSSDEEP_CITY                   HEAL_LOCATION_NONE
#define HEAL_LOCATION_PACIFIDLOG_TOWN                 HEAL_LOCATION_NONE
#define HEAL_LOCATION_RUSTBORO_CITY                   HEAL_LOCATION_NONE
#define HEAL_LOCATION_SLATEPORT_CITY                  HEAL_LOCATION_NONE
#define HEAL_LOCATION_SOOTOPOLIS_CITY                 HEAL_LOCATION_NONE
#define HEAL_LOCATION_SOUTHERN_ISLAND_EXTERIOR        HEAL_LOCATION_NONE
#define HEAL_LOCATION_VERDANTURF_TOWN                 HEAL_LOCATION_NONE

#define HEAL_LOCATION_LAVARIDGE_TOWN HEAL_LOCATION_NONE
#define HEAL_LOCATION_PETALBURG_CITY HEAL_LOCATION_NONE

// Object event local IDs (original values)
#define LOCALID_BIRTH_ISLAND_EXTERIOR_ROCK            1
#define LOCALID_CONTESTANT_1                          3
#define LOCALID_CONTESTANT_2                          4
#define LOCALID_CONTESTANT_3                          5
#define LOCALID_CONTESTANT_4                          14
#define LOCALID_DAYCARE_LADY                          1
#define LOCALID_FALLARBOR_MART_CLERK                  1
#define LOCALID_FARAWAY_ISLAND_MEW                    1
#define LOCALID_FORTREE_MART_CLERK                    1
#define LOCALID_FRONTIER_MART_CLERK                   1
#define LOCALID_LAVARIDGE_MART_CLERK                  1
#define LOCALID_MAUVILLE_MART_CLERK                   1
#define LOCALID_MOSSDEEP_MART_CLERK                   1
#define LOCALID_PETALBURG_MART_CLERK                  1
#define LOCALID_PIKE_ROOM_NPC_1                       1
#define LOCALID_PIKE_ROOM_NPC_2                       2
#define LOCALID_ROUTE111_GABBY_1                      14
#define LOCALID_ROUTE111_GABBY_2                      21
#define LOCALID_ROUTE111_GABBY_3                      23
#define LOCALID_ROUTE111_PLAYER_FALLING               45
#define LOCALID_ROUTE111_TY_1                         13
#define LOCALID_ROUTE111_TY_2                         22
#define LOCALID_ROUTE111_TY_3                         24
#define LOCALID_ROUTE118_GABBY_1                      5
#define LOCALID_ROUTE118_GABBY_2                      8
#define LOCALID_ROUTE118_GABBY_3                      10
#define LOCALID_ROUTE118_TY_1                         6
#define LOCALID_ROUTE118_TY_2                         9
#define LOCALID_ROUTE118_TY_3                         11
#define LOCALID_ROUTE120_GABBY_1                      18
#define LOCALID_ROUTE120_GABBY_2                      19
#define LOCALID_ROUTE120_TY_1                         17
#define LOCALID_ROUTE120_TY_2                         20
#define LOCALID_RUSTBORO_MART_CLERK                   1
#define LOCALID_RUSTURF_TUNNEL_WANDA                  10
#define LOCALID_RUSTURF_TUNNEL_WANDAS_BF              1
#define LOCALID_SLATEPORT_ENERGY_GURU                 25
#define LOCALID_SLATEPORT_MART_CLERK                  1
#define LOCALID_SOOTOPOLIS_MART_CLERK                 1
#define LOCALID_TOWER_LOBBY_REPORTER                  5
#define LOCALID_VERDANTURF_MART_CLERK                 1

#endif // GUARD_CONSTANTS_REMOVED_MAPS_H
