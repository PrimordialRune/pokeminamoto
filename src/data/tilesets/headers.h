const struct Tileset gTileset_General =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_General,
    .palettes = gTilesetPalettes_General,
    .metatiles = gMetatiles_General,
    .metatileAttributes = gMetatileAttributes_General,
    .callback = InitTilesetAnim_General,
};

const struct Tileset gTileset_Petalburg =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Petalburg,
    .palettes = gTilesetPalettes_Petalburg,
    .metatiles = gMetatiles_Petalburg,
    .metatileAttributes = gMetatileAttributes_Petalburg,
    .callback = InitTilesetAnim_Petalburg,
};

const struct Tileset gTileset_Building =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_InsideBuilding,
    .palettes = gTilesetPalettes_InsideBuilding,
    .metatiles = gMetatiles_InsideBuilding,
    .metatileAttributes = gMetatileAttributes_InsideBuilding,
    .callback = InitTilesetAnim_Building,
};

const struct Tileset gTileset_PokemonCenter =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PokemonCenter,
    .palettes = gTilesetPalettes_PokemonCenter,
    .metatiles = gMetatiles_PokemonCenter,
    .metatileAttributes = gMetatileAttributes_PokemonCenter,
    .callback = NULL,
};

const struct Tileset gTileset_Cave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Cave,
    .palettes = gTilesetPalettes_Cave,
    .metatiles = gMetatiles_Cave,
    .metatileAttributes = gMetatileAttributes_Cave,
    .callback = InitTilesetAnim_Cave,
};

const struct Tileset gTileset_CableClub =
{
    .isCompressed = FALSE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_CableClub,
    .palettes = gTilesetPalettes_CableClub,
    .metatiles = gMetatiles_CableClub,
    .metatileAttributes = gMetatileAttributes_CableClub,
    .callback = NULL,
};

const struct Tileset gTileset_SecretBaseRedCave =
{
    .isCompressed = FALSE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_SecretBaseRedCave,
    .palettes = gTilesetPalettes_SecretBaseRedCave,
    .metatiles = gMetatiles_SecretBaseSecondary,
    .metatileAttributes = gMetatileAttributes_SecretBaseSecondary,
    .callback = NULL,
};

const struct Tileset gTileset_InsideOfTruck =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_InsideOfTruck,
    .palettes = gTilesetPalettes_InsideOfTruck,
    .metatiles = gMetatiles_InsideOfTruck,
    .metatileAttributes = gMetatileAttributes_InsideOfTruck,
    .callback = NULL,
};

const struct Tileset gTileset_BrendansMaysHouse =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_BrendansMaysHouse,
    .palettes = gTilesetPalettes_BrendansMaysHouse,
    .metatiles = gMetatiles_BrendansMaysHouse,
    .metatileAttributes = gMetatileAttributes_BrendansMaysHouse,
    .callback = NULL,
};

const struct Tileset gTileset_Lab =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_Lab,
    .palettes = gTilesetPalettes_Lab,
    .metatiles = gMetatiles_Lab,
    .metatileAttributes = gMetatileAttributes_Lab,
    .callback = NULL,
};

const struct Tileset gTileset_GenericBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_GenericBuilding,
    .palettes = gTilesetPalettes_GenericBuilding,
    .metatiles = gMetatiles_GenericBuilding,
    .metatileAttributes = gMetatileAttributes_GenericBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_SecretBase =
{
    .isCompressed = FALSE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_SecretBase,
    .palettes = gTilesetPalettes_SecretBase,
    .metatiles = gMetatiles_SecretBasePrimary,
    .metatileAttributes = gMetatileAttributes_SecretBasePrimary,
    .callback = NULL,
};

const struct Tileset *const gTilesetPointer_SecretBase = &gTileset_SecretBase;
const struct Tileset *const gTilesetPointer_SecretBaseRedCave = &gTileset_SecretBaseRedCave;

