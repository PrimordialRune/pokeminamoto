#ifndef GUARD_DATA_H
#define GUARD_DATA_H

#include "constants/moves.h"

#define SPECIES_SHINY_TAG 500

#define MAX_TRAINER_ITEMS 4

#define TRAINER_PIC_WIDTH 64
#define TRAINER_PIC_HEIGHT 64
#define TRAINER_PIC_SIZE (TRAINER_PIC_WIDTH * TRAINER_PIC_HEIGHT / 2)

// Red and Leaf's back pics have 5 frames, but this is presumably irrelevant in the places this is used.
#define MAX_TRAINER_PIC_FRAMES 4

enum {
    BATTLER_AFFINE_NORMAL,
    BATTLER_AFFINE_EMERGE,
    BATTLER_AFFINE_RETURN,
};

struct MonCoords
{
    // This would use a bitfield, but some function
    // uses it as a u8 and casting won't match.
    u8 size; // u8 width:4, height:4;
    u8 y_offset;
};

#define MON_COORDS_SIZE(width, height) (DIV_ROUND_UP(width, 8) << 4 | DIV_ROUND_UP(height, 8))
#define GET_MON_COORDS_WIDTH(size) ((size >> 4) * 8)
#define GET_MON_COORDS_HEIGHT(size) ((size & 0xF) * 8)

// One Pokémon of a trainer's party. src/data/trainers.h is generated from
// src/data/trainers.party by tools/trainerproc, which documents every field.
// All of pokeemerald-expansion's fields are here. CreateNPCTrainerParty applies
// nickname, ev, iv, moves, species, heldItem, ability, lvl, ball, friendship,
// nature, gender and isShiny; teraType, the Dynamax fields and tags are stored
// for features this engine does not have yet.
struct TrainerMon
{
    const u8 *nickname;
    const u8 *ev;                   // TRAINER_PARTY_EVS; NULL = no EVs
    u32 iv;                         // TRAINER_PARTY_IVS
    u16 moves[MAX_MON_MOVES];       // all MOVE_NONE = the moves learned by level-up
    u16 species;
    u16 heldItem;
    u16 ability;                    // ABILITY_NONE = the slot the personality picks
    u8 lvl;
    u8 ball;                        // ITEM_*_BALL; ITEM_NONE = Poké Ball
    u8 friendship;                  // 0 = the species' base friendship
    u8 nature:5;                    // TRAINER_MON_NATURE_DEFAULT = from the personality
    u8 gender:2;                    // TRAINER_MON_RANDOM_GENDER, _MALE or _FEMALE
    u8 isShiny:1;
    u8 teraType:5;
    u8 gigantamaxFactor:1;
    u8 shouldUseDynamax:1;
    u8 padding1:1;
    u8 dynamaxLevel:4;
    u8 padding2:4;
    u32 tags;
};

// EVs and IVs in the order HP, Attack, Defense, Speed, Sp. Attack, Sp. Defense
#define TRAINER_PARTY_EVS(hp, atk, def, speed, spatk, spdef) \
    ((const u8[NUM_STATS]){hp, atk, def, speed, spatk, spdef})
#define TRAINER_PARTY_IVS(hp, atk, def, speed, spatk, spdef) \
    ((hp) | ((atk) << 5) | ((def) << 10) | ((speed) << 15) | ((spatk) << 20) | ((spdef) << 25))
#define TRAINER_PARTY_IV(iv, stat) (((iv) >> ((stat) * 5)) & MAX_PER_STAT_IVS)

struct Trainer
{
    /*0x00*/ u8 trainerClass;
    /*0x01*/ u8 encounterMusic_gender; // last bit is gender
    /*0x02*/ u8 trainerPic;
    /*0x03*/ u8 trainerName[TRAINER_NAME_LENGTH + 1];
    /*0x0E*/ u16 items[MAX_TRAINER_ITEMS];
    /*0x16*/ bool8 doubleBattle;
    /*0x17*/ u8 partySize;
    /*0x18*/ u32 aiFlags;
    /*0x1C*/ const struct TrainerMon *party;
    // Minamoto trainer data (Talk lines, a starting Trainer State, TP rules)
    // goes here once docs/design defines it; see tools/trainerproc/README.md.
};

#define TRAINER_ENCOUNTER_MUSIC(trainer) ((gTrainers[trainer].encounterMusic_gender & 0x7F))

extern const u16 gMinigameDigits_Pal[];
extern const u32 gMinigameDigits_Gfx[];

extern const struct SpriteFrameImage gBattlerPicTable_PlayerLeft[];
extern const struct SpriteFrameImage gBattlerPicTable_OpponentLeft[];
extern const struct SpriteFrameImage gBattlerPicTable_PlayerRight[];
extern const struct SpriteFrameImage gBattlerPicTable_OpponentRight[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Brendan[];
extern const struct SpriteFrameImage gTrainerBackPicTable_May[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Red[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Leaf[];
extern const struct SpriteFrameImage gTrainerBackPicTable_RubySapphireBrendan[];
extern const struct SpriteFrameImage gTrainerBackPicTable_RubySapphireMay[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Wally[];
extern const struct SpriteFrameImage gTrainerBackPicTable_Steven[];

extern const union AffineAnimCmd *const gAffineAnims_BattleSpritePlayerSide[];
extern const union AffineAnimCmd *const gAffineAnims_BattleSpriteOpponentSide[];
extern const union AffineAnimCmd *const gAffineAnims_BattleSpriteContest[];

extern const union AnimCmd *const gAnims_MonPic[];
extern const struct MonCoords gMonFrontPicCoords[];
extern const struct CompressedSpriteSheet gMonStillFrontPicTable[];
extern const struct MonCoords gMonBackPicCoords[];
extern const struct CompressedSpriteSheet gMonBackPicTable[];
extern const struct CompressedSpritePalette gMonPaletteTable[];
extern const struct CompressedSpritePalette gMonShinyPaletteTable[];
extern const union AnimCmd *const *const gTrainerFrontAnimsPtrTable[];
extern const struct MonCoords gTrainerFrontPicCoords[];
extern const struct CompressedSpriteSheet gTrainerFrontPicTable[];
extern const struct CompressedSpritePalette gTrainerFrontPicPaletteTable[];
extern const union AnimCmd *const *const gTrainerBackAnimsPtrTable[];
extern const struct MonCoords gTrainerBackPicCoords[];
extern const struct CompressedSpriteSheet gTrainerBackPicTable[]; // functionally unused
extern const struct CompressedSpritePalette gTrainerBackPicPaletteTable[];

extern const u8 gEnemyMonElevation[NUM_SPECIES];

extern const union AnimCmd *const *const gMonFrontAnimsPtrTable[];
extern const struct CompressedSpriteSheet gMonFrontPicTable[];

extern const struct Trainer gTrainers[];
extern const u8 gTrainerClassNames[][13];
extern const u8 gSpeciesNames[][POKEMON_NAME_LENGTH + 1];

#endif // GUARD_DATA_H
