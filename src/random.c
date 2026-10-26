#include "global.h"
#include "random.h"

// IWRAM common
COMMON_DATA u32 gRngValue = 0;
COMMON_DATA u32 gRng2Value = 0;

// Mulberry32 (Tommy Ettinger): a Weyl-sequence counter whose output is scrambled.
// It keeps vanilla's 4-byte state, so seeding, saves, link and recorded battles still
// see a u32, but unlike the vanilla LCG every output bit is usable and consecutive
// results aren't correlated. The period is 2^32, the same as vanilla.
static inline u32 Mulberry32(u32 *state)
{
    u32 z = (*state += 0x6D2B79F5);
    z = (z ^ (z >> 15)) * (z | 1);
    z ^= z + (z ^ (z >> 7)) * (z | 61);
    return z ^ (z >> 14);
}

u16 Random(void)
{
    return Mulberry32(&gRngValue) >> 16;
}

u32 Random32(void)
{
    return Mulberry32(&gRngValue);
}

void SeedRng(u16 seed)
{
    gRngValue = seed;
}

void SeedRng2(u16 seed)
{
    gRng2Value = seed;
}

u16 Random2(void)
{
    return Mulberry32(&gRng2Value) >> 16;
}
