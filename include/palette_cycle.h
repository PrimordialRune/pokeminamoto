#ifndef GUARD_PALETTE_CYCLE_H
#define GUARD_PALETTE_CYCLE_H

// Palette cycling for map tilesets: colors inside a palette that is already
// loaded change on a timer (water shimmer, flowers, lit windows at night). It
// costs no VRAM, no palette slots and no tile DMA, just a few palette writes.

enum
{
    PALETTE_CYCLE_ROTATE,  // the colors [entry, entry + count) rotate one place per step
    PALETTE_CYCLE_COLORS,  // the color at entry takes colors[0 .. count - 1] in turn
};

struct PaletteCycle
{
    const struct Tileset *tileset; // runs while this tileset is the map's primary or secondary
    u8 palette;                    // BG palette, 0 .. NUM_PALS_TOTAL - 1
    u8 entry;
    u8 count;
    u8 mode;
    u8 period;                     // frames per step
    u8 timeOfDayMask;              // 1 << TIME_OF_DAY_*, 0 = all day
    const u16 *colors;             // PALETTE_CYCLE_COLORS, final colors (time of day tint already in)
};

// Called by the tileset animation code (tileset_anims.c).
void PaletteCycles_Init(void);
void PaletteCycles_Update(void);
// The map palettes were loaded again (time of day): put the current colors back.
void PaletteCycles_Refresh(void);

#endif // GUARD_PALETTE_CYCLE_H
