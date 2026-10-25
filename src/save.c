#include "global.h"
#include "agb_flash.h"
#include "gba/flash_internal.h"
#include "fieldmap.h"
#include "save.h"
#include "task.h"
#include "decompress.h"
#include "load_save.h"
#include "overworld.h"
#include "pokemon_storage_system.h"
#include "link.h"
#include "malloc.h"
#include "regress.h"
#include "constants/game_stat.h"

static u16 CalculateChecksum(void *, u16);
static u16 CalculateSectorChecksum(void *, u16, u16, u16);
static bool8 ReadFlashSector(u8, struct SaveSector *);
static u8 TryWriteSector(u8, u8 *);
static void FillSaveSector(u16, const struct SaveSectorLocation *, u32);
static u8 RunSaveMigrations(u8 *, u16, struct SaveSectorLocation *);
#ifndef SAVE_COPY_ON_WRITE
static u8 GetSaveValidStatus(const struct SaveSectorLocation *);
static u8 CopySaveSlotData(u16, struct SaveSectorLocation *);
static bool32 IsCurrentFormatSector(const struct SaveSector *);
static u8 TryMigrateSave(struct SaveSectorLocation *);
static u8 HandleWriteSector(u16, const struct SaveSectorLocation *);
static u8 HandleReplaceSector(u16, const struct SaveSectorLocation *);
#endif

// Divide save blocks into individual chunks to be written to flash sectors

/*
 * Sector Layout:
 *
 * Sectors 0 - 12:      Save Slot 1 (copy-on-write save: home A of sectors 0-12)
 * Sectors 13 - 25:     Save Slot 2 (copy-on-write save: home B of sectors 0-12)
 * Sectors 26 - 27:     Free (Emerald's 14th sector of each slot)
 *                      (copy-on-write save: the two commit sectors)
 * Sectors 28 - 29:     Hall of Fame
 * Sector 30:           Free (was Trainer Hill)
 * Sector 31:           Free (was Recorded Battle)
 *
 * There are two save slots for saving the player's game data. We alternate between
 * them each time the game is saved, so that if the current save slot is corrupt,
 * we can load the previous one. We also rotate the sectors in each save slot
 * so that the same data is not always being written to the same sector. This
 * might be done to reduce wear on the flash memory, but I'm not sure, since all
 * 13 sectors get written anyway. The rotation is also what makes a save cut
 * off by a power loss fall back to the other slot, see below.
 *
 * See SECTOR_ID_* constants in save.h
 *
 * That is Emerald's scheme. With SAVE_COPY_ON_WRITE (include/config.h) the
 * game data is instead written copy-on-write, see the comment above
 * WriteSaveData further down.
 */

#define SAVEBLOCK_CHUNK(structure, chunkNum)                                   \
{                                                                              \
    chunkNum * SECTOR_DATA_SIZE,                                               \
    sizeof(structure) >= chunkNum * SECTOR_DATA_SIZE ?                         \
    min(sizeof(structure) - chunkNum * SECTOR_DATA_SIZE, SECTOR_DATA_SIZE) : 0 \
}

struct
{
    u16 offset;
    u16 size;
} static const sSaveSlotLayout[NUM_SECTORS_PER_SLOT] =
{
    SAVEBLOCK_CHUNK(struct SaveBlock2, 0), // SECTOR_ID_SAVEBLOCK2

    SAVEBLOCK_CHUNK(struct SaveBlock1, 0), // SECTOR_ID_SAVEBLOCK1_START
    SAVEBLOCK_CHUNK(struct SaveBlock1, 1),
    SAVEBLOCK_CHUNK(struct SaveBlock1, 2),
    SAVEBLOCK_CHUNK(struct SaveBlock1, 3), // SECTOR_ID_SAVEBLOCK1_END

    SAVEBLOCK_CHUNK(struct PokemonStorage, 0), // SECTOR_ID_PKMN_STORAGE_START
    SAVEBLOCK_CHUNK(struct PokemonStorage, 1),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 2),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 3),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 4),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 5),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 6),
    SAVEBLOCK_CHUNK(struct PokemonStorage, 7), // SECTOR_ID_PKMN_STORAGE_END
};

// These will produce an error if a save struct is larger than the space
// alloted for it in the flash.
STATIC_ASSERT(sizeof(struct SaveBlock2) <= SECTOR_DATA_SIZE, SaveBlock2FreeSpace);
STATIC_ASSERT(sizeof(struct SaveBlock1) <= SECTOR_DATA_SIZE * (SECTOR_ID_SAVEBLOCK1_END - SECTOR_ID_SAVEBLOCK1_START + 1), SaveBlock1FreeSpace);
STATIC_ASSERT(sizeof(struct PokemonStorage) <= SECTOR_DATA_SIZE * (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START + 1), PokemonStorageFreeSpace);
// Every slot sector holds data: the PC needs all of its sectors, so none is written empty.
STATIC_ASSERT(sizeof(struct PokemonStorage) > SECTOR_DATA_SIZE * (SECTOR_ID_PKMN_STORAGE_END - SECTOR_ID_PKMN_STORAGE_START), PokemonStorageNoEmptySector);
STATIC_ASSERT(SECTOR_ID_PKMN_STORAGE_END + 1 == NUM_SECTORS_PER_SLOT, SaveSlotLayoutComplete);
STATIC_ASSERT(NUM_SECTORS_PER_SLOT * NUM_SAVE_SLOTS <= SECTOR_ID_FREE_1 && SECTOR_ID_FREE_2 < SECTOR_ID_HOF_1, SaveSlotsFitBeforeHallOfFame);

// Why a save cut off by a power loss falls back to the other slot:
// a save writes every sector of one slot, sector id i to physical sector
// (i + gLastWrittenSector) % NUM_SECTORS_PER_SLOT, and gLastWrittenSector goes
// up by one per save. Saves alternate slots, so between two writes of the same
// slot the rotation moves by NUM_SAVE_SLOTS (2). A save cut after k sectors
// (0 < k < NUM_SECTORS_PER_SLOT) has replaced ids 0..k-1 at the physical sectors
// that held ids 2..k+1 of the older save, so ids k and k+1 (mod the slot size)
// are in neither and the slot fails the "all ids present" check in
// GetSaveValidStatus. That only holds while the rotation step is not a multiple
// of the slot size; with a step of 0 a cut save would leave a complete set of
// ids that mixes two saves and loads as if it were good.
STATIC_ASSERT(NUM_SAVE_SLOTS % NUM_SECTORS_PER_SLOT != 0, TornSaveLeavesSectorIdMissing);

// Every slot chunk is a whole number of words: the checksum adds up words, and
// a migration checks old sectors over all of their data (see TryMigrateSave).
STATIC_ASSERT(sizeof(struct SaveBlock2) % 4 == 0 && sizeof(struct SaveBlock1) % 4 == 0
           && sizeof(struct PokemonStorage) % 4 == 0 && SECTOR_DATA_SIZE % 4 == 0, SaveChunksAreWords);
STATIC_ASSERT(offsetof(struct SaveBlock2, saveFormatVersion) == 0x16, SaveBlock2VersionInPadding);

// The layout hash goes into every sector footer next to SAVE_FORMAT_VERSION.
// It changes when any save block changes size or the sectors are laid out
// differently, so a build that changed the layout without bumping the version
// still refuses saves it would misread. It is never 0 (Emerald's footer).
#ifdef SAVE_COPY_ON_WRITE
#define SAVE_SCHEME 1 // the same data in another place: the schemes can't read each other's saves
#else
#define SAVE_SCHEME 0
#endif
#define LAYOUT_HASH_STEP(h, x) ((h) * 33u + (u32)(x))
#define SAVE_LAYOUT_HASH_32                                                        \
    LAYOUT_HASH_STEP(LAYOUT_HASH_STEP(LAYOUT_HASH_STEP(LAYOUT_HASH_STEP(LAYOUT_HASH_STEP( \
    LAYOUT_HASH_STEP(LAYOUT_HASH_STEP(LAYOUT_HASH_STEP(LAYOUT_HASH_STEP(5381u,     \
        SECTOR_DATA_SIZE), NUM_SECTORS_PER_SLOT), SECTOR_ID_SAVEBLOCK2),           \
        SECTOR_ID_SAVEBLOCK1_START), SECTOR_ID_PKMN_STORAGE_START),                \
        sizeof(struct SaveBlock2)), sizeof(struct SaveBlock1)), sizeof(struct PokemonStorage)), SAVE_SCHEME)
