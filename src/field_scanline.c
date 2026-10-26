#include "global.h"
#include "battle_pyramid.h"
#include "field_camera.h"
#include "field_scanline.h"
#include "overworld.h"
#include "palette.h"
#include "palette_tone.h"
#include "scanline_effect.h"
#include "trig.h"
#include "constants/rgb.h"

// Overworld scanline effects (see field_scanline.h).
//
// Waves: every HBlank, DMA0 writes the three map layers' scroll registers
// (BG1HOFS..BG3VOFS, 3 words) for the next line, so each line can be shifted
// sideways. The buffer is rebuilt at the end of every overworld frame from the
// camera position, after the camera has moved, into the half of the double
// buffer the next VBlank hands to DMA0 (about 3-4k cycles a frame). BG0 (text
// boxes) and sprites don't move.
//
// Gradient: DMA0 writes one BG palette entry each HBlank. The 17 shades are
// recomputed only when the entry's color in gPlttBufferFaded changes (fades,
// time of day, weather), and go through the LCD profile like everything else.

#define WAVE_DMA_CONTROL (((DMA_ENABLE | DMA_START_HBLANK | DMA_REPEAT | DMA_SRC_INC | DMA_DEST_INC | DMA_32BIT | DMA_DEST_RELOAD) << 16) | 3)
#define WAVE_WORDS_PER_LINE 3
#define GRADIENT_SHADES 17

struct FieldScanline
{
    u8 effect;
    u8 phase;
    u8 lcdProfile;
    u16 sourceColor;
    struct FieldScanlineWave wave;
    struct FieldScanlineGradient gradient;
    u16 shades[GRADIENT_SHADES];
};

static EWRAM_DATA struct FieldScanline sFieldScanline = {0};

static volatile void *GetEffectDest(void)
{
    if (sFieldScanline.effect == FIELD_SCANLINE_GRADIENT)
        return (vu16 *)PLTT + sFieldScanline.gradient.palIndex;
    return &REG_BG1HOFS;
}

static u32 GetEffectControl(void)
{
    if (sFieldScanline.effect == FIELD_SCANLINE_GRADIENT)
        return SCANLINE_EFFECT_DMACNT_16BIT;
    return WAVE_DMA_CONTROL;
}

// FALSE once anything else (Flash, a battle transition, a map load) has
// reconfigured or stopped DMA0.
static bool32 OwnsDma0(void)
{
    return sFieldScanline.effect != FIELD_SCANLINE_NONE
        && gScanlineEffect.state != 0
        && gScanlineEffect.dmaDest == GetEffectDest()
        && gScanlineEffect.dmaControl == GetEffectControl();
}

static bool32 CanStart(void)
{
    if (GetFlashLevel() != 0 || InBattlePyramid_())
        return FALSE;   // the Flash circle owns DMA0 on this map
    if (gScanlineEffect.state != 0 && !OwnsDma0())
        return FALSE;   // something else is running a scanline effect
    return TRUE;
}

static void WriteWaveBuffer(u32 *buffer)
{
    const struct FieldScanlineWave *wave = &sFieldScanline.wave;
    s16 x, y;
    u32 line, value, plain;
    u8 theta = sFieldScanline.phase;

    GetCameraOffsetWithPan(&x, &y);
    plain = (u16)x | ((u32)(u16)y << 16);
    for (line = 0; line < DISPLAY_HEIGHT; line++, buffer += WAVE_WORDS_PER_LINE)
    {
        value = plain;
        if (line >= wave->top && line < wave->bottom)
        {
            value = (u16)(x + ((gSineTable[theta] * wave->amplitude) >> 8)) | ((u32)(u16)y << 16);
            theta += wave->lineStep;
        }
        buffer[0] = value;
        buffer[1] = value;
        buffer[2] = value;
    }
}