const struct Tileset gTileset_MirageTower =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_MirageTower,
    .palettes = gTilesetPalettes_MirageTower,
    .metatiles = gMetatiles_MirageTower,
    .metatileAttributes = gMetatileAttributes_MirageTower,
    .callback = NULL,
};

const struct Tileset gTileset_UnionRoom =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_UnionRoom,
    .palettes = gTilesetPalettes_UnionRoom,
    .metatiles = gMetatiles_UnionRoom,
    .metatileAttributes = gMetatileAttributes_UnionRoom,
    .callback = NULL,
};

const struct Tileset gTileset_Kanto =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_PorytilesManaged_Kanto,
    .palettes = gTilesetPalettes_PorytilesManaged_Kanto,
    .metatiles = gMetatiles_PorytilesManaged_Kanto,
    .metatileAttributes = gMetatileAttributes_PorytilesManaged_Kanto,
    .callback = InitTilesetAnim_PorytilesManaged_Kanto,
};

const struct Tileset gTileset_MasaraTown =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_PorytilesManaged_MasaraTown,
    .palettes = gTilesetPalettes_PorytilesManaged_MasaraTown,
    .metatiles = gMetatiles_PorytilesManaged_MasaraTown,
    .metatileAttributes = gMetatileAttributes_PorytilesManaged_MasaraTown,
    .callback = NULL,
};

// FRLG import begin (tools/frlg_import/import_maps.py)
const struct Tileset gTileset_FrlgBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_FrlgBuilding,
    .palettes = gTilesetPalettes_FrlgBuilding,
    .metatiles = gMetatiles_FrlgBuilding,
    .metatileAttributes = gMetatileAttributes_FrlgBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgGeneral =
{
    .isCompressed = TRUE,
    .isSecondary = FALSE,
    .tiles = gTilesetTiles_FrlgGeneral,
    .palettes = gTilesetPalettes_FrlgGeneral,
    .metatiles = gMetatiles_FrlgGeneral,
    .metatileAttributes = gMetatileAttributes_FrlgGeneral,
    .callback = InitTilesetAnim_FrlgGeneral,
};

const struct Tileset gTileset_FrlgGenericBuilding1 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgGenericBuilding1,
    .palettes = gTilesetPalettes_FrlgGenericBuilding1,
    .metatiles = gMetatiles_FrlgGenericBuilding1,
    .metatileAttributes = gMetatileAttributes_FrlgGenericBuilding1,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgGenericBuilding2 =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgGenericBuilding2,
    .palettes = gTilesetPalettes_FrlgGenericBuilding2,
    .metatiles = gMetatiles_FrlgGenericBuilding2,
    .metatileAttributes = gMetatileAttributes_FrlgGenericBuilding2,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgLab =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgLab,
    .palettes = gTilesetPalettes_FrlgLab,
    .metatiles = gMetatiles_FrlgLab,
    .metatileAttributes = gMetatileAttributes_FrlgLab,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPokemonCenter =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPokemonCenter,
    .palettes = gTilesetPalettes_FrlgPokemonCenter,
    .metatiles = gMetatiles_FrlgPokemonCenter,
    .metatileAttributes = gMetatileAttributes_FrlgPokemonCenter,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgMart =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgMart,
    .palettes = gTilesetPalettes_FrlgMart,
    .metatiles = gMetatiles_FrlgMart,
    .metatileAttributes = gMetatileAttributes_FrlgMart,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCeruleanGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCeruleanGym,
    .palettes = gTilesetPalettes_FrlgCeruleanGym,
    .metatiles = gMetatiles_FrlgCeruleanGym,
    .metatileAttributes = gMetatileAttributes_FrlgCeruleanGym,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCeladonGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCeladonGym,
    .palettes = gTilesetPalettes_FrlgCeladonGym,
    .metatiles = gMetatiles_FrlgCeladonGym,
    .metatileAttributes = gMetatileAttributes_FrlgCeladonGym,
    .callback = InitTilesetAnim_FrlgCeladonGym,
};