#define SAVE_LAYOUT_HASH ((u16)(SAVE_LAYOUT_HASH_32 % 0xFFFFu + 1))

// Migrations turn the save image of one version into the next. The image is
// the data of every slot sector in id order (sector i at i * SECTOR_DATA_SIZE,
// zero padded), and a migration rewrites it in place into the next version's
// layout. LoadGameSave runs them one after another from the save's version up
// to SAVE_FORMAT_VERSION. Only the sector contents can change this way: the
// way sectors sit on the flash (NUM_SECTORS_PER_SLOT, the two slots or the
// homes and commit sectors) must stay the same across versions, or loading
// old saves needs more than a migration.
typedef bool8 (*SaveMigrationFunc)(u8 *image);

struct SaveMigration
{
    u16 fromVersion;
    u16 fromLayoutHash; // the layout hash saves of fromVersion carry
    SaveMigrationFunc migrate;
};

static const struct SaveMigration sSaveMigrations[] =
{
    // Version 1 is the first versioned layout, so there is nothing to migrate yet.
    // { .fromVersion = 1, .fromLayoutHash = 0x...., .migrate = MigrateSave_1To2 },
    { .fromVersion = 0, .fromLayoutHash = 0, .migrate = NULL },
};

#define SAVE_MIGRATION_IMAGE_SIZE (NUM_SECTORS_PER_SLOT * SECTOR_DATA_SIZE)
STATIC_ASSERT(SAVE_MIGRATION_IMAGE_SIZE <= HEAP_SIZE, SaveMigrationImageFitsInHeap);

COMMON_DATA u16 gLastWrittenSector = 0;
COMMON_DATA u32 gLastSaveCounter = 0;
COMMON_DATA u16 gLastKnownGoodSector = 0;
COMMON_DATA u32 gDamagedSaveSectors = 0;
COMMON_DATA u32 gSaveCounter = 0;
COMMON_DATA struct SaveSector *gReadWriteSector = NULL; // Pointer to a buffer for reading/writing a sector
COMMON_DATA u16 gIncrementalSectorId = 0;
COMMON_DATA u16 gSaveUnusedVar = 0;
COMMON_DATA u16 gSaveFileStatus = 0;
COMMON_DATA MainCallback gGameContinueCallback = NULL;
COMMON_DATA struct SaveSectorLocation gRamSaveSectorLocations[NUM_SECTORS_PER_SLOT] = {0};
COMMON_DATA u16 gSaveUnusedVar2 = 0;
COMMON_DATA u16 gSaveAttemptStatus = 0;

EWRAM_DATA struct SaveSector gSaveDataBuffer = {0}; // Buffer used for reading/writing sectors
#ifndef SAVE_COPY_ON_WRITE
// The newest intact sector of another version the last load found: its
// version, layout hash and save counter
EWRAM_DATA static u16 sForeignSaveVersion = 0;
EWRAM_DATA static u16 sForeignSaveLayoutHash = 0;
EWRAM_DATA static u32 sForeignSaveCounter = 0;
EWRAM_DATA static bool8 sForeignSaveFound = FALSE;
#endif

#ifdef SAVE_COPY_ON_WRITE
static void CowForgetSave(void);
#endif

void ClearSaveData(void)
{
    u16 i;

    // Clear the full save two sectors at a time
    for (i = 0; i < SECTORS_COUNT / 2; i++)
    {
        EraseFlashSector(i);
        EraseFlashSector(i + SECTORS_COUNT / 2);
    }
#ifdef SAVE_COPY_ON_WRITE
    CowForgetSave();
#endif
}

void Save_ResetSaveCounters(void)
{
    gSaveCounter = 0;
    gLastWrittenSector = 0;
    gDamagedSaveSectors = 0;
#ifdef SAVE_COPY_ON_WRITE
    CowForgetSave();
#endif
}

static bool32 SetDamagedSectorBits(u8 op, u8 sectorId)
{
    bool32 retVal = FALSE;

    switch (op)
    {
    case ENABLE:
        gDamagedSaveSectors |= (1 << sectorId);
        break;
    case DISABLE:
        gDamagedSaveSectors &= ~(1 << sectorId);
        break;
    case CHECK: // unused
        if (gDamagedSaveSectors & (1 << sectorId))
            retVal = TRUE;
        break;
    }

    return retVal;
}

// Fills the sector buffer with sector sectorId of the save blocks and its footer
static void FillSaveSector(u16 sectorId, const struct SaveSectorLocation *locations, u32 counter)
{
    u16 i;
    u8 *data = locations[sectorId].data;
    u16 size = locations[sectorId].size;

    // Clear temp save sector
    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)gReadWriteSector)[i] = 0;

    // Set footer data
    gReadWriteSector->id = sectorId;
    gReadWriteSector->signature = SECTOR_SIGNATURE;
    gReadWriteSector->counter = counter;
    gReadWriteSector->formatVersion = SAVE_FORMAT_VERSION;
    gReadWriteSector->layoutHash = SAVE_LAYOUT_HASH;

    // Copy current data to temp buffer for writing
    for (i = 0; i < size; i++)
        gReadWriteSector->data[i] = data[i];

    gReadWriteSector->checksum = CalculateSectorChecksum(data, size, SAVE_FORMAT_VERSION, SAVE_LAYOUT_HASH);
}

#ifndef SAVE_COPY_ON_WRITE
static u8 WriteSaveSectorOrSlot(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u32 status;
    u16 i;

    gReadWriteSector = &gSaveDataBuffer;

    if (sectorId != FULL_SAVE_SLOT)
    {
        // A sector was specified, just write that sector.
        // This is never reached, FULL_SAVE_SLOT is always used instead.
        status = HandleWriteSector(sectorId, locations);
    }
    else
    {
        // No sector was specified, write full save slot.
        gLastKnownGoodSector = gLastWrittenSector; // backup the current written sector before attempting to write.
        gLastSaveCounter = gSaveCounter;
        gLastWrittenSector++;
        gLastWrittenSector = gLastWrittenSector % NUM_SECTORS_PER_SLOT;
        gSaveCounter++;
        status = SAVE_STATUS_OK;

        for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
            HandleWriteSector(i, locations);

        if (gDamagedSaveSectors)
        {
            // At least one sector save failed
            status = SAVE_STATUS_ERROR;
            gLastWrittenSector = gLastKnownGoodSector;
            gSaveCounter = gLastSaveCounter;
        }
    }

    return status;
}

static u8 HandleWriteSector(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 sector;

    // Adjust sector id for current save slot
    sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    FillSaveSector(sectorId, locations, gSaveCounter);
    return TryWriteSector(sector, gReadWriteSector->data);
}

static u8 WriteSaveData(void)
{
    return WriteSaveSectorOrSlot(FULL_SAVE_SLOT, gRamSaveSectorLocations);
}
#endif // !SAVE_COPY_ON_WRITE

static u8 HandleWriteSectorNBytes(u8 sectorId, u8 *data, u16 size)
{
    u16 i;
    struct SaveSector *sector = &gSaveDataBuffer;

    // Clear temp save sector
    for (i = 0; i < SECTOR_SIZE; i++)
        ((u8 *)sector)[i] = 0;

    sector->signature = SECTOR_SIGNATURE;

    // Copy data to temp buffer for writing
    for (i = 0; i < size; i++)
        sector->data[i] = data[i];

    sector->id = CalculateChecksum(data, size); // though this appears to be incorrect, it might be some sector checksum instead of a whole save checksum and only appears to be relevent to HOF data, if used.
    return TryWriteSector(sectorId, sector->data);
}

