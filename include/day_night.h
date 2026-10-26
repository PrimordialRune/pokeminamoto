#ifndef GUARD_DAY_NIGHT_H
#define GUARD_DAY_NIGHT_H

enum
{
    TIME_OF_DAY_MORN,
    TIME_OF_DAY_DAY,
    TIME_OF_DAY_NITE,
    TIME_OF_DAY_COUNT
};

// TRUE while sprite palettes loaded in the field get the time-of-day tone.
extern bool8 gTintFieldSprites;

// The time of day from the clock (or VAR_TIME_OF_DAY_OVERRIDE). Reads the
// cartridge RTC at most once a second; without a working RTC it runs on play time.
u8 GetTimeOfDay(void);
// The time of day the field palettes are tinted for (always day indoors).
u8 GetFieldTimeOfDay(void);
// Tints palette entries in both palette buffers for the field's time of day.
// Background entries use the background tone; sprite slots use the sprite tone
// and remember their day colors, so a later change of time can redo them.
void ApplyTimeOfDayTint(u16 offset, u16 count);
// Palettes drawn for the field's time of day, or NULL to tint the day ones.
const u16 (*GetTilesetPalettesForTimeOfDay(const struct Tileset *tileset))[16];
// Map palettes are about to load: pick the time of day for this map.
void DayNight_OnMapPalettesLoad(void);
// FreeAllSpritePalettes: the slots no longer hold field palettes.
void DayNight_ResetSpriteTint(void);
// End of every overworld frame: retints everything when the time of day changes.
void DayNight_Update(void);

#endif // GUARD_DAY_NIGHT_H