const struct Tileset gTileset_FrlgFuchsiaGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgFuchsiaGym,
    .palettes = gTilesetPalettes_FrlgFuchsiaGym,
    .metatiles = gMetatiles_FrlgFuchsiaGym,
    .metatileAttributes = gMetatileAttributes_FrlgFuchsiaGym,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgVermilionGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgVermilionGym,
    .palettes = gTilesetPalettes_FrlgVermilionGym,
    .metatiles = gMetatiles_FrlgVermilionGym,
    .metatileAttributes = gMetatileAttributes_FrlgVermilionGym,
    .callback = InitTilesetAnim_FrlgVermilionGym,
};

const struct Tileset gTileset_FrlgBikeShop =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgBikeShop,
    .palettes = gTilesetPalettes_FrlgBikeShop,
    .metatiles = gMetatiles_FrlgBikeShop,
    .metatileAttributes = gMetatileAttributes_FrlgBikeShop,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgGameCorner =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgGameCorner,
    .palettes = gTilesetPalettes_FrlgGameCorner,
    .metatiles = gMetatiles_FrlgGameCorner,
    .metatileAttributes = gMetatileAttributes_FrlgGameCorner,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPewterGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPewterGym,
    .palettes = gTilesetPalettes_FrlgPewterGym,
    .metatiles = gMetatiles_FrlgPewterGym,
    .metatileAttributes = gMetatileAttributes_FrlgPewterGym,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSaffronGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSaffronGym,
    .palettes = gTilesetPalettes_FrlgSaffronGym,
    .metatiles = gMetatiles_FrlgSaffronGym,
    .metatileAttributes = gMetatileAttributes_FrlgSaffronGym,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCinnabarGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCinnabarGym,
    .palettes = gTilesetPalettes_FrlgCinnabarGym,
    .metatiles = gMetatiles_FrlgCinnabarGym,
    .metatileAttributes = gMetatileAttributes_FrlgCinnabarGym,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgViridianGym =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgViridianGym,
    .palettes = gTilesetPalettes_FrlgViridianGym,
    .metatiles = gMetatiles_FrlgViridianGym,
    .metatileAttributes = gMetatileAttributes_FrlgViridianGym,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSafariZoneBuilding =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSafariZoneBuilding,
    .palettes = gTilesetPalettes_FrlgSafariZoneBuilding,
    .metatiles = gMetatiles_FrlgSafariZoneBuilding,
    .metatileAttributes = gMetatileAttributes_FrlgSafariZoneBuilding,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPalletTown =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPalletTown,
    .palettes = gTilesetPalettes_FrlgPalletTown,
    .metatiles = gMetatiles_FrlgPalletTown,
    .metatileAttributes = gMetatileAttributes_FrlgPalletTown,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgViridianCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgViridianCity,
    .palettes = gTilesetPalettes_FrlgViridianCity,
    .metatiles = gMetatiles_FrlgViridianCity,
    .metatileAttributes = gMetatileAttributes_FrlgViridianCity,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPewterCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPewterCity,
    .palettes = gTilesetPalettes_FrlgPewterCity,
    .metatiles = gMetatiles_FrlgPewterCity,
    .metatileAttributes = gMetatileAttributes_FrlgPewterCity,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCeruleanCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCeruleanCity,
    .palettes = gTilesetPalettes_FrlgCeruleanCity,
    .metatiles = gMetatiles_FrlgCeruleanCity,
    .metatileAttributes = gMetatileAttributes_FrlgCeruleanCity,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgLavenderTown =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgLavenderTown,
    .palettes = gTilesetPalettes_FrlgLavenderTown,
    .metatiles = gMetatiles_FrlgLavenderTown,
    .metatileAttributes = gMetatileAttributes_FrlgLavenderTown,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgVermilionCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgVermilionCity,
    .palettes = gTilesetPalettes_FrlgVermilionCity,
    .metatiles = gMetatiles_FrlgVermilionCity,
    .metatileAttributes = gMetatileAttributes_FrlgVermilionCity,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCeladonCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCeladonCity,
    .palettes = gTilesetPalettes_FrlgCeladonCity,
    .metatiles = gMetatiles_FrlgCeladonCity,
    .metatileAttributes = gMetatileAttributes_FrlgCeladonCity,
    .callback = InitTilesetAnim_FrlgCeladonCity,
};

