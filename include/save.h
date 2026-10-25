#ifndef GUARD_SAVE_H
#define GUARD_SAVE_H

#include "main.h"

// Each 4 KiB flash sector contains 3968 bytes of actual data followed by a 128 byte footer.
// Only 16 bytes of the footer are used.
#define SECTOR_DATA_SIZE 3968
#define SECTOR_FOOTER_SIZE 128
#define SECTOR_SIZE (SECTOR_DATA_SIZE + SECTOR_FOOTER_SIZE)

#define NUM_SAVE_SLOTS 2

// If the sector's signature field is not this value then the sector is either invalid or empty.
#define SECTOR_SIGNATURE 0x8012025

#define SPECIAL_SECTOR_SENTINEL 0xB39D

#define SECTOR_ID_SAVEBLOCK2          0
#define SECTOR_ID_SAVEBLOCK1_START    1
#define SECTOR_ID_SAVEBLOCK1_END      4
#define SECTOR_ID_PKMN_STORAGE_START  5
#define SECTOR_ID_PKMN_STORAGE_END   12
#define NUM_SECTORS_PER_SLOT         13
// Save Slot 1: 0-12;  Save Slot 2: 13-25
// The PC is 12 boxes (28,924 B), which fits in 8 sectors, so each slot is
// 13 sectors instead of Emerald's 14. That frees the last two slot sectors.
#define SECTOR_ID_FREE_1             26
#define SECTOR_ID_FREE_2             27
// The copy-on-write save (SAVE_COPY_ON_WRITE) uses slot 1 and 2 as the A and B
// homes of the 13 sectors and these two as its commit sectors
#define SECTOR_ID_SAVE_COMMIT_0      SECTOR_ID_FREE_1
#define SECTOR_ID_SAVE_COMMIT_1      SECTOR_ID_FREE_2
#define SECTOR_ID_HOF_1              28
#define SECTOR_ID_HOF_2              29
// Trainer Hill (FREE_TRAINER_HILL) and the battle recorder (FREE_RECORDED_BATTLE)
// are gone, so nothing writes these two sectors either; TryWriteSpecialSaveSector
// refuses them while those defines are set. Nothing erases sectors 26, 27, 30
// or 31: they are free for future Minamoto data (4 sectors, 16 KB; 30 and 31
// only, 8 KB, with SAVE_COPY_ON_WRITE). They can still hold whatever an older
// build left there, so erase one before using it.
#define SECTOR_ID_TRAINER_HILL       30
#define SECTOR_ID_RECORDED_BATTLE    31
#define SECTOR_ID_FREE_3             SECTOR_ID_TRAINER_HILL
#define SECTOR_ID_FREE_4             SECTOR_ID_RECORDED_BATTLE
#define SECTORS_COUNT                32

#define NUM_HOF_SECTORS 2

// Version of the save layout, stored in every sector footer and at the start
// of SaveBlock2. Bump it whenever a change moves or resizes anything in
// SaveBlock1, SaveBlock2 or PokemonStorage (the layout hash in save.c catches
// size changes that forget to), and add an entry to sSaveMigrations in
// src/save.c if saves of the old version should keep loading.
// Version 0 is every save written before versioning (Emerald's footer).
#define SAVE_FORMAT_VERSION 1

#define SAVE_STATUS_EMPTY    0
#define SAVE_STATUS_OK       1
#define SAVE_STATUS_CORRUPT  2
#define SAVE_STATUS_INCOMPATIBLE 3 // a save of another version that can't be migrated; left on the flash
#define SAVE_STATUS_NO_FLASH 4
#define SAVE_STATUS_ERROR    0xFF

// Special sector id value for certain save functions to
// indicate that no specific sector should be used.
#define FULL_SAVE_SLOT 0xFFFF

// SetDamagedSectorBits states
enum
{
    ENABLE,
    DISABLE,
    CHECK // unused
};

// Do save types
enum
{
    SAVE_NORMAL,
    SAVE_LINK, // Link / Battle Frontier
    SAVE_EREADER, // deprecated in Emerald
    SAVE_HALL_OF_FAME,
    SAVE_OVERWRITE_DIFFERENT_FILE,
    SAVE_HALL_OF_FAME_ERASE_BEFORE // unused
};

// A save sector location holds a pointer to the data for a particular sector
// and the size of that data. Size cannot be greater than SECTOR_DATA_SIZE.
struct SaveSectorLocation
{
    void *data;
    u16 size;
};

struct SaveSector
{
    u8 data[SECTOR_DATA_SIZE];
    u8 unused[SECTOR_FOOTER_SIZE - 16]; // Unused portion of the footer
    u16 formatVersion; // SAVE_FORMAT_VERSION; 0 in saves from before versioning
    u16 layoutHash;    // hash of the save block sizes and the sector layout
    u16 id;
    u16 checksum;      // covers the data, formatVersion and layoutHash
    u32 signature;
    u32 counter;
}; // size is SECTOR_SIZE (0x1000)

#define SECTOR_SIGNATURE_OFFSET offsetof(struct SaveSector, signature)
#define SECTOR_COUNTER_OFFSET   offsetof(struct SaveSector, counter)

extern u16 gLastWrittenSector;
extern u32 gLastSaveCounter;
extern u16 gLastKnownGoodSector;
extern u32 gDamagedSaveSectors;
extern u32 gSaveCounter;
extern struct SaveSector *gFastSaveSector;
extern u16 gIncrementalSectorId;
extern u16 gSaveFileStatus;
extern MainCallback gGameContinueCallback;
extern struct SaveSectorLocation gRamSaveSectorLocations[];

extern struct SaveSector gSaveDataBuffer;

void ClearSaveData(void);
void Save_ResetSaveCounters(void);
u8 HandleSavingData(u8 saveType);
u8 TrySavingData(u8 saveType);
bool8 LinkFullSave_Init(void);
bool8 LinkFullSave_WriteSector(void);
bool8 LinkFullSave_ReplaceLastSector(void);
bool8 LinkFullSave_SetLastSectorSignature(void);
bool8 WriteSaveBlock2(void);
bool8 WriteSaveBlock1Sector(void);
// SAVE_NORMAL may use gHeap as scratch space to migrate an old save, so it
// must only run while nothing is allocated (its callers InitHeap right after).
u8 LoadGameSave(u8 saveType);
u32 TryWriteSpecialSaveSector(u8 sector, u8 *src);
void Task_LinkFullSave(u8 taskId);

// save_failed_screen.c
void DoSaveFailedScreen(u8 saveType);

#endif // GUARD_SAVE_H
