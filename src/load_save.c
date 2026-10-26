#include "global.h"
#include "malloc.h"
#include "berry_powder.h"
#include "item.h"
#include "load_save.h"
#include "regress.h"
#include "main.h"
#include "overworld.h"
#include "pokemon.h"
#include "pokemon_storage_system.h"
#include "random.h"
#include "save_location.h"
#include "gba/flash_internal.h"
#include "decoration_inventory.h"
#include "agb_flash.h"

static void ApplyNewEncryptionKeyToAllEncryptedData(u32 encryptionKey);

// EWRAM DATA
EWRAM_DATA struct SaveBlock2 gSaveblock2 = {0};
EWRAM_DATA struct SaveBlock1 gSaveblock1 = {0};
EWRAM_DATA struct PokemonStorage gPokemonStorage = {0};

// IWRAM common
COMMON_DATA bool32 gFlashMemoryPresent = 0;
COMMON_DATA struct SaveBlock1 *gSaveBlock1Ptr = NULL;
COMMON_DATA struct SaveBlock2 *gSaveBlock2Ptr = NULL;
COMMON_DATA struct PokemonStorage *gPokemonStoragePtr = NULL;

// code
void CheckForFlashMemory(void)
{
    if (!IdentifyFlash())
    {
        gFlashMemoryPresent = TRUE;
        InitFlashTimer();
    }
    else
    {
        gFlashMemoryPresent = FALSE;
    }
}

void ClearSav2(void)
{
    CpuFill16(0, &gSaveblock2, sizeof(struct SaveBlock2));
}

void ClearSav1(void)
{
    CpuFill16(0, &gSaveblock1, sizeof(struct SaveBlock1));
}

void SetSaveBlocksPointers(void)
{
    gSaveBlock2Ptr = &gSaveblock2;
    gSaveBlock1Ptr = &gSaveblock1;
    gPokemonStoragePtr = &gPokemonStorage;

    SetBagItemsPointers();
    SetDecorationInventoriesPointers();
}

// Called when a battle starts or ends and when the overworld reloads. The save
// blocks no longer move, so this only resets the heap and re-keys the
// encrypted values (money, item counts and the like).
void MoveSaveBlocks_ResetHeap(void)
{
    u32 encryptionKey;

    InitHeap(gHeap, HEAP_SIZE);

    // create a new encryption key
    encryptionKey = (Random() << 16) + (Random());
    ApplyNewEncryptionKeyToAllEncryptedData(encryptionKey);
    gSaveBlock2Ptr->encryptionKey = encryptionKey;
    REGRESS_RESEED();
}

u32 UseContinueGameWarp(void)
{
    return gSaveBlock2Ptr->specialSaveWarpFlags & CONTINUE_GAME_WARP;
}

void ClearContinueGameWarpStatus(void)
{
    gSaveBlock2Ptr->specialSaveWarpFlags &= ~CONTINUE_GAME_WARP;
}

void SetContinueGameWarpStatus(void)
{
    gSaveBlock2Ptr->specialSaveWarpFlags |= CONTINUE_GAME_WARP;
}

void SetContinueGameWarpStatusToDynamicWarp(void)
{
    SetContinueGameWarpToDynamicWarp(0);
    gSaveBlock2Ptr->specialSaveWarpFlags |= CONTINUE_GAME_WARP;
}

void ClearContinueGameWarpStatus2(void)
{
    gSaveBlock2Ptr->specialSaveWarpFlags &= ~CONTINUE_GAME_WARP;
}

void SavePlayerParty(void)
{
    int i;

    gSaveBlock1Ptr->playerPartyCount = gPlayerPartyCount;

    for (i = 0; i < PARTY_SIZE; i++)
        gSaveBlock1Ptr->playerParty[i] = gPlayerParty[i];
}

void LoadPlayerParty(void)
{
    int i;

    gPlayerPartyCount = gSaveBlock1Ptr->playerPartyCount;

    for (i = 0; i < PARTY_SIZE; i++)
        gPlayerParty[i] = gSaveBlock1Ptr->playerParty[i];
}

void SaveObjectEvents(void)
{
    int i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        gSaveBlock1Ptr->objectEvents[i] = gObjectEvents[i];
}

void LoadObjectEvents(void)
{
    int i;

    for (i = 0; i < OBJECT_EVENTS_COUNT; i++)
        gObjectEvents[i] = gSaveBlock1Ptr->objectEvents[i];
}

void CopyPartyAndObjectsToSave(void)
{
    SavePlayerParty();
    SaveObjectEvents();
}

void CopyPartyAndObjectsFromSave(void)
{
    LoadPlayerParty();
    LoadObjectEvents();
}

void ApplyNewEncryptionKeyToHword(u16 *hWord, u32 newKey)
{
    *hWord ^= gSaveBlock2Ptr->encryptionKey;
    *hWord ^= newKey;
}

void ApplyNewEncryptionKeyToWord(u32 *word, u32 newKey)
{
    *word ^= gSaveBlock2Ptr->encryptionKey;
    *word ^= newKey;
}

static void ApplyNewEncryptionKeyToAllEncryptedData(u32 encryptionKey)
{
    ApplyNewEncryptionKeyToGameStats(encryptionKey);
    ApplyNewEncryptionKeyToBagItems_(encryptionKey);
    ApplyNewEncryptionKeyToBerryPowder(encryptionKey);
    ApplyNewEncryptionKeyToWord(&gSaveBlock1Ptr->money, encryptionKey);
    ApplyNewEncryptionKeyToHword(&gSaveBlock1Ptr->coins, encryptionKey);
}
