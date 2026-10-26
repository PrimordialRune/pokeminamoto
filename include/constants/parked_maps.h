#ifndef GUARD_CONSTANTS_PARKED_MAPS_H
#define GUARD_CONSTANTS_PARKED_MAPS_H

// The map and layout ids that the parked Battle Frontier code and the Safari
// Zone exit scripts still compare against. Their maps were deleted from
// data/maps and data/layouts, and each value sits in a map group (0x7B-0x7E)
// that does not exist, so a comparison with the current map is always false.
// Map numbers start at 0x40 so that checks on a map number alone (assuming
// group 0) don't match a real map either. Never warp to one of these. Delete
// an entry together with the code that uses it, and this file with the last.

#define MAP_GROUP_REMOVED   0x7E
#define MAP_GROUP_REMOVED_2 0x7D
#define MAP_GROUP_REMOVED_3 0x7C
#define MAP_GROUP_REMOVED_4 0x7B

// Maps
enum
{
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_LOBBY               = (0x52 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_FINAL          = (0x53 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_NORMAL         = (0x54 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_ROOM_WILD_MONS      = (0x55 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PIKE_THREE_PATH_ROOM     = (0x56 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_PYRAMID_LOBBY            = (0x58 | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_LOBBY              = (0x5B | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_CORRIDOR     = (0x5D | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_FRONTIER_BATTLE_TOWER_MULTI_PARTNER_ROOM = (0x5E | (MAP_GROUP_REMOVED << 8)),
    MAP_BATTLE_PYRAMID_SQUARE01                         = (0x64 | (MAP_GROUP_REMOVED << 8)),
    MAP_ROUTE121_SAFARI_ZONE_ENTRANCE                   = (0x7B | (MAP_GROUP_REMOVED_2 << 8)),
};

// Layouts
enum
{
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
};

// Object local ids that the parked scripts name
#define LOCALID_DAYCARE_LADY    1
#define LOCALID_PIKE_ROOM_NPC_1 1
#define LOCALID_PIKE_ROOM_NPC_2 2

#endif // GUARD_CONSTANTS_PARKED_MAPS_H