const struct Tileset gTileset_FrlgFuchsiaCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgFuchsiaCity,
    .palettes = gTilesetPalettes_FrlgFuchsiaCity,
    .metatiles = gMetatiles_FrlgFuchsiaCity,
    .metatileAttributes = gMetatileAttributes_FrlgFuchsiaCity,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCinnabarIsland =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCinnabarIsland,
    .palettes = gTilesetPalettes_FrlgCinnabarIsland,
    .metatiles = gMetatiles_FrlgCinnabarIsland,
    .metatileAttributes = gMetatileAttributes_FrlgCinnabarIsland,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgIndigoPlateau =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgIndigoPlateau,
    .palettes = gTilesetPalettes_FrlgIndigoPlateau,
    .metatiles = gMetatiles_FrlgIndigoPlateau,
    .metatileAttributes = gMetatileAttributes_FrlgIndigoPlateau,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSaffronCity =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSaffronCity,
    .palettes = gTilesetPalettes_FrlgSaffronCity,
    .metatiles = gMetatiles_FrlgSaffronCity,
    .metatileAttributes = gMetatileAttributes_FrlgSaffronCity,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCave,
    .palettes = gTilesetPalettes_FrlgCave,
    .metatiles = gMetatiles_FrlgCave,
    .metatileAttributes = gMetatileAttributes_FrlgCave,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgViridianForest =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgViridianForest,
    .palettes = gTilesetPalettes_FrlgViridianForest,
    .metatiles = gMetatiles_FrlgViridianForest,
    .metatileAttributes = gMetatileAttributes_FrlgViridianForest,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSSAnne =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSSAnne,
    .palettes = gTilesetPalettes_FrlgSSAnne,
    .metatiles = gMetatiles_FrlgSSAnne,
    .metatileAttributes = gMetatileAttributes_FrlgSSAnne,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgDiglettsCave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgDiglettsCave,
    .palettes = gTilesetPalettes_FrlgDiglettsCave,
    .metatiles = gMetatiles_FrlgDiglettsCave,
    .metatileAttributes = gMetatileAttributes_FrlgDiglettsCave,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSilphCo =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSilphCo,
    .palettes = gTilesetPalettes_FrlgSilphCo,
    .metatiles = gMetatiles_FrlgSilphCo,
    .metatileAttributes = gMetatileAttributes_FrlgSilphCo,
    .callback = InitTilesetAnim_FrlgSilphCo,
};

