#include "global.h"
#include "day_night.h"
#include "event_data.h"
#include "field_weather.h"
#include "fieldmap.h"
#include "main.h"
#include "overworld.h"
#include "palette.h"
#include "palette_tone.h"
#include "rtc.h"
#include "sprite.h"
#include "tileset_anims.h"
#include "constants/rgb.h"
#include "constants/vars.h"

// Day and night, after Gold/Silver/Crystal: morning warms the light colors,
// night shifts everything to a dark violet and lit windows glow yellow.
//
// The tint is written into the palette buffers (gPlttBufferUnfaded and Faded)
// when a map's palettes load and when the time of day changes, never per frame.
// Fades, weather color maps and Flash all start from gPlttBufferUnfaded, so they
// work on the tinted colors without knowing about it. No palette slots are used.
//
// Background palettes come from a tileset's own morning/night palettes when it
// has them (struct Tileset palettesMorn/palettesNite, for hand-drawn night art),
// otherwise from its day palettes through the background tone below. Sprite
// palettes always go through the sprite tone, so no sprite needs night art.
//
// The time comes from the cartridge clock. Reading the Seiko RTC bit-bangs GPIO
// with interrupts masked, so it is read at most once a second, from the main
// loop, and not in the scanlines before the sound DMA restart (line 150) and
// VBlank. Without a working RTC (many flash carts, a dead battery) a clock
// driven by play time stands in for it.

#define MORN_HOUR_BEGIN 4
#define DAY_HOUR_BEGIN  10
#define NITE_HOUR_BEGIN 18
#define PLAYTIME_MINUTES_PER_HOUR 5   // no-RTC fallback: a full day every 2 hours of play
#define RTC_READ_INTERVAL 60          // frames
#define RTC_READ_LAST_SAFE_LINE 120   // a read takes under 10 scanlines; line 150 is the m4a VCount IRQ

#define WINDOW_GLASS RGB(14, 24, 27)  // Masara's window glass, #74C1DC
#define WINDOW_LIT   RGB(30, 30, 11)  // GSC's lit window yellow

// Background tone, fitted to GSC's day -> morn/nite palettes: in the morning
// only the light end warms up (GSC's yellower paper); at night red and green
// roughly halve and blue stays high, so the scene goes violet rather than black.
#define BG_MORN_R(x) ((x) > 20 && (x) < 31 ? (x) + 1 : (x))
#define BG_MORN_B(x) ((x) > 20 ? (x) - ((x) - 20) / 2 : (x))
#define BG_NITE_R(x) TONE_CLAMP((x) * 5 / 8 - 1)
#define BG_NITE_G(x) TONE_CLAMP((x) * 9 / 16 - 1)
#define BG_NITE_B(x) ((x) * 7 / 8)

// Sprite tone: the same morning, a slightly lighter night than the ground, so
// people and Pokémon stay readable against it (GSC kept its night NPCs bright too).
#define OBJ_NITE_R(x) ((x) * 11 / 16)
#define OBJ_NITE_G(x) ((x) * 5 / 8)
#define OBJ_NITE_B(x) ((x) * 7 / 8)

static const struct ToneTable sBgTones[TIME_OF_DAY_COUNT] =
{
    [TIME_OF_DAY_MORN] = {TONE_CHANNEL(BG_MORN_R), TONE_CHANNEL(TONE_IDENTITY), TONE_CHANNEL(BG_MORN_B)},
    [TIME_OF_DAY_DAY]  = {TONE_CHANNEL(TONE_IDENTITY), TONE_CHANNEL(TONE_IDENTITY), TONE_CHANNEL(TONE_IDENTITY)},
    [TIME_OF_DAY_NITE] = {TONE_CHANNEL(BG_NITE_R), TONE_CHANNEL(BG_NITE_G), TONE_CHANNEL(BG_NITE_B)},
};

static const struct ToneTable sSpriteTones[TIME_OF_DAY_COUNT] =
{
    [TIME_OF_DAY_MORN] = {TONE_CHANNEL(BG_MORN_R), TONE_CHANNEL(TONE_IDENTITY), TONE_CHANNEL(BG_MORN_B)},
    [TIME_OF_DAY_DAY]  = {TONE_CHANNEL(TONE_IDENTITY), TONE_CHANNEL(TONE_IDENTITY), TONE_CHANNEL(TONE_IDENTITY)},
    [TIME_OF_DAY_NITE] = {TONE_CHANNEL(OBJ_NITE_R), TONE_CHANNEL(OBJ_NITE_G), TONE_CHANNEL(OBJ_NITE_B)},
};

// Background colors that are swapped instead of tinted at night.
static const u16 sNiteColorSwaps[][2] =
{
    {WINDOW_GLASS, WINDOW_LIT},
};

