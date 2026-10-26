#ifndef GUARD_FIELD_SCANLINE_H
#define GUARD_FIELD_SCANLINE_H

// HBlank-DMA effects for the overworld, on top of scanline_effect.c. They use
// DMA0 and gScanlineEffectRegBuffers, which the Flash circle (dark caves, the
// Battle Pyramid), the orb effect and battle transitions also use, so only one
// can run: a start is refused while DMA0 is taken, Flash stops these effects
// first, and an effect that finds DMA0 taken over stops itself.

enum
{
    FIELD_SCANLINE_NONE,
    FIELD_SCANLINE_HEAT_HAZE,   // fast, small horizontal shimmer (deserts, lava, fire)
    FIELD_SCANLINE_WATER,       // slow, wide horizontal wobble (underwater, reflections in a pond)
    FIELD_SCANLINE_GRADIENT,    // one BG color shaded from the top of a band to its bottom (sky, deep water)
};

struct FieldScanlineWave
{
    u8 top;          // first screen line
    u8 bottom;       // one past the last line
    u8 amplitude;    // pixels
    u8 lineStep;     // sine table step per line (256 = one period)
    u8 frameStep;    // sine table step per frame
};

struct FieldScanlineGradient
{
    u8 top;
    u8 bottom;
    u8 palIndex;     // BG palette entry, as in BG_PLTT_ID(pal) + color
    u16 bottomColor; // the entry's color at the top is its palette color; it blends to this at the bottom
};

bool8 FieldScanline_StartWave(u8 effect, const struct FieldScanlineWave *wave);
bool8 FieldScanline_StartGradient(const struct FieldScanlineGradient *gradient);
void FieldScanline_Stop(void);
u8 FieldScanline_GetEffect(void);
// End of every overworld frame, after the camera has moved.
void FieldScanline_Update(void);

#endif // GUARD_FIELD_SCANLINE_H