const struct Tileset gTileset_FrlgPokemonMansion =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPokemonMansion,
    .palettes = gTilesetPalettes_FrlgPokemonMansion,
    .metatiles = gMetatiles_FrlgPokemonMansion,
    .metatileAttributes = gMetatileAttributes_FrlgPokemonMansion,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCeruleanCave =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCeruleanCave,
    .palettes = gTilesetPalettes_FrlgCeruleanCave,
    .metatiles = gMetatiles_FrlgCeruleanCave,
    .metatileAttributes = gMetatileAttributes_FrlgCeruleanCave,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgRockTunnel =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgRockTunnel,
    .palettes = gTilesetPalettes_FrlgRockTunnel,
    .metatiles = gMetatiles_FrlgRockTunnel,
    .metatileAttributes = gMetatileAttributes_FrlgRockTunnel,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSeafoamIslands =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSeafoamIslands,
    .palettes = gTilesetPalettes_FrlgSeafoamIslands,
    .metatiles = gMetatiles_FrlgSeafoamIslands,
    .metatileAttributes = gMetatileAttributes_FrlgSeafoamIslands,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPokemonTower =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPokemonTower,
    .palettes = gTilesetPalettes_FrlgPokemonTower,
    .metatiles = gMetatiles_FrlgPokemonTower,
    .metatileAttributes = gMetatileAttributes_FrlgPokemonTower,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPowerPlant =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPowerPlant,
    .palettes = gTilesetPalettes_FrlgPowerPlant,
    .metatiles = gMetatiles_FrlgPowerPlant,
    .metatileAttributes = gMetatileAttributes_FrlgPowerPlant,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSeaCottage =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSeaCottage,
    .palettes = gTilesetPalettes_FrlgSeaCottage,
    .metatiles = gMetatiles_FrlgSeaCottage,
    .metatileAttributes = gMetatileAttributes_FrlgSeaCottage,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgUndergroundPath =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgUndergroundPath,
    .palettes = gTilesetPalettes_FrlgUndergroundPath,
    .metatiles = gMetatiles_FrlgUndergroundPath,
    .metatileAttributes = gMetatileAttributes_FrlgUndergroundPath,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgMuseum =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgMuseum,
    .palettes = gTilesetPalettes_FrlgMuseum,
    .metatiles = gMetatiles_FrlgMuseum,
    .metatileAttributes = gMetatileAttributes_FrlgMuseum,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgBurgledHouse =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgBurgledHouse,
    .palettes = gTilesetPalettes_FrlgBurgledHouse,
    .metatiles = gMetatiles_FrlgBurgledHouse,
    .metatileAttributes = gMetatileAttributes_FrlgBurgledHouse,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgCondominiums =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgCondominiums,
    .palettes = gTilesetPalettes_FrlgCondominiums,
    .metatiles = gMetatiles_FrlgCondominiums,
    .metatileAttributes = gMetatileAttributes_FrlgCondominiums,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgSchool =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgSchool,
    .palettes = gTilesetPalettes_FrlgSchool,
    .metatiles = gMetatiles_FrlgSchool,
    .metatileAttributes = gMetatileAttributes_FrlgSchool,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgRestaurantHotel =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgRestaurantHotel,
    .palettes = gTilesetPalettes_FrlgRestaurantHotel,
    .metatiles = gMetatiles_FrlgRestaurantHotel,
    .metatileAttributes = gMetatileAttributes_FrlgRestaurantHotel,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgDepartmentStore =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgDepartmentStore,
    .palettes = gTilesetPalettes_FrlgDepartmentStore,
    .metatiles = gMetatiles_FrlgDepartmentStore,
    .metatileAttributes = gMetatileAttributes_FrlgDepartmentStore,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgPokemonLeague =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgPokemonLeague,
    .palettes = gTilesetPalettes_FrlgPokemonLeague,
    .metatiles = gMetatiles_FrlgPokemonLeague,
    .metatileAttributes = gMetatileAttributes_FrlgPokemonLeague,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgHallOfFame =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgHallOfFame,
    .palettes = gTilesetPalettes_FrlgHallOfFame,
    .metatiles = gMetatiles_FrlgHallOfFame,
    .metatileAttributes = gMetatileAttributes_FrlgHallOfFame,
    .callback = NULL,
};

const struct Tileset gTileset_FrlgFanClubDaycare =
{
    .isCompressed = TRUE,
    .isSecondary = TRUE,
    .tiles = gTilesetTiles_FrlgFanClubDaycare,
    .palettes = gTilesetPalettes_FrlgFanClubDaycare,
    .metatiles = gMetatiles_FrlgFanClubDaycare,
    .metatileAttributes = gMetatileAttributes_FrlgFanClubDaycare,
    .callback = NULL,
};

// FRLG import end