static u8 TryWriteSector(u8 sector, u8 *data)
{
    REGRESS_SAVE_FLASH_OP(sector, data, SECTOR_SIZE, TRUE);
    if (ProgramFlashSectorAndVerify(sector, data)) // is damaged?
    {
        // Failed
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

#ifndef SAVE_COPY_ON_WRITE
static u32 RestoreSaveBackupVarsAndIncrement(const struct SaveSectorLocation *locations)
{
    gReadWriteSector = &gSaveDataBuffer;
    gLastKnownGoodSector = gLastWrittenSector;
    gLastSaveCounter = gSaveCounter;
    gLastWrittenSector++;
    gLastWrittenSector %= NUM_SECTORS_PER_SLOT;
    gSaveCounter++;
    gIncrementalSectorId = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u32 RestoreSaveBackupVars(const struct SaveSectorLocation *locations)
{
    gReadWriteSector = &gSaveDataBuffer;
    gLastKnownGoodSector = gLastWrittenSector;
    gLastSaveCounter = gSaveCounter;
    gIncrementalSectorId = 0;
    gDamagedSaveSectors = 0;
    return 0;
}

static u8 HandleWriteIncrementalSector(u16 numSectors, const struct SaveSectorLocation *locations)
{
    u8 status;

    if (gIncrementalSectorId < numSectors - 1)
    {
        status = SAVE_STATUS_OK;
        HandleWriteSector(gIncrementalSectorId, locations);
        gIncrementalSectorId++;
        if (gDamagedSaveSectors)
        {
            status = SAVE_STATUS_ERROR;
            gLastWrittenSector = gLastKnownGoodSector;
            gSaveCounter = gLastSaveCounter;
        }
    }
    else
    {
        // Exceeded max sector, finished
        status = SAVE_STATUS_ERROR;
    }

    return status;
}

static u8 HandleReplaceSectorAndVerify(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u8 status = SAVE_STATUS_OK;

    HandleReplaceSector(sectorId - 1, locations);

    if (gDamagedSaveSectors)
    {
        status = SAVE_STATUS_ERROR;
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
    }
    return status;
}

// Similar to HandleWriteSector, but fully erases the sector first, and skips writing the first signature byte
static u8 HandleReplaceSector(u16 sectorId, const struct SaveSectorLocation *locations)
{
    u16 i;
    u16 sector;
    u8 status;

    // Adjust sector id for current save slot
    sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    FillSaveSector(sectorId, locations, gSaveCounter);

    // Erase old save data
    EraseFlashSector(sector);

    status = SAVE_STATUS_OK;

    // Write new save data up to signature field
    for (i = 0; i < SECTOR_SIGNATURE_OFFSET; i++)
    {
        if (ProgramFlashByte(sector, i, ((u8 *)gReadWriteSector)[i]))
        {
            status = SAVE_STATUS_ERROR;
            break;
        }
    }

    if (status == SAVE_STATUS_ERROR)
    {
        // Writing save data failed
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Writing save data succeeded, write signature and counter
        status = SAVE_STATUS_OK;

        // Write signature (skipping the first byte) and counter fields.
        // The byte of signature that is skipped is instead written by WriteSectorSignatureByte or WriteSectorSignatureByte_NoOffset
        for (i = 0; i < SECTOR_SIZE - (SECTOR_SIGNATURE_OFFSET + 1); i++)
        {
            if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET + 1 + i, ((u8 *)gReadWriteSector)[SECTOR_SIGNATURE_OFFSET + 1 + i]))
            {
                status = SAVE_STATUS_ERROR;
                break;
            }
        }

        if (status == SAVE_STATUS_ERROR)
        {
            // Writing signature/counter failed
            SetDamagedSectorBits(ENABLE, sector);
            return SAVE_STATUS_ERROR;
        }
        else
        {
            // Succeeded
            SetDamagedSectorBits(DISABLE, sector);
            return SAVE_STATUS_OK;
        }
    }
}

static u8 WriteSectorSignatureByte_NoOffset(u16 sectorId, const struct SaveSectorLocation *locations)
{
    // Adjust sector id for current save slot
    // This first line lacking -1 is the only difference from WriteSectorSignatureByte
    u16 sector = sectorId + gLastWrittenSector;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Write just the first byte of the signature field, which was skipped by HandleReplaceSector
    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, SECTOR_SIGNATURE & 0xFF))
    {
        // Sector is damaged, so enable the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 CopySectorSignatureByte(u16 sectorId, const struct SaveSectorLocation *locations)
{
    // Adjust sector id for current save slot
    u16 sector = sectorId + gLastWrittenSector - 1;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Copy just the first byte of the signature field from the read/write buffer
    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, ((u8 *)gReadWriteSector)[SECTOR_SIGNATURE_OFFSET]))
    {
        // Sector is damaged, so enable the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 WriteSectorSignatureByte(u16 sectorId, const struct SaveSectorLocation *locations)
{
    // Adjust sector id for current save slot
    u16 sector = sectorId + gLastWrittenSector - 1;
    sector %= NUM_SECTORS_PER_SLOT;
    sector += NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);

    // Write just the first byte of the signature field, which was skipped by HandleReplaceSector
    if (ProgramFlashByte(sector, SECTOR_SIGNATURE_OFFSET, SECTOR_SIGNATURE & 0xFF))
    {
        // Sector is damaged, so enable the bit in gDamagedSaveSectors and restore the last written sector and save counter.
        SetDamagedSectorBits(ENABLE, sector);
        gLastWrittenSector = gLastKnownGoodSector;
        gSaveCounter = gLastSaveCounter;
        return SAVE_STATUS_ERROR;
    }
    else
    {
        // Succeeded
        SetDamagedSectorBits(DISABLE, sector);
        return SAVE_STATUS_OK;
    }
}

static u8 TryLoadSaveSlot(u16 sectorId, struct SaveSectorLocation *locations)
{
    u8 status;
    gReadWriteSector = &gSaveDataBuffer;
    if (sectorId != FULL_SAVE_SLOT)
    {
        // This function may not be used with a specific sector id
        status = SAVE_STATUS_ERROR;
    }
    else
    {
        status = GetSaveValidStatus(locations);
        if (status == SAVE_STATUS_INCOMPATIBLE)
            return TryMigrateSave(locations); // leaves the save blocks alone if it can't
        CopySaveSlotData(FULL_SAVE_SLOT, locations);
    }

    return status;
}

// sectorId arg is ignored, this always reads the full save slot
static u8 CopySaveSlotData(u16 sectorId, struct SaveSectorLocation *locations)
{
    u16 i;
    u16 checksum;
    u16 slotOffset = NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);
    u16 id;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + slotOffset, gReadWriteSector);

        id = gReadWriteSector->id;
        if (!IsCurrentFormatSector(gReadWriteSector)) // e.g. Emerald's 14th sector
            continue;
        if (id == 0)
            gLastWrittenSector = i;

        checksum = CalculateSectorChecksum(gReadWriteSector->data, locations[id].size, SAVE_FORMAT_VERSION, SAVE_LAYOUT_HASH);

        // Only copy data for sectors whose signature and checksum fields are correct
        if (gReadWriteSector->signature == SECTOR_SIGNATURE && gReadWriteSector->checksum == checksum)
        {
            u16 j;
            for (j = 0; j < locations[id].size; j++)
                ((u8 *)locations[id].data)[j] = gReadWriteSector->data[j];
        }
    }

    return SAVE_STATUS_OK;
}

static bool32 IsCurrentFormatSector(const struct SaveSector *sector)
{
    return sector->formatVersion == SAVE_FORMAT_VERSION
        && sector->layoutHash == SAVE_LAYOUT_HASH
        && sector->id < NUM_SECTORS_PER_SLOT;
}

