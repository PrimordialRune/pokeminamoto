#include "global.h"
#include "palette.h"
#include "palette_tone.h"

// LCD profiles. The art is drawn and checked on PC screens, which show the
// GBA's 15-bit colors as they are; so do backlit IPS mods, emulators and the
// Analogue Pocket. The original GBA's unlit LCD (AGB-001) shows the same values
// much darker, with crushed shadows, and the SP's front/back light sits in
// between. Games of that era brightened their palettes for it; here the player
// picks a curve that lifts the darks for their screen instead.
//
// The curve is applied on the way from gPlttBufferFaded to palette RAM, so it
// covers every screen (field, battle, menus) and every fade, and the palette
// buffers keep the authored colors that day/night, weather and fades work on.
// With the IPS profile (the default) the copy is the plain DMA it always was.
// Otherwise the 32 palette rows are hashed each VBlank and only the rows that
// changed are converted into sPlttBufferLcd, which is then copied by DMA.
// One more row is redone every frame anyway, so a hash collision can at worst
// leave a row stale for 32 frames.

static const struct ToneTable sLcdTones[LCD_PROFILE_COUNT] =
{
    [LCD_PROFILE_IPS] = {
        .r = TONE_CHANNEL(TONE_IDENTITY),
        .g = TONE_CHANNEL(TONE_IDENTITY),
        .b = TONE_CHANNEL(TONE_IDENTITY),
    },
    // 31 * (x / 31) ^ 0.85
#define SP_CURVE { 0, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 28, 29, 30, 31 }
    [LCD_PROFILE_SP] = {.r = SP_CURVE, .g = SP_CURVE, .b = SP_CURVE},
    // 31 * (x / 31) ^ 0.65
#define AGB_CURVE { 0, 3, 5, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23, 23, 24, 25, 26, 26, 27, 28, 28, 29, 30, 30, 31 }
    [LCD_PROFILE_AGB] = {.r = AGB_CURVE, .g = AGB_CURVE, .b = AGB_CURVE},
};

// IWRAM: read on every VBlank while a profile is on.
static struct ToneTable sLcdTone;
static u32 sLcdRowHash[32];
static u8 sLcdToneProfile;   // profile sLcdTone holds; LCD_PROFILE_IPS = not built
static u8 sLcdRefreshRow;
static EWRAM_DATA u16 ALIGNED(4) sPlttBufferLcd[PLTT_BUFFER_SIZE] = {0};

u8 GetLcdProfile(void)
{
    u8 profile = gSaveBlock2Ptr->optionsLcdProfile;

    return profile < LCD_PROFILE_COUNT ? profile : LCD_PROFILE_IPS;
}

// ARM in IWRAM: about 25 cycles per hashed row and 20 per converted color.
static IWRAM_CODE void ConvertChangedRows(const struct ToneTable *tone, u32 *rowHash, u32 forcedRow)
{
    const u32 *src = (const u32 *)gPlttBufferFaded;
    u32 *dst = (u32 *)sPlttBufferLcd;
    u32 row, i;

    for (row = 0; row < 32; row++, src += 8, dst += 8)
    {
        u32 hash = row;

        for (i = 0; i < 8; i++)
            hash = ((hash << 7) | (hash >> 25)) ^ src[i];
        if (hash == rowHash[row] && row != forcedRow)
            continue;
        rowHash[row] = hash;
        for (i = 0; i < 8; i++)
        {
            u32 pair = src[i];
            dst[i] = tone->r[pair & 0x1F]
                   | (tone->g[(pair >> 5) & 0x1F] << 5)
                   | (tone->b[(pair >> 10) & 0x1F] << 10)
                   | (tone->r[(pair >> 16) & 0x1F] << 16)
                   | (tone->g[(pair >> 21) & 0x1F] << 21)
                   | (tone->b[(pair >> 26) & 0x1F] << 26);
        }
    }
}

void TransferPlttBufferToPltt(void)
{
    u8 profile = GetLcdProfile();

    if (profile == LCD_PROFILE_IPS)
    {
        sLcdToneProfile = LCD_PROFILE_IPS;
        DmaCopy16(3, gPlttBufferFaded, (void *)PLTT, PLTT_SIZE);
        return;
    }
    if (profile != sLcdToneProfile)
    {
        u32 i;

        sLcdTone = sLcdTones[profile];
        sLcdToneProfile = profile;
        // A new curve: make every stored hash wrong so every row is redone.
        for (i = 0; i < ARRAY_COUNT(sLcdRowHash); i++)
            sLcdRowHash[i] = ~sLcdRowHash[i];
    }
    ConvertChangedRows(&sLcdTone, sLcdRowHash, sLcdRefreshRow);
    sLcdRefreshRow = (sLcdRefreshRow + 1) % 32;
    DmaCopy16(3, sPlttBufferLcd, (void *)PLTT, PLTT_SIZE);
}

// For colors written to palette RAM without going through the palette buffers
// (HBlank effects): the same curve TransferPlttBufferToPltt applies.
u16 ApplyLcdProfile(u16 color)
{
    return ToneColor(&sLcdTones[GetLcdProfile()], color);
}
