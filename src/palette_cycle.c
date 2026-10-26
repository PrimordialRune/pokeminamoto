#include "global.h"
#include "day_night.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "palette.h"
#include "palette_cycle.h"
#include "constants/rgb.h"

// Palette cycles of the map tilesets, run from the tileset animation timer
// (tileset_anims.c). A cycle writes gPlttBufferUnfaded, then redoes that one
// palette's weather color map into gPlttBufferFaded; while a palette fade runs
// it only writes gPlttBufferUnfaded, which the fade blends from every frame.
// Day/night tints the colors when the palettes load, so a rotation moves tinted
// colors around; PALETTE_CYCLE_COLORS lists final colors and is meant for one
// time of day (timeOfDayMask).

extern const struct Tileset gTileset_Kanto;
extern const struct Tileset gTileset_MasaraTown;

// Masara at night: the lit windows (glass #74C1DC, made GSC yellow by the night
// tint) flicker like lamplight. The glass color sits in Kanto palette 1 and in
// Masara's own palette 6.
static const u16 sMasaraLitWindowColors[] =
{
    RGB(30, 30, 11),
    RGB(31, 28, 10),
    RGB(31, 31, 14),
    RGB(30, 29, 11),
    RGB(31, 30, 13),
    RGB(29, 28, 10),
};

static const struct PaletteCycle sPaletteCycles[] =
{
    {
        .tileset = &gTileset_Kanto,
        .palette = 1,
        .entry = 9,
        .count = ARRAY_COUNT(sMasaraLitWindowColors),
        .mode = PALETTE_CYCLE_COLORS,
        .period = 12,
        .timeOfDayMask = 1 << TIME_OF_DAY_NITE,
        .colors = sMasaraLitWindowColors,
    },
    {
        .tileset = &gTileset_MasaraTown,
        .palette = 6,
        .entry = 10,
        .count = ARRAY_COUNT(sMasaraLitWindowColors),
        .mode = PALETTE_CYCLE_COLORS,
        .period = 12,
        .timeOfDayMask = 1 << TIME_OF_DAY_NITE,
        .colors = sMasaraLitWindowColors,
    },
};

static EWRAM_DATA u16 sPaletteCycleTimer = 0;

static bool32 IsCycleActive(const struct PaletteCycle *cycle)
{
    const struct MapLayout *layout = gMapHeader.mapLayout;

    if (layout == NULL || (cycle->tileset != layout->primaryTileset && cycle->tileset != layout->secondaryTileset))
        return FALSE;
    if (cycle->timeOfDayMask != 0 && !(cycle->timeOfDayMask & (1 << GetFieldTimeOfDay())))
        return FALSE;
    return TRUE;
}

static void CommitPalette(u8 palette)
{
    // Fades (palette or weather) rebuild gPlttBufferFaded from Unfaded themselves.
    if (!gPaletteFade.active && IsWeatherPalStateIdle())
        ApplyWeatherColorMapToPal(palette);
}

static void StepCycle(const struct PaletteCycle *cycle, u16 step, bool8 refresh)
{
    u16 *colors = &gPlttBufferUnfaded[BG_PLTT_ID(cycle->palette) + cycle->entry];

    if (cycle->mode == PALETTE_CYCLE_COLORS)
    {
        colors[0] = cycle->colors[step % cycle->count];
    }
    else if (!refresh)
    {
        u16 last = colors[cycle->count - 1];
        u32 i;

        for (i = cycle->count - 1; i > 0; i--)
            colors[i] = colors[i - 1];
        colors[0] = last;
    }
    CommitPalette(cycle->palette);
}

void PaletteCycles_Init(void)
{
    sPaletteCycleTimer = 0;
    PaletteCycles_Refresh();
}

void PaletteCycles_Update(void)
{
    u32 i;

    sPaletteCycleTimer++;
    for (i = 0; i < ARRAY_COUNT(sPaletteCycles); i++)
    {
        const struct PaletteCycle *cycle = &sPaletteCycles[i];

        if (sPaletteCycleTimer % cycle->period == 0 && IsCycleActive(cycle))
            StepCycle(cycle, sPaletteCycleTimer / cycle->period, FALSE);
    }
}

void PaletteCycles_Refresh(void)
{
    u32 i;

    for (i = 0; i < ARRAY_COUNT(sPaletteCycles); i++)
    {
        if (IsCycleActive(&sPaletteCycles[i]))
            StepCycle(&sPaletteCycles[i], sPaletteCycleTimer / sPaletteCycles[i].period, TRUE);
    }
}