// Checks one save slot for a complete save of the given version. Sectors of
// any other version count as foreign: their version goes into
// sForeignSaveVersion/LayoutHash and they don't make the slot invalid.
// Sectors of the current version are checked over their chunk size, like
// Emerald; other versions over all their data, which is the same sum since
// the rest of a sector is zero (SaveChunksAreWords).
static u8 GetSaveSlotStatus(u16 slotOffset, u16 version, u16 layoutHash, const struct SaveSectorLocation *locations, u32 *counter)
{
    u16 i;
    u16 size;
    u32 validSectorFlags = 0;
    bool8 signatureValid = FALSE;
    bool8 foreign = FALSE;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + slotOffset, gReadWriteSector);
        if (gReadWriteSector->signature != SECTOR_SIGNATURE)
            continue;

        signatureValid = TRUE;
        if (gReadWriteSector->formatVersion != version
         || gReadWriteSector->layoutHash != layoutHash
         || gReadWriteSector->id >= NUM_SECTORS_PER_SLOT)
        {
            foreign = TRUE;
            if ((gReadWriteSector->formatVersion != SAVE_FORMAT_VERSION || gReadWriteSector->layoutHash != SAVE_LAYOUT_HASH)
             && gReadWriteSector->checksum == CalculateSectorChecksum(gReadWriteSector->data, SECTOR_DATA_SIZE,
                                                gReadWriteSector->formatVersion, gReadWriteSector->layoutHash)
             && (!sForeignSaveFound || gReadWriteSector->counter > sForeignSaveCounter))
            {
                sForeignSaveFound = TRUE;
                sForeignSaveVersion = gReadWriteSector->formatVersion;
                sForeignSaveLayoutHash = gReadWriteSector->layoutHash;
                sForeignSaveCounter = gReadWriteSector->counter;
            }
            continue;
        }

        size = (version == SAVE_FORMAT_VERSION) ? locations[gReadWriteSector->id].size : SECTOR_DATA_SIZE;
        if (gReadWriteSector->checksum == CalculateSectorChecksum(gReadWriteSector->data, size, version, layoutHash))
        {
            *counter = gReadWriteSector->counter;
            validSectorFlags |= 1 << gReadWriteSector->id;
        }
    }

    if (!signatureValid)
        return SAVE_STATUS_EMPTY; // No sectors in the slot have the correct signature, treat it as empty
    if (validSectorFlags == (1 << NUM_SECTORS_PER_SLOT) - 1)
        return SAVE_STATUS_OK;
    if (foreign && validSectorFlags == 0)
        return SAVE_STATUS_INCOMPATIBLE; // only sectors of another version
    return SAVE_STATUS_ERROR;
}

static u8 GetSaveValidStatus(const struct SaveSectorLocation *locations)
{
    u32 saveSlot1Counter = 0;
    u32 saveSlot2Counter = 0;
    u8 saveSlot1Status;
    u8 saveSlot2Status;

    sForeignSaveVersion = 0;
    sForeignSaveLayoutHash = 0;
    sForeignSaveCounter = 0;
    sForeignSaveFound = FALSE;
    saveSlot1Status = GetSaveSlotStatus(0, SAVE_FORMAT_VERSION, SAVE_LAYOUT_HASH, locations, &saveSlot1Counter);
    saveSlot2Status = GetSaveSlotStatus(NUM_SECTORS_PER_SLOT, SAVE_FORMAT_VERSION, SAVE_LAYOUT_HASH, locations, &saveSlot2Counter);

    if (((saveSlot1Status == SAVE_STATUS_INCOMPATIBLE || saveSlot2Status == SAVE_STATUS_INCOMPATIBLE)
      && (saveSlot1Status == SAVE_STATUS_INCOMPATIBLE || saveSlot1Status == SAVE_STATUS_EMPTY)
      && (saveSlot2Status == SAVE_STATUS_INCOMPATIBLE || saveSlot2Status == SAVE_STATUS_EMPTY))
     // A newer save of another version (a later build saved last): loading the
     // older slot of this version instead would quietly undo that progress
     || (sForeignSaveFound
      && (saveSlot1Status != SAVE_STATUS_OK || sForeignSaveCounter > saveSlot1Counter)
      && (saveSlot2Status != SAVE_STATUS_OK || sForeignSaveCounter > saveSlot2Counter)
      && (saveSlot1Status == SAVE_STATUS_OK || saveSlot2Status == SAVE_STATUS_OK)))
    {
        // Nothing usable but a save of another version. The next save counts
        // on from it, so it is newer than everything left on the flash.
        gSaveCounter = sForeignSaveCounter;
        gLastWrittenSector = 0;
        return SAVE_STATUS_INCOMPATIBLE;
    }

    // Sectors left from another version next to a save of this one (the
    // first save over an old file only replaces one slot) are as good as empty
    if (saveSlot1Status == SAVE_STATUS_INCOMPATIBLE)
        saveSlot1Status = SAVE_STATUS_EMPTY;
    if (saveSlot2Status == SAVE_STATUS_INCOMPATIBLE)
        saveSlot2Status = SAVE_STATUS_EMPTY;

    if (saveSlot1Status == SAVE_STATUS_OK && saveSlot2Status == SAVE_STATUS_OK)
    {
        if ((saveSlot1Counter == -1 && saveSlot2Counter ==  0)
         || (saveSlot1Counter ==  0 && saveSlot2Counter == -1))
        {
            if ((unsigned)(saveSlot1Counter + 1) < (unsigned)(saveSlot2Counter + 1))
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        else
        {
            if (saveSlot1Counter < saveSlot2Counter)
                gSaveCounter = saveSlot2Counter;
            else
                gSaveCounter = saveSlot1Counter;
        }
        return SAVE_STATUS_OK;
    }

    // One or both save slots are not OK

    if (saveSlot1Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot1Counter;
        if (saveSlot2Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR; // Slot 2 errored
        return SAVE_STATUS_OK; // Slot 1 is OK, slot 2 is empty
    }

    if (saveSlot2Status == SAVE_STATUS_OK)
    {
        gSaveCounter = saveSlot2Counter;
        if (saveSlot1Status == SAVE_STATUS_ERROR)
            return SAVE_STATUS_ERROR; // Slot 1 errored
        return SAVE_STATUS_OK; // Slot 2 is OK, slot 1 is empty
    }

    // Neither slot is OK, check if both are empty
    if (saveSlot1Status == SAVE_STATUS_EMPTY
     && saveSlot2Status == SAVE_STATUS_EMPTY)
    {
        gSaveCounter = 0;
        gLastWrittenSector = 0;
        return SAVE_STATUS_EMPTY;
    }

    // Both slots errored
    gSaveCounter = 0;
    gLastWrittenSector = 0;
    return SAVE_STATUS_CORRUPT;
}

#endif // !SAVE_COPY_ON_WRITE

static const struct SaveMigration *FindSaveMigration(u16 version)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sSaveMigrations); i++)
    {
        if (sSaveMigrations[i].migrate != NULL && sSaveMigrations[i].fromVersion == version)
            return &sSaveMigrations[i];
    }
    return NULL;
}

// Whether a save of this version and layout hash can be migrated
static bool32 CanMigrateSave(u16 version, u16 layoutHash)
{
    const struct SaveMigration *migration;
    u16 v;

    if (version == 0 || version >= SAVE_FORMAT_VERSION)
        return FALSE;
    migration = FindSaveMigration(version);
    if (migration == NULL || migration->fromLayoutHash != layoutHash)
        return FALSE;
    for (v = version + 1; v < SAVE_FORMAT_VERSION; v++)
    {
        if (FindSaveMigration(v) == NULL)
            return FALSE;
    }
    return TRUE;
}

// Migrates the image of a save of the given version (read from the flash by
// the caller) up to this version and copies it into the save blocks
static u8 RunSaveMigrations(u8 *image, u16 version, struct SaveSectorLocation *locations)
{
    u16 i, v;

    for (v = version; v < SAVE_FORMAT_VERSION; v++)
    {
        if (!FindSaveMigration(v)->migrate(image))
            return SAVE_STATUS_INCOMPATIBLE;
    }

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
        memcpy(locations[i].data, &image[i * SECTOR_DATA_SIZE], locations[i].size);
    gSaveBlock2Ptr->saveFormatVersion = SAVE_FORMAT_VERSION;
    return SAVE_STATUS_OK;
}

