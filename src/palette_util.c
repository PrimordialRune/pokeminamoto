#include "global.h"
#include "palette.h"
#include "palette_util.h"
#include "util.h"

// "RouletteFlash" is more accurately a general flashing/fading util
// this file handles fading the palettes for the color/icon selections on the Roulette wheel
// but it also handles the "pulse blend" effect of Mirage Tower

void RouletteFlash_Reset(struct RouletteFlashUtil *flash)
{
    flash->enabled = 0;
    flash->flags = 0;
    memset(&flash->palettes, 0, sizeof(flash->palettes));
}

u8 RouletteFlash_Add(struct RouletteFlashUtil *flash, u8 id, const struct RouletteFlashSettings *settings)
{
    if (id >= ARRAY_COUNT(flash->palettes) || flash->palettes[id].available)
        return 0xFF;

    flash->palettes[id].settings.color = settings->color;
    flash->palettes[id].settings.paletteOffset = settings->paletteOffset;
    flash->palettes[id].settings.numColors = settings->numColors;
    flash->palettes[id].settings.delay = settings->delay;
    flash->palettes[id].settings.unk6 = settings->unk6;
    flash->palettes[id].settings.numFadeCycles = settings->numFadeCycles;
    flash->palettes[id].settings.unk7_5 = settings->unk7_5;
    flash->palettes[id].settings.colorDeltaDir = settings->colorDeltaDir;
    flash->palettes[id].state = 0;
    flash->palettes[id].available = TRUE;
    flash->palettes[id].fadeCycleCounter = 0;
    flash->palettes[id].delayCounter = 0;
    if (flash->palettes[id].settings.colorDeltaDir < 0)
        flash->palettes[id].colorDelta = -1;
    else
        flash->palettes[id].colorDelta = 1;

    return id;
}

static u8 UNUSED RouletteFlash_Remove(struct RouletteFlashUtil *flash, u8 id)
{
    if (id >= ARRAY_COUNT(flash->palettes))
        return 0xFF;
    if (!flash->palettes[id].available)
        return 0xFF;

    memset(&flash->palettes[id], 0, sizeof(flash->palettes[id]));
    return id;
}

static u8 RouletteFlash_FadePalette(struct RouletteFlashPalette *pal)
{
    u8 i;
    u8 returnval;

    for (i = 0; i < pal->settings.numColors; i++)
    {
        struct PlttData *faded =   (struct PlttData *)&gPlttBufferFaded[pal->settings.paletteOffset + i];
        struct PlttData *unfaded = (struct PlttData *)&gPlttBufferUnfaded[pal->settings.paletteOffset + i];

        switch (pal->state)
        {
        case 1:
            // Fade color
            if (faded->r + pal->colorDelta >= 0 && faded->r + pal->colorDelta < 32)
                faded->r += pal->colorDelta;
            if (faded->g + pal->colorDelta >= 0 && faded->g + pal->colorDelta < 32)
                faded->g += pal->colorDelta;
            if (faded->b + pal->colorDelta >= 0 && faded->b + pal->colorDelta < 32)
                faded->b += pal->colorDelta;
            break;
        case 2:
            // Fade back to original color
            if (pal->colorDelta < 0)
            {
                if (faded->r + pal->colorDelta >= unfaded->r)
                    faded->r += pal->colorDelta;
                if (faded->g + pal->colorDelta >= unfaded->g)
                    faded->g += pal->colorDelta;
                if (faded->b + pal->colorDelta >= unfaded->b)
                    faded->b += pal->colorDelta;
            }
            else
            {
                if (faded->r + pal->colorDelta <= unfaded->r)
                    faded->r += pal->colorDelta;
                if (faded->g + pal->colorDelta <= unfaded->g)
                    faded->g += pal->colorDelta;
                if (faded->b + pal->colorDelta <= unfaded->b)
                    faded->b += pal->colorDelta;
            }
            break;
        }
    }
    if ((u32)pal->fadeCycleCounter++ != pal->settings.numFadeCycles)
    {
        returnval = 0;
    }
    else
    {
        pal->fadeCycleCounter = 0;
        pal->colorDelta *= -1;
        if (pal->state == 1)
            pal->state++;
        else
            pal->state--;
        returnval = 1;
    }
    return returnval;
}

static u8 RouletteFlash_FlashPalette(struct RouletteFlashPalette *pal)
{
    u8 i = 0;
    switch (pal->state)
    {
    case 1:
        // Flash to color
        for (; i < pal->settings.numColors; i++)
            gPlttBufferFaded[pal->settings.paletteOffset + i] = pal->settings.color;
        pal->state++;
        break;
    case 2:
        // Restore to original color
        for (; i < pal->settings.numColors; i++)
            gPlttBufferFaded[pal->settings.paletteOffset + i] = gPlttBufferUnfaded[pal->settings.paletteOffset + i];
        pal->state--;
        break;
    }
    return 1;
}

void RouletteFlash_Run(struct RouletteFlashUtil *flash)
{
    u8 i = 0;

    if (flash->enabled)
    {
        for (i = 0; i < ARRAY_COUNT(flash->palettes); i++)
        {
            if ((flash->flags >> i) & 1)
            {
                if (--flash->palettes[i].delayCounter == (u8)-1)
                {
                    if (flash->palettes[i].settings.color & FLASHUTIL_USE_EXISTING_COLOR)
                        RouletteFlash_FadePalette(&flash->palettes[i]);
                    else
                        RouletteFlash_FlashPalette(&flash->palettes[i]);

                    flash->palettes[i].delayCounter = flash->palettes[i].settings.delay;
                }
            }
        }
    }
}