static void UpdateGradientShades(void)
{
    u16 color = gPlttBufferFaded[sFieldScanline.gradient.palIndex];
    u8 profile = GetLcdProfile();
    u32 i;

    if (color == sFieldScanline.sourceColor && profile == sFieldScanline.lcdProfile)
        return;
    sFieldScanline.sourceColor = color;
    sFieldScanline.lcdProfile = profile;
    for (i = 0; i < GRADIENT_SHADES; i++)
    {
        s32 r = GET_R(color), g = GET_G(color), b = GET_B(color);
        u16 to = sFieldScanline.gradient.bottomColor;

        r += ((GET_R(to) - r) * (s32)i) / (GRADIENT_SHADES - 1);
        g += ((GET_G(to) - g) * (s32)i) / (GRADIENT_SHADES - 1);
        b += ((GET_B(to) - b) * (s32)i) / (GRADIENT_SHADES - 1);
        sFieldScanline.shades[i] = ApplyLcdProfile(RGB(r, g, b));
    }
}

static void WriteGradientBuffer(u16 *buffer)
{
    const struct FieldScanlineGradient *gradient = &sFieldScanline.gradient;
    u32 line, height = gradient->bottom - gradient->top;

    for (line = 0; line < DISPLAY_HEIGHT; line++)
    {
        u32 shade = 0;

        if (line >= gradient->bottom)
            shade = GRADIENT_SHADES - 1;
        else if (line >= gradient->top && height > 1)
            shade = (line - gradient->top) * (GRADIENT_SHADES - 1) / (height - 1);
        buffer[line] = sFieldScanline.shades[shade];
    }
}

static void WriteBuffer(u8 buffer)
{
    if (sFieldScanline.effect == FIELD_SCANLINE_GRADIENT)
    {
        UpdateGradientShades();
        WriteGradientBuffer(gScanlineEffectRegBuffers[buffer]);
    }
    else
    {
        WriteWaveBuffer((u32 *)gScanlineEffectRegBuffers[buffer]);
    }
}

static bool8 Start(u8 effect)
{
    struct ScanlineEffectParams params;

    sFieldScanline.effect = effect;
    sFieldScanline.phase = 0;
    sFieldScanline.sourceColor = ~gPlttBufferFaded[sFieldScanline.gradient.palIndex];
    ScanlineEffect_Stop();
    ScanlineEffect_Clear();
    WriteBuffer(0);
    WriteBuffer(1);
    params.dmaDest = GetEffectDest();
    params.dmaControl = GetEffectControl();
    params.initState = 1;
    params.unused9 = 0;
    ScanlineEffect_SetParams(params);
    return TRUE;
}

bool8 FieldScanline_StartWave(u8 effect, const struct FieldScanlineWave *wave)
{
    if (!CanStart() || (effect != FIELD_SCANLINE_HEAT_HAZE && effect != FIELD_SCANLINE_WATER))
        return FALSE;
    sFieldScanline.wave = *wave;
    if (sFieldScanline.wave.bottom > DISPLAY_HEIGHT)
        sFieldScanline.wave.bottom = DISPLAY_HEIGHT;
    return Start(effect);
}

bool8 FieldScanline_StartGradient(const struct FieldScanlineGradient *gradient)
{
    if (!CanStart() || gradient->palIndex >= BG_PLTT_ID(16))
        return FALSE;
    sFieldScanline.gradient = *gradient;
    if (sFieldScanline.gradient.bottom > DISPLAY_HEIGHT)
        sFieldScanline.gradient.bottom = DISPLAY_HEIGHT;
    return Start(FIELD_SCANLINE_GRADIENT);
}

void FieldScanline_Stop(void)
{
    if (OwnsDma0())
    {
        ScanlineEffect_Stop();
        ScanlineEffect_Clear();
    }
    sFieldScanline.effect = FIELD_SCANLINE_NONE;
}

u8 FieldScanline_GetEffect(void)
{
    if (!OwnsDma0())
        sFieldScanline.effect = FIELD_SCANLINE_NONE;
    return sFieldScanline.effect;
}

void FieldScanline_Update(void)
{
    if (sFieldScanline.effect == FIELD_SCANLINE_NONE)
        return;
    if (!OwnsDma0())
    {
        sFieldScanline.effect = FIELD_SCANLINE_NONE;
        return;
    }
    sFieldScanline.phase += sFieldScanline.wave.frameStep;
    // The half the next VBlank gives to DMA0.
    WriteBuffer(gScanlineEffect.srcBuffer);
}