EWRAM_DATA bool8 gTintFieldSprites = FALSE;
static EWRAM_DATA u8 sTimeOfDay = TIME_OF_DAY_DAY;    // what the field palettes show
static EWRAM_DATA u8 sSpriteTimeOfDay = 0;            // what the tinted sprite slots show
static EWRAM_DATA u8 sHourPlusOne = 0;                // cached clock hour + 1, 0 = none
static EWRAM_DATA bool8 sHourFromRtc = FALSE;
static EWRAM_DATA u8 sRtcBadReads = 0;                // in a row, see GetHour
static EWRAM_DATA u16 sTintedSpriteSlots = 0;         // bit n: OBJ slot n holds tinted colors
static EWRAM_DATA u32 sHourReadFrame = 0;
// Day colors of tinted sprite slots, to redo them when the time changes (512 B).
static EWRAM_DATA u16 sSpriteDayPalettes[16][16] = {0};

#define RTC_NONE     -1   // no clock, or it failed its check at boot
#define RTC_BAD_READ -2   // this read came back invalid
#define RTC_MAX_BAD_READS 5

static s32 ReadRtcHour(void)
{
    struct SiiRtcInfo rtc;
    struct Time time;

    if (RtcGetErrorStatus() & RTC_ERR_FLAG_MASK)
        return RTC_NONE;
    RtcGetInfo(&rtc);
    if (RtcCheckInfo(&rtc) & RTC_ERR_FLAG_MASK)
        return RTC_BAD_READ;
    RtcCalcTimeDifference(&rtc, &time, &gSaveBlock2Ptr->localTimeOffset);
    return ((time.hours % 24) + 24) % 24;
}

static s32 GetPlayTimeHour(void)
{
    s32 hour = (gSaveBlock2Ptr->playTimeHours * 60 + gSaveBlock2Ptr->playTimeMinutes) / PLAYTIME_MINUTES_PER_HOUR;

    return (hour + DAY_HOUR_BEGIN) % 24;
}

static s32 GetHour(void)
{
    s32 hour;

    if (sHourPlusOne != 0)
    {
        if (gMain.vblankCounter1 - sHourReadFrame < RTC_READ_INTERVAL)
            return sHourPlusOne - 1;
        // Not this frame if the read could still run when the sound IRQ is due.
        if (REG_VCOUNT > RTC_READ_LAST_SAFE_LINE)
            return sHourPlusOne - 1;
    }
    hour = ReadRtcHour();
    if (hour >= 0)
    {
        sHourFromRtc = TRUE;
        sRtcBadReads = 0;
    }
    else if (hour == RTC_BAD_READ && sHourFromRtc && sHourPlusOne != 0 && ++sRtcBadReads < RTC_MAX_BAD_READS)
    {
        // A bad read from a clock that was working: keep its last hour for a few seconds.
        hour = sHourPlusOne - 1;
    }
    else
    {
        sHourFromRtc = FALSE;
        hour = GetPlayTimeHour();
    }
    sHourPlusOne = hour + 1;
    sHourReadFrame = gMain.vblankCounter1;
    return hour;
}

u8 GetTimeOfDay(void)
{
    s32 hour;
    u16 override = VarGet(VAR_TIME_OF_DAY_OVERRIDE);

    if (override != 0 && override <= TIME_OF_DAY_COUNT)
        return override - 1;
    hour = GetHour();
    if (hour >= MORN_HOUR_BEGIN && hour < DAY_HOUR_BEGIN)
        return TIME_OF_DAY_MORN;
    if (hour >= DAY_HOUR_BEGIN && hour < NITE_HOUR_BEGIN)
        return TIME_OF_DAY_DAY;
    return TIME_OF_DAY_NITE;
}

u8 GetFieldTimeOfDay(void)
{
    return sTimeOfDay;
}

const u16 (*GetTilesetPalettesForTimeOfDay(const struct Tileset *tileset))[16]
{
    switch (sTimeOfDay)
    {
    case TIME_OF_DAY_MORN:
        return tileset->palettesMorn;
    case TIME_OF_DAY_NITE:
        return tileset->palettesNite;
    }
    return NULL;
}

static void TintBgEntries(u16 offset, u16 count)
{
    // A copy on the (IWRAM) stack: lookups from ROM cost wait states.
    struct ToneTable tone = sBgTones[sTimeOfDay];
    u16 *unfaded = &gPlttBufferUnfaded[offset];
    u16 *faded = &gPlttBufferFaded[offset];
    u32 i, j, numSwaps = (sTimeOfDay == TIME_OF_DAY_NITE) ? ARRAY_COUNT(sNiteColorSwaps) : 0;

    for (i = 0; i < count; i++)
    {
        u32 color = unfaded[i];

        for (j = 0; j < numSwaps; j++)
        {
            if (color == sNiteColorSwaps[j][0])
                break;
        }
        if (j < numSwaps)
            color = sNiteColorSwaps[j][1];
        else
            color = ToneColor(&tone, color);
        unfaded[i] = faded[i] = color;
    }
}

