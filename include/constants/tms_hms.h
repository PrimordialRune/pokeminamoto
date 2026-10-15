#ifndef GUARD_CONSTANTS_TMS_HMS_H
#define GUARD_CONSTANTS_TMS_HMS_H

#define FOREACH_TM(F) \
    F(MEGA_PUNCH) \
    F(RAZOR_WIND) \
    F(SWORDS_DANCE) \
    F(WHIRLWIND) \
    F(MEGA_KICK) \
    F(TOXIC) \
    F(HORN_DRILL) \
    F(BODY_SLAM) \
    F(TAKE_DOWN) \
    F(DOUBLE_EDGE) \
    F(BUBBLE_BEAM) \
    F(WATER_GUN) \
    F(ICE_BEAM) \
    F(BLIZZARD) \
    F(HYPER_BEAM) \
    F(PAY_DAY) \
    F(SUBMISSION) \
    F(COUNTER) \
    F(SEISMIC_TOSS) \
    F(RAGE) \
    F(MEGA_DRAIN) \
    F(SOLAR_BEAM) \
    F(DRAGON_RAGE) \
    F(THUNDERBOLT) \
    F(THUNDER) \
    F(EARTHQUAKE) \
    F(FISSURE) \
    F(DIG) \
    F(PSYCHIC) \
    F(TELEPORT) \
    F(MIMIC) \
    F(DOUBLE_TEAM) \
    F(REFLECT) \
    F(BIDE) \
    F(METRONOME) \
    F(SELF_DESTRUCT) \
    F(EGG_BOMB) \
    F(FIRE_BLAST) \
    F(SWIFT) \
    F(SKULL_BASH) \
    F(SOFT_BOILED) \
    F(DREAM_EATER) \
    F(SKY_ATTACK) \
    F(REST) \
    F(THUNDER_WAVE) \
    F(PSYWAVE) \
    F(EXPLOSION) \
    F(ROCK_SLIDE) \
    F(TRI_ATTACK) \
    F(SUBSTITUTE)

#define FOREACH_HM(F) \
    F(CUT) \
    F(FLY) \
    F(SURF) \
    F(STRENGTH) \
    F(FLASH) \
    F(ROCK_SMASH) \
    F(WATERFALL) \
    F(DIVE)

#define FOREACH_TMHM(F) \
    FOREACH_TM(F) \
    FOREACH_HM(F)

#endif
