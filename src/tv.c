#include "global.h"
#include "event_data.h"
#include "field_camera.h"
#include "fieldmap.h"
#include "random.h"
#include "string_util.h"
#include "strings.h"
#include "tv.h"
#include "constants/metatile_behaviors.h"
#include "constants/metatile_labels.h"
#include "constants/layouts.h"

// Only the TV screen in the player's house is left: shows, news, outbreaks
// and Gabby & Ty are gone, so a TV just says that Mom or Dad might like the
// program.

static void SetTVMetatilesOnMap(int width, int height, u16 metatileId)
{
    int x;
    int y;

    for (y = 0; y < height; y++)
    {
        for (x = 0; x < width; x++)
        {
            if (MapGridGetMetatileBehaviorAt(x, y) == MB_TELEVISION)
                MapGridSetMetatileIdAt(x, y, metatileId | MAPGRID_COLLISION_MASK);
        }
    }
}

void TurnOffTVScreen(void)
{
    SetTVMetatilesOnMap(gBackupMapLayout.width, gBackupMapLayout.height, METATILE_Building_TV_Off);
    DrawWholeMapView();
}

void TurnOnTVScreen(void)
{
    SetTVMetatilesOnMap(gBackupMapLayout.width, gBackupMapLayout.height, METATILE_Building_TV_On);
    DrawWholeMapView();
}

u8 CheckForPlayersHouseNews(void)
{
    // Check if not in Littleroot house map group
    if (gSaveBlock1Ptr->location.mapGroup != MAP_GROUP(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F))
        return PLAYERS_HOUSE_TV_NONE;

    // Check if not in player's house (dependent on gender)
    if (gSaveBlock2Ptr->playerGender == MALE)
    {
        if (gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F))
            return PLAYERS_HOUSE_TV_NONE;
    }
    else
    {
        if (gSaveBlock1Ptr->location.mapNum != MAP_NUM(MAP_LITTLEROOT_TOWN_MAYS_HOUSE_1F))
            return PLAYERS_HOUSE_TV_NONE;
    }

    if (FlagGet(FLAG_SYS_TV_LATIAS_LATIOS) == TRUE)
        return PLAYERS_HOUSE_TV_LATI;

    if (FlagGet(FLAG_SYS_TV_HOME) == TRUE)
        return PLAYERS_HOUSE_TV_MOVIE;

    return PLAYERS_HOUSE_TV_LATI;
}

void GetMomOrDadStringForTVMessage(void)
{
    // If the player is checking the TV in their house it will only refer to their Mom.
    if (gSaveBlock1Ptr->location.mapGroup == MAP_GROUP(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F))
    {
        if (gSaveBlock2Ptr->playerGender == MALE)
        {
            if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_LITTLEROOT_TOWN_BRENDANS_HOUSE_1F))
            {
                StringCopy(gStringVar1, gText_Mom);
                VarSet(VAR_TEMP_3, 1);
            }
        }
        else
        {
            if (gSaveBlock1Ptr->location.mapNum == MAP_NUM(MAP_LITTLEROOT_TOWN_MAYS_HOUSE_1F))
            {
                StringCopy(gStringVar1, gText_Mom);
                VarSet(VAR_TEMP_3, 1);
            }
        }
    }
    if (VarGet(VAR_TEMP_3) == 1)
    {
        StringCopy(gStringVar1, gText_Mom);
    }
    else if (VarGet(VAR_TEMP_3) == 2)
    {
        StringCopy(gStringVar1, gText_Dad);
    }
    else if (VarGet(VAR_TEMP_3) > 2)
    {
        // Should only happen if VAR_TEMP_3 is already in use by something else.
        if (VarGet(VAR_TEMP_3) % 2 == 0)
            StringCopy(gStringVar1, gText_Mom);
        else
            StringCopy(gStringVar1, gText_Dad);
    }
    else
    {
        // Randomly choose whether to refer to Mom or Dad.
        // NOTE: Because of this, any map that has a TV in it shouldn't rely on VAR_TEMP_3.
        //       If its value is 0, checking the TV will set it to 1 or 2.
        if (Random() % 2 != 0)
        {
            StringCopy(gStringVar1, gText_Mom);
            VarSet(VAR_TEMP_3, 1);
        }
        else
        {
            StringCopy(gStringVar1, gText_Dad);
            VarSet(VAR_TEMP_3, 2);
        }
    }
}

void UpdateTVScreensOnMap(int width, int height)
{
    if (CheckForPlayersHouseNews() == PLAYERS_HOUSE_TV_LATI)
        SetTVMetatilesOnMap(width, height, METATILE_Building_TV_On);
}