// The slot's colors in gPlttBufferUnfaded are day colors: keep them, then tint.
static void TintSpriteSlot(u32 slot, u8 timeOfDay)
{
    u16 *pal = &gPlttBufferUnfaded[OBJ_PLTT_ID(slot)];
    u16 *faded = &gPlttBufferFaded[OBJ_PLTT_ID(slot)];
    const struct ToneTable *tone = &sSpriteTones[timeOfDay];
    u32 i;

    if (timeOfDay == TIME_OF_DAY_DAY)
    {
        sTintedSpriteSlots &= ~(1 << slot);
        return;
    }
    CpuCopy16(pal, sSpriteDayPalettes[slot], PLTT_SIZE_4BPP);
    for (i = 0; i < 16; i++)
        pal[i] = faded[i] = ToneColor(tone, pal[i]);
    sTintedSpriteSlots |= 1 << slot;
}

static bool32 IsSpriteSlotInUse(u32 slot)
{
    // Tagged palettes plus the player's fixed, untagged slots.
    return GetSpritePaletteTagByPaletteNum(slot) != TAG_NONE || slot < gReservedSpritePaletteCount;
}

// Brings every sprite slot in use to timeOfDay, from the day colors.
static void SetSpriteTimeOfDay(u8 timeOfDay)
{
    u32 slot;

    for (slot = 0; slot < 16; slot++)
    {
        bool32 tinted = (sTintedSpriteSlots >> slot) & 1;

        if (!IsSpriteSlotInUse(slot))
        {
            sTintedSpriteSlots &= ~(1 << slot);
            continue;
        }
        if (tinted && sSpriteTimeOfDay == timeOfDay)
            continue;
        if (tinted)
        {
            CpuCopy16(sSpriteDayPalettes[slot], &gPlttBufferUnfaded[OBJ_PLTT_ID(slot)], PLTT_SIZE_4BPP);
            CpuCopy16(sSpriteDayPalettes[slot], &gPlttBufferFaded[OBJ_PLTT_ID(slot)], PLTT_SIZE_4BPP);
        }
        TintSpriteSlot(slot, timeOfDay);
    }
    sSpriteTimeOfDay = timeOfDay;
}

void ApplyTimeOfDayTint(u16 offset, u16 count)
{
    if (sTimeOfDay == TIME_OF_DAY_DAY)
        return;
    if (offset < OBJ_PLTT_OFFSET)
    {
        TintBgEntries(offset, count);
    }
    else
    {
        // Sprite palettes load a whole slot at a time.
        u32 slot;

        for (slot = (offset - OBJ_PLTT_OFFSET) / 16; slot < 16 && OBJ_PLTT_ID(slot) < offset + count; slot++)
            TintSpriteSlot(slot, sTimeOfDay);
        sSpriteTimeOfDay = sTimeOfDay;
    }
}

void DayNight_OnMapPalettesLoad(void)
{
    // Indoors always looks like day. Sprites loaded from here until the next
    // FreeAllSpritePalettes (battle, menus) get the same tint.
    sTimeOfDay = IsMapTypeOutdoors(gMapHeader.mapType) ? GetTimeOfDay() : TIME_OF_DAY_DAY;
    // The player and some objects are already loaded at this point.
    SetSpriteTimeOfDay(sTimeOfDay);
    gTintFieldSprites = (sTimeOfDay != TIME_OF_DAY_DAY);
}

void DayNight_ResetSpriteTint(void)
{
    gTintFieldSprites = FALSE;
    sTintedSpriteSlots = 0;
}

static void ChangeTimeOfDay(u8 timeOfDay)
{
    u32 i;

    sTimeOfDay = timeOfDay;
    gTintFieldSprites = (timeOfDay != TIME_OF_DAY_DAY);
    SetSpriteTimeOfDay(timeOfDay);
    ReloadMapTilesetPalettes();
    // The palette buffers now hold the new colors; redo the weather on top.
    for (i = 0; i < NUM_PALS_TOTAL; i++)
        ApplyWeatherColorMapToPal(i);
    for (i = 0; i < 16; i++)
    {
        if (IsSpriteSlotInUse(i))
            UpdateSpritePaletteWithWeather(i);
    }
}

void DayNight_Update(void)
{
    u8 timeOfDay;

    if (!IsMapTypeOutdoors(gMapHeader.mapType))
        return;
    timeOfDay = GetTimeOfDay();
    if (timeOfDay == sTimeOfDay)
        return;
    // Wait for fades and weather changes to finish, and keep off the frames
    // that DMA tile animations.
    if (gPaletteFade.active || !IsWeatherPalStateIdle() || AreTilesetAnimsBusy())
        return;
    ChangeTimeOfDay(timeOfDay);
}
