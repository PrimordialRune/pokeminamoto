#include "global.h"
#include "battle_pyramid.h"
#include "event_object_movement.h"
#include "field_blend.h"
#include "field_weather.h"
#include "gpu_regs.h"
#include "overworld.h"
#include "sprite.h"
#include "constants/weather.h"

// The overworld sets up blending once (overworld.c InitOverworldGraphicsRegisters):
// BLDCNT = alpha blend with BG1-3 and OBJ as the second target and no first
// target, so only semi-transparent sprites blend, with BLDALPHA (13/7, later
// set by the weather). WIN0 and WIN1 cover the whole screen and show
// everything; the Flash circle narrows WIN0 per scanline (DMA0) in dark caves.
//
// Ghosts use that as it is: a semi-transparent sprite is always a first target
// and always alpha-blends, whatever BLDCNT's effect is. The windows' color
// effect bits must be on where it is drawn, which FieldBlend_EnableGhosts does.
//
// Lights switch WIN0/WIN1 off and the OBJ window on: sprites in OBJ window
// mode aren't drawn, their opaque pixels mark a region. BLDCNT darkens BG1-3,
// the backdrop and sprites (BLDY = darkness) outside that region only (WINOUT),
// BG0 (text) never. Semi-transparent sprites keep alpha-blending inside and out.
// The light shapes have a dithered edge, as a window can't be soft.

#define TAG_LIGHT_CIRCLE 0x2E10
#define TAG_LIGHT_CONE   0x2E11
#define MAX_LIGHTS 8

struct FieldLights
{
    bool8 active;
    u8 spriteIds[MAX_LIGHTS];
    u16 dispcnt;
    u16 winin;
    u16 winout;
    u16 bldcnt;
    u16 bldy;
};

static EWRAM_DATA struct FieldLights sFieldLights = {0};

static const u32 sLightCircle_Gfx[] = INCBIN_U32("graphics/field_effects/pics/light_circle.4bpp");
static const u32 sLightCone_Gfx[] = INCBIN_U32("graphics/field_effects/pics/light_cone.4bpp");

static const struct SpriteSheet sLightSheets[] =
{
    [FIELD_LIGHT_CIRCLE] = {sLightCircle_Gfx, sizeof(sLightCircle_Gfx), TAG_LIGHT_CIRCLE},
    [FIELD_LIGHT_CONE]   = {sLightCone_Gfx, sizeof(sLightCone_Gfx), TAG_LIGHT_CONE},
};

static const struct OamData sOam_LightCircle =
{
    .objMode = ST_OAM_OBJ_WINDOW,
    .shape = SPRITE_SHAPE(64x64),
    .size = SPRITE_SIZE(64x64),
};

static const struct OamData sOam_LightCone =
{
    .objMode = ST_OAM_OBJ_WINDOW,
    .shape = SPRITE_SHAPE(32x64),
    .size = SPRITE_SIZE(32x64),
};

static void SpriteCB_LightOnPlayer(struct Sprite *sprite);

static const struct SpriteTemplate sLightTemplates[] =
{
    [FIELD_LIGHT_CIRCLE] = {
        .tileTag = TAG_LIGHT_CIRCLE,
        .paletteTag = TAG_NONE,
        .oam = &sOam_LightCircle,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
    [FIELD_LIGHT_CONE] = {
        .tileTag = TAG_LIGHT_CONE,
        .paletteTag = TAG_NONE,
        .oam = &sOam_LightCone,
        .anims = gDummySpriteAnimTable,
        .images = NULL,
        .affineAnims = gDummySpriteAffineAnimTable,
        .callback = SpriteCallbackDummy,
    },
};

// --- Ghosts -------------------------------------------------------------

void FieldBlend_SetObjectEventGhostly(u8 objectEventId, bool8 ghostly)
{
    struct Sprite *sprite = &gSprites[gObjectEvents[objectEventId].spriteId];

    sprite->oam.objMode = ghostly ? ST_OAM_OBJ_BLEND : ST_OAM_OBJ_NORMAL;
}

static bool32 IsWeatherBlendingSprites(void)
{
    switch (GetCurrentWeather())
    {
    case WEATHER_SUNNY_CLOUDS:
    case WEATHER_FOG_HORIZONTAL:
    case WEATHER_FOG_DIAGONAL:
    case WEATHER_VOLCANIC_ASH:
    case WEATHER_SANDSTORM:
    case WEATHER_UNDERWATER_BUBBLES:
        return TRUE;
    }
    return FALSE;
}

void FieldBlend_SetGhostAlpha(u8 eva, u8 evb)
{
    if (!IsWeatherBlendingSprites())
        Weather_SetBlendCoeffs(eva, evb);
}

void FieldBlend_EnableGhosts(bool8 enable)
{
    if (sFieldLights.active)
        return;     // WIN0/WIN1 are off; WINOUT already lets them blend
    if (enable)
        SetGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR | WININ_WIN1_CLR);
    else
        ClearGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR | WININ_WIN1_CLR);
}