#ifndef SAVE_COPY_ON_WRITE
// The flash holds only a save of another version (sForeignSaveVersion). If it
// is older and there are migrations from it up to this version, load it into
// gHeap, migrate it there and copy the result into the save blocks. Otherwise
// leave everything alone and report the save as incompatible.
static u8 TryMigrateSave(struct SaveSectorLocation *locations)
{
    u16 version = sForeignSaveVersion;
    u16 layoutHash = sForeignSaveLayoutHash;
    u32 slot1Counter = 0, slot2Counter = 0;
    u8 slot1Status, slot2Status;
    u16 slotOffset, i;
    u8 *image = gHeap; // free here, see LoadGameSave

    if (!CanMigrateSave(version, layoutHash))
        return SAVE_STATUS_INCOMPATIBLE;

    slot1Status = GetSaveSlotStatus(0, version, layoutHash, locations, &slot1Counter);
    slot2Status = GetSaveSlotStatus(NUM_SECTORS_PER_SLOT, version, layoutHash, locations, &slot2Counter);
    if (slot1Status == SAVE_STATUS_OK && (slot2Status != SAVE_STATUS_OK || slot1Counter >= slot2Counter))
        gSaveCounter = slot1Counter;
    else if (slot2Status == SAVE_STATUS_OK)
        gSaveCounter = slot2Counter;
    else
        return SAVE_STATUS_INCOMPATIBLE; // the old save is damaged too

    // Read the slot into the image, sector i at i * SECTOR_DATA_SIZE
    slotOffset = NUM_SECTORS_PER_SLOT * (gSaveCounter % NUM_SAVE_SLOTS);
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(i + slotOffset, gReadWriteSector);
        if (gReadWriteSector->id >= NUM_SECTORS_PER_SLOT)
            return SAVE_STATUS_INCOMPATIBLE;
        if (gReadWriteSector->id == 0)
            gLastWrittenSector = i;
        memcpy(&image[gReadWriteSector->id * SECTOR_DATA_SIZE], gReadWriteSector->data, SECTOR_DATA_SIZE);
    }

    return RunSaveMigrations(image, version, locations);
}
#endif // !SAVE_COPY_ON_WRITE

static u8 TryLoadSaveSector(u8 sectorId, u8 *data, u16 size)
{
    u16 i;
    struct SaveSector *sector = &gSaveDataBuffer;
    ReadFlashSector(sectorId, sector);
    if (sector->signature == SECTOR_SIGNATURE)
    {
        u16 checksum = CalculateChecksum(sector->data, size);
        if (sector->id == checksum)
        {
            // Signature and checksum are correct, copy data
            for (i = 0; i < size; i++)
                data[i] = sector->data[i];
            return SAVE_STATUS_OK;
        }
        else
        {
            // Incorrect checksum
            return SAVE_STATUS_CORRUPT;
        }
    }
    else
    {
        // Incorrect signature value
        return SAVE_STATUS_EMPTY;
    }
}

// Return value always ignored
static bool8 ReadFlashSector(u8 sectorId, struct SaveSector *sector)
{
    ReadFlash(sectorId, 0, sector->data, SECTOR_SIZE);
    return TRUE;
}

static u16 CalculateChecksum(void *data, u16 size)
{
    u16 i;
    u32 checksum = 0;

    for (i = 0; i < (size / 4); i++)
    {
        checksum += *((u32 *)data);
        data += sizeof(u32);
    }

    return ((checksum >> 16) + checksum);
}

// Like CalculateChecksum, but the footer's version and layout hash are part of it
static u16 CalculateSectorChecksum(void *data, u16 size, u16 version, u16 layoutHash)
{
    u16 i;
    u32 checksum = version | (layoutHash << 16);

    for (i = 0; i < (size / 4); i++)
    {
        checksum += *((u32 *)data);
        data += sizeof(u32);
    }

    return ((checksum >> 16) + checksum);
}

