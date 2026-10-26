#ifndef GUARD_FIELD_BLEND_H
#define GUARD_FIELD_BLEND_H

// Hardware blending in the overworld: see-through ghost sprites and lights cut
// out of the dark with the OBJ window. Both cost no CPU per frame and no
// palette slots (field_blend.c has the register details).

// Ghosts: an object event drawn semi-transparent (Pokémon Tower). The alpha
// is BLDALPHA, which is global and shared with weather (clouds, fog), so a map
// picks one: FieldBlend_SetGhostAlpha only applies when the weather isn't
// blending sprites of its own.
void FieldBlend_SetObjectEventGhostly(u8 objectEventId, bool8 ghostly);
void FieldBlend_SetGhostAlpha(u8 eva, u8 evb);
void FieldBlend_EnableGhosts(bool8 enable);

// Lights: while on, everything but text boxes is darkened by `darkness`
// (0-16, BLDY) except inside light sprites (OBJ window): a lantern around the
// player, cones under lamps and lit windows. Refused in Flash caves and the
// Battle Pyramid, which use WIN0/WIN1 for their circle.
enum
{
    FIELD_LIGHT_CIRCLE,   // 64x64
    FIELD_LIGHT_CONE,     // 32x64, from a lamp at the top
};

bool8 FieldLight_Begin(u8 darkness);
void FieldLight_End(void);
bool8 FieldLight_IsActive(void);
// A light that follows the player, or one fixed at map coordinates (with MAP_OFFSET).
u8 FieldLight_AddOnPlayer(u8 shape);
u8 FieldLight_AddAtMapCoords(u8 shape, s16 x, s16 y);

#endif // GUARD_FIELD_BLEND_H