// --- Lights -------------------------------------------------------------

bool8 FieldLight_Begin(u8 darkness)
{
    u32 i;

    if (sFieldLights.active || GetFlashLevel() != 0 || InBattlePyramid_())
        return FALSE;
    sFieldLights.active = TRUE;
    for (i = 0; i < MAX_LIGHTS; i++)
        sFieldLights.spriteIds[i] = MAX_SPRITES;
    sFieldLights.dispcnt = GetGpuReg(REG_OFFSET_DISPCNT);
    sFieldLights.winin = GetGpuReg(REG_OFFSET_WININ);
    sFieldLights.winout = GetGpuReg(REG_OFFSET_WINOUT);
    sFieldLights.bldcnt = GetGpuReg(REG_OFFSET_BLDCNT);
    sFieldLights.bldy = GetGpuReg(REG_OFFSET_BLDY);

    SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR
                               | WINOUT_WINOBJ_BG_ALL | WINOUT_WINOBJ_OBJ);
    SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG1 | BLDCNT_TGT1_BG2 | BLDCNT_TGT1_BG3 | BLDCNT_TGT1_OBJ | BLDCNT_TGT1_BD
                               | BLDCNT_EFFECT_DARKEN
                               | (sFieldLights.bldcnt & (BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ)));
    SetGpuReg(REG_OFFSET_BLDY, min(darkness, 16));
    ClearGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_WIN0_ON | DISPCNT_WIN1_ON);
    SetGpuRegBits(REG_OFFSET_DISPCNT, DISPCNT_OBJWIN_ON);
    return TRUE;
}

void FieldLight_End(void)
{
    u32 i;

    if (!sFieldLights.active)
        return;
    for (i = 0; i < MAX_LIGHTS; i++)
    {
        if (sFieldLights.spriteIds[i] != MAX_SPRITES)
            DestroySprite(&gSprites[sFieldLights.spriteIds[i]]);
    }
    FreeSpriteTilesByTag(TAG_LIGHT_CIRCLE);
    FreeSpriteTilesByTag(TAG_LIGHT_CONE);
    SetGpuReg(REG_OFFSET_DISPCNT, sFieldLights.dispcnt);
    SetGpuReg(REG_OFFSET_WININ, sFieldLights.winin);
    SetGpuReg(REG_OFFSET_WINOUT, sFieldLights.winout);
    SetGpuReg(REG_OFFSET_BLDCNT, sFieldLights.bldcnt);
    SetGpuReg(REG_OFFSET_BLDY, sFieldLights.bldy);
    sFieldLights.active = FALSE;
}

bool8 FieldLight_IsActive(void)
{
    // A map load resets the registers and the sprites under us.
    if (sFieldLights.active && !(GetGpuReg(REG_OFFSET_DISPCNT) & DISPCNT_OBJWIN_ON))
        sFieldLights.active = FALSE;
    return sFieldLights.active;
}

static u8 AddLight(u8 shape, s16 x, s16 y, SpriteCallback callback)
{
    u32 i;
    u8 spriteId;

    if (!sFieldLights.active || shape >= ARRAY_COUNT(sLightTemplates))
        return MAX_SPRITES;
    for (i = 0; i < MAX_LIGHTS && sFieldLights.spriteIds[i] != MAX_SPRITES; i++)
        ;
    if (i == MAX_LIGHTS)
        return MAX_SPRITES;
    if (GetSpriteTileStartByTag(sLightSheets[shape].tag) == TAG_NONE)
        LoadSpriteSheet(&sLightSheets[shape]);
    spriteId = CreateSprite(&sLightTemplates[shape], x, y, 0);
    if (spriteId == MAX_SPRITES)
        return MAX_SPRITES;
    gSprites[spriteId].coordOffsetEnabled = TRUE;
    gSprites[spriteId].callback = callback;
    sFieldLights.spriteIds[i] = spriteId;
    return spriteId;
}

static void SpriteCB_LightOnPlayer(struct Sprite *sprite)
{
    struct Sprite *player = &gSprites[gPlayerAvatar.spriteId];

    sprite->x = player->x;
    sprite->y = player->y;
    sprite->x2 = player->x2;
    sprite->y2 = player->y2;
    sprite->coordOffsetEnabled = player->coordOffsetEnabled;
}

u8 FieldLight_AddOnPlayer(u8 shape)
{
    u8 spriteId = AddLight(shape, 0, 0, SpriteCB_LightOnPlayer);

    if (spriteId != MAX_SPRITES)
        SpriteCB_LightOnPlayer(&gSprites[spriteId]);
    return spriteId;
}

u8 FieldLight_AddAtMapCoords(u8 shape, s16 x, s16 y)
{
    // Field effects' convention: the center of the metatile, top-left origin.
    SetSpritePosToOffsetMapCoords(&x, &y, 8, 8);
    return AddLight(shape, x, y, SpriteCallbackDummy);
}