static void UpdateSaveAddresses(void)
{
    int i = SECTOR_ID_SAVEBLOCK2;

    // Every save path starts here, so the RAM image always says what it is
    gSaveBlock2Ptr->saveFormatVersion = SAVE_FORMAT_VERSION;
    gRamSaveSectorLocations[i].data = (void *)(gSaveBlock2Ptr) + sSaveSlotLayout[i].offset;
    gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;

    for (i = SECTOR_ID_SAVEBLOCK1_START; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
    {
        gRamSaveSectorLocations[i].data = (void *)(gSaveBlock1Ptr) + sSaveSlotLayout[i].offset;
        gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;
    }

    for (; i <= SECTOR_ID_PKMN_STORAGE_END; i++) //setting i to SECTOR_ID_PKMN_STORAGE_START does not match
    {
        gRamSaveSectorLocations[i].data = (void *)(gPokemonStoragePtr) + sSaveSlotLayout[i].offset;
        gRamSaveSectorLocations[i].size = sSaveSlotLayout[i].size;
    }
}

#ifdef SAVE_COPY_ON_WRITE
/*
 * Copy-on-write save (SAVE_COPY_ON_WRITE, include/config.h)
 *
 * Sector id i (0-12, same chunks as above) has two homes on the flash:
 * A = sector i and B = sector NUM_SECTORS_PER_SLOT + i. A commit record says
 * which home holds each id; the two commit sectors take turns, generation g
 * goes into SECTOR_ID_SAVE_COMMIT_0 + g % 2.
 *
 * A save of generation g + 1:
 *  1. hashes every sector's data and compares it with the hash of what its
 *     current home holds (remembered from the load or the last save); only
 *     ids that changed are written, each into its other home, and verified,
 *  2. erases the commit sector generation g - 1 used and programs the new
 *     record into it except its last byte, and verifies that,
 *  3. programs the record's last byte: the save is committed.
 * Nothing the generation g record points at is written before step 3, and its
 * commit sector is not touched at all, so a power cut at any point leaves the
 * generation g save loadable: the loader takes the newest commit record that
 * is complete and whose sectors check out. A cut in step 1 or 2 loses the
 * new save, never the old one. Sector writes that fail verification mark the
 * sector in gDamagedSaveSectors and stop the save; the sectors marked are
 * never part of the committed save, so the save-failed screen can wipe them.
 *
 * The record also holds a hash of all 13 sectors' contents, so a commit
 * record can't be paired with sectors that were overwritten after it (which
 * the order above never does, but a half-erased or damaged sector could).
 *
 * The content hash is a 32-bit FNV-1a over words: a change of any one word
 * always changes it, and the chance that a bigger change keeps it is 1 in
 * 2^32 per changed sector per save. The sector checksum (Emerald's 16-bit
 * word sum) is too weak for this: swapping two Pokémon in a box keeps it.
 */

#define SAVE_COMMIT_MAGIC 0x4D4F4321 // "!COM"
#define SAVE_COMMIT_DONE  0x5A       // the last byte of a complete record

struct SaveCommit
{
    u32 magic;         // SAVE_COMMIT_MAGIC
    u16 formatVersion; // SAVE_FORMAT_VERSION of the save
    u16 layoutHash;
    u32 generation;    // goes up by one with every save
    u32 contentHash;   // CowCombineHashes of the sectors at their homes
    u16 homes;         // bit i set: sector id i is in its B home
    u16 unused;        // 0xFFFF
    u16 check;         // CalculateChecksum of everything above
    u8 done;           // SAVE_COMMIT_DONE, programmed last: the commit point
    u8 padding;
};

STATIC_ASSERT(NUM_SECTORS_PER_SLOT <= 16, SaveCommitHomesFitInU16);
STATIC_ASSERT(offsetof(struct SaveCommit, check) % 4 == 0, SaveCommitCheckIsWordSum);
STATIC_ASSERT(NUM_SECTORS_PER_SLOT * 2 <= SECTOR_ID_SAVE_COMMIT_0
           && SECTOR_ID_SAVE_COMMIT_0 + 1 == SECTOR_ID_SAVE_COMMIT_1
           && SECTOR_ID_SAVE_COMMIT_1 < SECTOR_ID_HOF_1, SaveCowLayoutFits);

#define COW_HOME_SECTOR(id, inB) ((id) + ((inB) ? NUM_SECTORS_PER_SLOT : 0))
#define COW_COMMIT_SECTOR(generation) (SECTOR_ID_SAVE_COMMIT_0 + ((generation) & 1))

struct CowState
{
    u32 generation;
    u16 homes;     // as in SaveCommit
    u16 hashValid; // bit i set: hashes[i] is what id i's current home holds
    u32 hashes[NUM_SECTORS_PER_SLOT];
};

enum
{
    COW_IDLE,
    COW_WRITING,   // CowBegin ran, sectors are being written
    COW_COMMITTING // the record is programmed except its last byte
};

// sCow is the committed save; sCowPending the one being written (120 B)
EWRAM_DATA static struct CowState sCow = {0};
EWRAM_DATA static struct CowState sCowPending = {0};
EWRAM_DATA static u8 sCowNextSector = 0;
EWRAM_DATA static u8 sCowStage = COW_IDLE;

// FNV-1a over the words of a sector's data, zero padded to SECTOR_DATA_SIZE,
// so it is the same whatever size the layout gives the sector
static u32 CowHash(const void *data, u16 size)
{
    const u32 *words = data;
    u32 hash = 0x811C9DC5;
    u16 i;

    for (i = 0; i < size / 4; i++)
        hash = (hash ^ words[i]) * 0x01000193;
    for (; i < SECTOR_DATA_SIZE / 4; i++)
        hash *= 0x01000193;
    return hash;
}

static u32 CowCombineHashes(const u32 *hashes)
{
    u32 hash = 0x811C9DC5;
    u16 i;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
        hash = (hash ^ hashes[i]) * 0x01000193;
    return hash;
}

static void CowForgetSave(void)
{
    memset(&sCow, 0, sizeof(sCow));
    sCowStage = COW_IDLE;
}

static void CowBegin(void)
{
    gReadWriteSector = &gSaveDataBuffer;
    // The save-failed screen has wiped what the last attempt marked, and a
    // sector this save doesn't write must not fail it
    gDamagedSaveSectors = 0;
    sCowPending = sCow;
    sCowPending.generation = sCow.generation + 1;
    sCowNextSector = 0;
    sCowStage = COW_WRITING;
}

// Writes the next sector whose data changed into its other home. FALSE when
// no changed sector is left.
static bool8 CowWriteNextChanged(void)
{
    const struct SaveSectorLocation *locations = gRamSaveSectorLocations;

    while (sCowNextSector < NUM_SECTORS_PER_SLOT)
    {
        u8 id = sCowNextSector++;
        u16 bit = 1 << id;
        u32 hash = CowHash(locations[id].data, locations[id].size);

        if ((sCow.hashValid & bit) && sCow.hashes[id] == hash)
            continue; // its home already holds this

        FillSaveSector(id, locations, sCowPending.generation);
        if (TryWriteSector(COW_HOME_SECTOR(id, !(sCow.homes & bit)), gReadWriteSector->data) == SAVE_STATUS_OK)
        {
            sCowPending.homes ^= bit;
            sCowPending.hashes[id] = hash;
            sCowPending.hashValid |= bit;
        }
        return TRUE;
    }
    return FALSE;
}

static void CowFillCommit(struct SaveCommit *commit, const struct CowState *state)
{
    memset(commit, 0xFF, sizeof(*commit));
    commit->magic = SAVE_COMMIT_MAGIC;
    commit->formatVersion = SAVE_FORMAT_VERSION;
    commit->layoutHash = SAVE_LAYOUT_HASH;
    commit->generation = state->generation;
    commit->contentHash = CowCombineHashes(state->hashes);
    commit->homes = state->homes;
    commit->check = CalculateChecksum(commit, offsetof(struct SaveCommit, check));
    commit->done = SAVE_COMMIT_DONE;
}

// Step 2: erase the commit sector and program the record but its last byte
static u8 CowWriteCommitRecord(void)
{
    u16 sector = COW_COMMIT_SECTOR(sCowPending.generation);
    struct SaveCommit *commit = (struct SaveCommit *)gSaveDataBuffer.data;
    struct SaveCommit readBack;
    u32 i, try;

    if (sCowStage != COW_WRITING || gDamagedSaveSectors)
        return SAVE_STATUS_ERROR;
    if (sCowPending.hashValid != (1 << NUM_SECTORS_PER_SLOT) - 1)
        return SAVE_STATUS_ERROR; // can't happen: every unknown sector gets written

    CowFillCommit(commit, &sCowPending);
    for (try = 0; try < 3; try++)
    {
        REGRESS_SAVE_FLASH_OP(sector, (u8 *)commit, offsetof(struct SaveCommit, done), FALSE);
        if (EraseFlashSector(sector))
            continue;
        for (i = 0; i < offsetof(struct SaveCommit, done); i++)
        {
            if (ProgramFlashByte(sector, i, ((u8 *)commit)[i]))
                break;
        }
        ReadFlash(sector, 0, (u8 *)&readBack, sizeof(readBack));
        if (i == offsetof(struct SaveCommit, done)
         && memcmp(&readBack, commit, offsetof(struct SaveCommit, done)) == 0
         && readBack.done == 0xFF)
        {
            sCowStage = COW_COMMITTING;
            return SAVE_STATUS_OK;
        }
    }
    SetDamagedSectorBits(ENABLE, sector);
    return SAVE_STATUS_ERROR;
}

// Step 3: program the record's last byte. Only now is the new save the one
// that loads.
static u8 CowFinishCommit(void)
{
    u16 sector = COW_COMMIT_SECTOR(sCowPending.generation);
    u8 done;

    if (sCowStage != COW_COMMITTING || gDamagedSaveSectors)
        return SAVE_STATUS_ERROR;

    REGRESS_SAVE_FLASH_OP(sector, NULL, 0, FALSE);
    ProgramFlashByte(sector, offsetof(struct SaveCommit, done), SAVE_COMMIT_DONE);
    ReadFlash(sector, offsetof(struct SaveCommit, done), &done, 1);
    sCowStage = COW_IDLE;
    if (done != SAVE_COMMIT_DONE)
    {
        SetDamagedSectorBits(ENABLE, sector);
        return SAVE_STATUS_ERROR;
    }
    sCow = sCowPending;
    return SAVE_STATUS_OK;
}

// A whole save in one go
static u8 WriteSaveData(void)
{
    CowBegin();
    while (CowWriteNextChanged())
    {
        if (gDamagedSaveSectors)
        {
            sCowStage = COW_IDLE;
            return SAVE_STATUS_ERROR;
        }
    }
    if (CowWriteCommitRecord() != SAVE_STATUS_OK)
        return SAVE_STATUS_ERROR;
    return CowFinishCommit();
}

static bool32 CowReadCommit(u16 sector, struct SaveCommit *commit)
{
    ReadFlash(sector, 0, (u8 *)commit, sizeof(*commit));
    return commit->magic == SAVE_COMMIT_MAGIC
        && commit->done == SAVE_COMMIT_DONE
        && commit->check == CalculateChecksum(commit, offsetof(struct SaveCommit, check));
}

// Checks that every sector the record points at is intact and belongs to it.
// With dest set, also copies their data: into the save blocks for a save of
// this version (dest == NULL image), or whole sectors into a migration image.
// hashes gets each sector's content hash.
static bool32 CowReadCommittedSectors(const struct SaveCommit *commit, bool32 copy, u8 *image, u32 *hashes)
{
    const struct SaveSectorLocation *locations = gRamSaveSectorLocations;
    bool32 current = (commit->formatVersion == SAVE_FORMAT_VERSION && commit->layoutHash == SAVE_LAYOUT_HASH);
    u16 i, size;

    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        ReadFlashSector(COW_HOME_SECTOR(i, commit->homes & (1 << i)), gReadWriteSector);
        size = current ? locations[i].size : SECTOR_DATA_SIZE;
        if (gReadWriteSector->signature != SECTOR_SIGNATURE
         || gReadWriteSector->id != i
         || gReadWriteSector->formatVersion != commit->formatVersion
         || gReadWriteSector->layoutHash != commit->layoutHash
         || gReadWriteSector->counter > commit->generation
         || gReadWriteSector->checksum != CalculateSectorChecksum(gReadWriteSector->data, size,
                                              commit->formatVersion, commit->layoutHash))
            return FALSE;
        hashes[i] = CowHash(gReadWriteSector->data, SECTOR_DATA_SIZE);
        if (copy && image != NULL)
            memcpy(&image[i * SECTOR_DATA_SIZE], gReadWriteSector->data, SECTOR_DATA_SIZE);
        else if (copy)
            memcpy(locations[i].data, gReadWriteSector->data, locations[i].size);
    }
    return CowCombineHashes(hashes) == commit->contentHash;
}

