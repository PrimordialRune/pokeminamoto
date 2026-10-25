#ifndef GUARD_PALETTE_TONE_H
#define GUARD_PALETTE_TONE_H

// A tone table remaps each 5-bit channel of a color through its own curve.
// Day/night (day_night.c) applies one in place when palettes load; the LCD
// profile applies one on the way to palette RAM (TransferPlttBuffer).
struct ToneTable
{
    u8 r[32];
    u8 g[32];
    u8 b[32];
};

// Builds the 32 entries of a channel from a macro f(x), at compile time.
#define TONE_CHANNEL(f) {                                                   \
    f(0),  f(1),  f(2),  f(3),  f(4),  f(5),  f(6),  f(7),                  \
    f(8),  f(9),  f(10), f(11), f(12), f(13), f(14), f(15),                 \
    f(16), f(17), f(18), f(19), f(20), f(21), f(22), f(23),                 \
    f(24), f(25), f(26), f(27), f(28), f(29), f(30), f(31) }
#define TONE_IDENTITY(x) (x)
#define TONE_CLAMP(x) ((x) < 0 ? 0 : (x) > 31 ? 31 : (x))

static inline u16 ToneColor(const struct ToneTable *tone, u16 color)
{
    return tone->r[color & 0x1F]
         | (tone->g[(color >> 5) & 0x1F] << 5)
         | (tone->b[(color >> 10) & 0x1F] << 10);
}

// Screen the player uses (Options > SCREEN), stored in SaveBlock2.
enum
{
    LCD_PROFILE_IPS,  // backlit IPS screens, emulators, Analogue Pocket: colors as drawn
    LCD_PROFILE_SP,   // GBA SP (front- or backlit): a light gamma lift
    LCD_PROFILE_AGB,  // original GBA, no light: a strong gamma lift
    LCD_PROFILE_COUNT
};

// Called from TransferPlttBuffer: copies gPlttBufferFaded to palette RAM,
// through the LCD curve when one is selected.
void TransferPlttBufferToPltt(void);
u8 GetLcdProfile(void);
u16 ApplyLcdProfile(u16 color);

#endif // GUARD_PALETTE_TONE_H