void RouletteFlash_Enable(struct RouletteFlashUtil *flash, u16 flags)
{
    u8 i = 0;

    flash->enabled++;
    for (i = 0; i < ARRAY_COUNT(flash->palettes); i++)
    {
        if ((flags >> i) & 1)
        {
            if (flash->palettes[i].available)
            {
                flash->flags |= 1 << i;
                flash->palettes[i].state = 1;
            }
        }
    }
}

void RouletteFlash_Stop(struct RouletteFlashUtil *flash, u16 flags)
{
    u8 i;

    for (i = 0; i < ARRAY_COUNT(flash->palettes); i++)
    {
        if ((flash->flags >> i) & 1)
        {
            if (flash->palettes[i].available)
            {
                if ((flags >> i) & 1)
                {
                    u32 offset = flash->palettes[i].settings.paletteOffset;
                    u16 *faded = &gPlttBufferFaded[offset];
                    u16 *unfaded = &gPlttBufferUnfaded[offset];
                    memcpy(faded, unfaded, flash->palettes[i].settings.numColors * 2);
                    flash->palettes[i].state = 0;
                    flash->palettes[i].fadeCycleCounter = 0;
                    flash->palettes[i].delayCounter = 0;
                    if (flash->palettes[i].settings.colorDeltaDir < 0)
                        flash->palettes[i].colorDelta = -1;
                    else
                        flash->palettes[i].colorDelta = 1;
                }
            }
        }
    }

    if (flags == 0xFFFF)
    {
        // Stopped all
        flash->enabled = 0;
        flash->flags = 0;
    }
    else
    {
        flash->flags &= ~flags;
    }
}






void UnmarkUsedPulseBlendPalettes(struct PulseBlend *pulseBlend, u16 pulseBlendPaletteSelector, u8 multiSelection)
{
    u16 i;
    struct PulseBlendPalette *pulseBlendPalette;
    u8 j = 0;

    if (!multiSelection)
    {
        pulseBlendPalette = &pulseBlend->pulseBlendPalettes[pulseBlendPaletteSelector & 0xF];
        if (!pulseBlendPalette->available && pulseBlendPalette->inUse)
        {
            if (pulseBlendPalette->pulseBlendSettings.restorePaletteOnUnload)
            {
                for (i = pulseBlendPalette->pulseBlendSettings.paletteOffset; i < pulseBlendPalette->pulseBlendSettings.paletteOffset + pulseBlendPalette->pulseBlendSettings.numColors; i++)
                    gPlttBufferFaded[i] = gPlttBufferUnfaded[i];
            }

            pulseBlendPalette->available = 1;
            pulseBlend->usedPulseBlendPalettes &= ~(1 << j);
        }
    }
    else
    {
        for (j = 0; j < 16; j++)
        {
            pulseBlendPalette = &pulseBlend->pulseBlendPalettes[j];
            if (!(pulseBlendPaletteSelector & 1) || pulseBlendPalette->available || !pulseBlendPalette->inUse)
            {
                pulseBlendPaletteSelector <<= 1;
            }
            else
            {
                if (pulseBlendPalette->pulseBlendSettings.restorePaletteOnUnload)
                {
                    for (i = pulseBlendPalette->pulseBlendSettings.paletteOffset; i < pulseBlendPalette->pulseBlendSettings.paletteOffset + pulseBlendPalette->pulseBlendSettings.numColors; i++)
                        gPlttBufferFaded[i] = gPlttBufferUnfaded[i];
                }

                pulseBlendPalette->available = 1;
                pulseBlend->usedPulseBlendPalettes &= ~(1 << j);
            }
        }
    }
}


// Below used for the Roulette grid
void FillTilemapRect(u16 *dest, u16 value, u8 left, u8 top, u8 width, u8 height)
{
    u16 *_dest;
    u8 i;
    u8 j;
    i = 0;
    dest = &dest[top * 32 + left];
    for (; i < height; i++)
    {
        _dest = dest + i * 32;
        for (j = 0; j < width; j++)
            *_dest++ = value;
    }
}

void SetTilemapRect(u16 *dest, u16 *src, u8 left, u8 top, u8 width, u8 height)
{
    u16 *_dest;
    u16 *_src = src;
    u8 i;
    u8 j;
    i = 0;
    dest = &dest[top * 32 + left];
    for (; i < height; i++)
    {
        _dest = dest + i * 32;
        for (j = 0; j < width; j++)
            *_dest++ = *_src++;
    }
}

static void UNUSED FillTilemapRect_Unused(void *dest, u16 value, u8 left, u8 top, u8 width, u8 height)
{
    u8 i, j;
    u8 x, y;

    for (i = 0, y = top; i < height; i++)
    {
        for (x = left, j = 0; j < width; j++)
        {
            *(u16 *)((dest) + (y * 64 + x * 2)) = value;
            x = (x + 1) % 32;
        }
        y = (y + 1) % 32;
    }
}

static void UNUSED SetTilemapRect_Unused(void *dest, const u16 *src, u8 left, u8 top, u8 width, u8 height)
{
    u8 i, j;
    u8 x, y;
    const u16 *_src;

    for (i = 0, _src = src, y = top; i < height; i++)
    {
        for (x = left, j = 0; j < width; j++)
        {
            *(u16 *)((dest) + (y * 64 + x * 2)) = *(_src++);
            x = (x + 1) % 32;
        }
        y = (y + 1) % 32;
    }
}