// Whether the save sectors hold a save in another format (Emerald's, or the
// two-slot scheme). Sectors of this format with no commit record are a first
// save that never finished: no save, as far as the player is concerned.
static bool32 CowFlashHasOldSave(void)
{
    u16 i;

    for (i = 0; i < NUM_SECTORS_PER_SLOT * 2 + 2; i++)
    {
        ReadFlashSector(i, gReadWriteSector);
        if (gReadWriteSector->signature == SECTOR_SIGNATURE
         && (gReadWriteSector->formatVersion != SAVE_FORMAT_VERSION || gReadWriteSector->layoutHash != SAVE_LAYOUT_HASH))
            return TRUE;
    }
    return FALSE;
}

static u8 TryLoadSaveSlot(u16 sectorId, struct SaveSectorLocation *locations)
{
    struct SaveCommit commits[2];
    bool8 valid[2];
    u32 hashes[NUM_SECTORS_PER_SLOT];
    u32 newestGeneration = 0;
    s8 newest = -1, newestForeign = -1;
    u8 order[2];
    u8 i, status;

    gReadWriteSector = &gSaveDataBuffer;
    CowForgetSave();

    for (i = 0; i < 2; i++)
    {
        valid[i] = CowReadCommit(SECTOR_ID_SAVE_COMMIT_0 + i, &commits[i]);
        if (!valid[i])
            continue;
        if (commits[i].generation >= newestGeneration)
            newestGeneration = commits[i].generation;
        if (commits[i].formatVersion != SAVE_FORMAT_VERSION || commits[i].layoutHash != SAVE_LAYOUT_HASH)
        {
            if (newestForeign < 0 || commits[i].generation > commits[newestForeign].generation)
                newestForeign = i;
        }
        else if (newest < 0 || commits[i].generation > commits[newest].generation)
        {
            newest = i;
        }
    }
    // Whatever happens, the next save's commit must be the newest record
    sCow.generation = newestGeneration;

    if (newestForeign >= 0 && (newest < 0 || commits[newestForeign].generation > commits[newest].generation))
    {
        // The newest save is of another version
        const struct SaveCommit *commit = &commits[newestForeign];
        u8 *image = gHeap; // free here, see LoadGameSave

        sCow.homes = commit->homes;
        if (!CanMigrateSave(commit->formatVersion, commit->layoutHash)
         || !CowReadCommittedSectors(commit, TRUE, image, hashes))
            return SAVE_STATUS_INCOMPATIBLE;
        status = RunSaveMigrations(image, commit->formatVersion, locations);
        // The flash still holds the old version, so the next save writes everything
        sCow.generation = commit->generation;
        return status;
    }

    if (newest < 0)
        return CowFlashHasOldSave() ? SAVE_STATUS_INCOMPATIBLE : SAVE_STATUS_EMPTY;

    // Newest first, then the other record if it is of this version too
    order[0] = newest;
    order[1] = newest ^ 1;
    for (i = 0; i < 2; i++)
    {
        const struct SaveCommit *commit = &commits[order[i]];
        if (!valid[order[i]] || commit->formatVersion != SAVE_FORMAT_VERSION || commit->layoutHash != SAVE_LAYOUT_HASH)
            break;
        if (!CowReadCommittedSectors(commit, FALSE, NULL, hashes))
            continue;
        CowReadCommittedSectors(commit, TRUE, NULL, hashes);
        // Saving goes on from the record that loaded, so the next commit
        // replaces the other record, never this one
        sCow.generation = commit->generation;
        sCow.homes = commit->homes;
        memcpy(sCow.hashes, hashes, sizeof(hashes));
        sCow.hashValid = (1 << NUM_SECTORS_PER_SLOT) - 1;
        // Like Emerald: the newest save was damaged, the one before it loaded
        return (i == 0) ? SAVE_STATUS_OK : SAVE_STATUS_ERROR;
    }

    // No commit record of this version has intact sectors. Home bits from the
    // newest record keep the next save off what is left of it.
    sCow.homes = commits[newest].homes;
    return SAVE_STATUS_CORRUPT;
}
#endif // SAVE_COPY_ON_WRITE

u8 HandleSavingData(u8 saveType)
{
    u8 i;
    u8 *tempAddr;

    UpdateSaveAddresses();
    switch (saveType)
    {
    case SAVE_HALL_OF_FAME_ERASE_BEFORE:
        // Unused. Erases the Hall of Fame sectors before overwriting them.
        for (i = SECTOR_ID_HOF_1; i < SECTOR_ID_HOF_1 + NUM_HOF_SECTORS; i++)
            EraseFlashSector(i);
        // fallthrough
    case SAVE_HALL_OF_FAME:
        if (GetGameStat(GAME_STAT_ENTERED_HOF) < 999)
            IncrementGameStat(GAME_STAT_ENTERED_HOF);

        // Write the full save slot first
        CopyPartyAndObjectsToSave();
        WriteSaveData();

        // Save the Hall of Fame
        tempAddr = gDecompressionBuffer;
        HandleWriteSectorNBytes(SECTOR_ID_HOF_1, tempAddr, SECTOR_DATA_SIZE);
        HandleWriteSectorNBytes(SECTOR_ID_HOF_2, tempAddr + SECTOR_DATA_SIZE, SECTOR_DATA_SIZE);
        break;
    case SAVE_NORMAL:
    default:
        CopyPartyAndObjectsToSave();
        WriteSaveData();
        break;
    case SAVE_LINK:
    case SAVE_EREADER: // Dummied, now duplicate of SAVE_LINK
        // Used by link / Battle Frontier
        CopyPartyAndObjectsToSave();
#ifdef SAVE_COPY_ON_WRITE
        // A normal save: it only writes what changed anyway, and a PC box
        // changed since the last save is kept too
        WriteSaveData();
#else
        // Write only SaveBlocks 1 and 2 (skips the PC), in place in the
        // current slot
        for(i = SECTOR_ID_SAVEBLOCK2; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
            HandleReplaceSector(i, gRamSaveSectorLocations);
        for(i = SECTOR_ID_SAVEBLOCK2; i <= SECTOR_ID_SAVEBLOCK1_END; i++)
            WriteSectorSignatureByte_NoOffset(i, gRamSaveSectorLocations);
#endif
        break;
    case SAVE_OVERWRITE_DIFFERENT_FILE:
        // Erase Hall of Fame. Emerald also erased the Trainer Hill and
        // Recorded Battle sectors here; they are free sectors now and kept.
        for (i = SECTOR_ID_HOF_1; i < SECTOR_ID_HOF_1 + NUM_HOF_SECTORS; i++)
            EraseFlashSector(i);

        // Overwrite save slot
        CopyPartyAndObjectsToSave();
        WriteSaveData();
        break;
    }
    return 0;
}

u8 TrySavingData(u8 saveType)
{
    if (gFlashMemoryPresent != TRUE)
    {
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }

    HandleSavingData(saveType);
    if (!gDamagedSaveSectors)
    {
        gSaveAttemptStatus = SAVE_STATUS_OK;
        return SAVE_STATUS_OK;
    }
    else
    {
        DoSaveFailedScreen(saveType);
        gSaveAttemptStatus = SAVE_STATUS_ERROR;
        return SAVE_STATUS_ERROR;
    }
}

#ifdef SAVE_COPY_ON_WRITE
// The link saves split a save over several frames: Task_LinkFullSave keeps
// both players in step between the steps, and the commit byte is the last.
bool8 LinkFullSave_Init(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;
    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    CowBegin();
    return FALSE;
}

// Writes the next changed sector. TRUE when there are none left (or a write
// failed, which also shows the save-failed screen).
bool8 LinkFullSave_WriteSector(void)
{
    if (sCowStage == COW_WRITING && CowWriteNextChanged() && !gDamagedSaveSectors)
        return FALSE;
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return TRUE;
}

// Programs the commit record but its last byte
bool8 LinkFullSave_ReplaceLastSector(void)
{
    CowWriteCommitRecord();
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

// Commits the save
bool8 LinkFullSave_SetLastSectorSignature(void)
{
    CowFinishCommit();
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

// WriteSaveBlock2, then WriteSaveBlock1Sector once a frame until it returns
// TRUE, is a save spread over frames for link screens. In Emerald they wrote
// the save blocks in place; here they write the changed sectors one per call
// and commit at the end.
bool8 WriteSaveBlock2(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;

    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    CowBegin();
    CowWriteNextChanged();
    return FALSE;
}

bool8 WriteSaveBlock1Sector(void)
{
    if (sCowStage == COW_WRITING && !gDamagedSaveSectors && CowWriteNextChanged())
        return FALSE;

    if (CowWriteCommitRecord() == SAVE_STATUS_OK)
        CowFinishCommit();
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_LINK);
    return TRUE;
}

#else

bool8 LinkFullSave_Init(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;
    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    RestoreSaveBackupVarsAndIncrement(gRamSaveSectorLocations);
    return FALSE;
}

bool8 LinkFullSave_WriteSector(void)
{
    u8 status = HandleWriteIncrementalSector(NUM_SECTORS_PER_SLOT, gRamSaveSectorLocations);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);

    // In this case "error" either means that an actual error was encountered
    // or that the given max sector has been reached (meaning it has finished successfully).
    // If there was an actual error the save failed screen above will also be shown.
    if (status == SAVE_STATUS_ERROR)
        return TRUE;
    else
        return FALSE;
}

bool8 LinkFullSave_ReplaceLastSector(void)
{
    HandleReplaceSectorAndVerify(NUM_SECTORS_PER_SLOT, gRamSaveSectorLocations);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

bool8 LinkFullSave_SetLastSectorSignature(void)
{
    CopySectorSignatureByte(NUM_SECTORS_PER_SLOT, gRamSaveSectorLocations);
    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_NORMAL);
    return FALSE;
}

bool8 WriteSaveBlock2(void)
{
    if (gFlashMemoryPresent != TRUE)
        return TRUE;

    UpdateSaveAddresses();
    CopyPartyAndObjectsToSave();
    RestoreSaveBackupVars(gRamSaveSectorLocations);

    // Because RestoreSaveBackupVars is called immediately prior, gIncrementalSectorId will always be 0 below,
    // so this function only saves the first sector (SECTOR_ID_SAVEBLOCK2)
    HandleReplaceSectorAndVerify(gIncrementalSectorId + 1, gRamSaveSectorLocations);
    return FALSE;
}

// Used in conjunction with WriteSaveBlock2 to write both for certain link saves.
// This will be called repeatedly in a task, writing each sector of SaveBlock1 incrementally.
// It returns TRUE when finished.
bool8 WriteSaveBlock1Sector(void)
{
    u8 finished = FALSE;
    u16 sectorId = ++gIncrementalSectorId; // Because WriteSaveBlock2 will have been called prior, this will be SECTOR_ID_SAVEBLOCK1_START
    if (sectorId <= SECTOR_ID_SAVEBLOCK1_END)
    {
        // Write a single sector of SaveBlock1
        HandleReplaceSectorAndVerify(gIncrementalSectorId + 1, gRamSaveSectorLocations);
        WriteSectorSignatureByte(sectorId, gRamSaveSectorLocations);
    }
    else
    {
        // Beyond SaveBlock1, don't write the sector.
        // Does write 1 byte of the next sector's signature field, but as these
        // are the same for all valid sectors it doesn't matter.
        WriteSectorSignatureByte(sectorId, gRamSaveSectorLocations);
        finished = TRUE;
    }

    if (gDamagedSaveSectors)
        DoSaveFailedScreen(SAVE_LINK);

    return finished;
}
#endif // SAVE_COPY_ON_WRITE

u8 LoadGameSave(u8 saveType)
{
    u8 status;

    if (gFlashMemoryPresent != TRUE)
    {
        gSaveFileStatus = SAVE_STATUS_NO_FLASH;
        return SAVE_STATUS_ERROR;
    }

    UpdateSaveAddresses();
    switch (saveType)
    {
    case SAVE_NORMAL:
    default:
        status = TryLoadSaveSlot(FULL_SAVE_SLOT, gRamSaveSectorLocations);
        CopyPartyAndObjectsFromSave();
        gSaveFileStatus = status;
        gGameContinueCallback = NULL;
        break;
    case SAVE_HALL_OF_FAME:
        status = TryLoadSaveSector(SECTOR_ID_HOF_1, gDecompressionBuffer, SECTOR_DATA_SIZE);
        if (status == SAVE_STATUS_OK)
            status = TryLoadSaveSector(SECTOR_ID_HOF_2, &gDecompressionBuffer[SECTOR_DATA_SIZE], SECTOR_DATA_SIZE);
        break;
    }

    return status;
}

u32 TryReadSpecialSaveSector(u8 sector, u8 *dst)
{
    s32 i;
    s32 size;
    u8 *savData;

    if (sector != SECTOR_ID_TRAINER_HILL && sector != SECTOR_ID_RECORDED_BATTLE)
        return SAVE_STATUS_ERROR;

    ReadFlash(sector, 0, (u8 *)&gSaveDataBuffer, SECTOR_SIZE);
    if (*(u32 *)(&gSaveDataBuffer.data[0]) != SPECIAL_SECTOR_SENTINEL)
        return SAVE_STATUS_ERROR;

    // Copies whole save sector except u32 counter
    i = 0;
    size = SECTOR_COUNTER_OFFSET - 1;
    savData = &gSaveDataBuffer.data[4]; // data[4] to skip past SPECIAL_SECTOR_SENTINEL
    for (; i <= size; i++)
        dst[i] = savData[i];
    return SAVE_STATUS_OK;
}

u32 TryWriteSpecialSaveSector(u8 sector, u8 *src)
{
    s32 i;
    s32 size;
    u8 *savData;
    void *savDataBuffer;

    if (sector != SECTOR_ID_TRAINER_HILL && sector != SECTOR_ID_RECORDED_BATTLE)
        return SAVE_STATUS_ERROR;
    // The sectors of removed systems are free for other data; never write them
#ifdef FREE_TRAINER_HILL
    if (sector == SECTOR_ID_TRAINER_HILL)
        return SAVE_STATUS_ERROR;
#endif
#ifdef FREE_RECORDED_BATTLE
    if (sector == SECTOR_ID_RECORDED_BATTLE)
        return SAVE_STATUS_ERROR;
#endif

    savDataBuffer = &gSaveDataBuffer;
    *(u32 *)(savDataBuffer) = SPECIAL_SECTOR_SENTINEL;

    // Copies whole save sector except u32 counter
    i = 0;
    size = SECTOR_COUNTER_OFFSET - 1;
    savData = &gSaveDataBuffer.data[4]; // data[4] to skip past SPECIAL_SECTOR_SENTINEL
    for (; i <= size; i++)
        savData[i] = src[i];
    if (ProgramFlashSectorAndVerify(sector, savDataBuffer) != 0)
        return SAVE_STATUS_ERROR;
    return SAVE_STATUS_OK;
}

#define tState         data[0]
#define tTimer         data[1]
#define tInBattleTower data[2]

// Note that this is very different from TrySavingData(SAVE_LINK).
// Most notably it does save the PC data.
void Task_LinkFullSave(u8 taskId)
{
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        gSoftResetDisabled = TRUE;
        tState = 1;
        break;
    case 1:
        SetLinkStandbyCallback();
        tState = 2;
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            if (!tInBattleTower)
                SaveMapView();
            tState = 3;
        }
        break;
    case 3:
        if (!tInBattleTower)
            SetContinueGameWarpStatusToDynamicWarp();
        LinkFullSave_Init();
        tState = 4;
        break;
    case 4:
        if (++tTimer == 5)
        {
            tTimer = 0;
            tState = 5;
        }
        break;
    case 5:
        if (LinkFullSave_WriteSector())
            tState = 6;
        else
            tState = 4; // Not finished, delay again
        break;
    case 6:
        LinkFullSave_ReplaceLastSector();
        tState = 7;
        break;
    case 7:
        if (!tInBattleTower)
            ClearContinueGameWarpStatus2();
        SetLinkStandbyCallback();
        tState = 8;
        break;
    case 8:
        if (IsLinkTaskFinished())
        {
            LinkFullSave_SetLastSectorSignature();
            tState = 9;
        }
        break;
    case 9:
        SetLinkStandbyCallback();
        tState = 10;
        break;
    case 10:
        if (IsLinkTaskFinished())
            tState++;
        break;
    case 11:
        if (++tTimer > 5)
        {
            gSoftResetDisabled = FALSE;
            DestroyTask(taskId);
        }
        break;
    }
}
