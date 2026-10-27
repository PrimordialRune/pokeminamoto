/* Shadow of include/gba/defines.h: IWRAM_CODE uses target("arm"), which x86-64 gcc rejects. */
#include "../../../../../include/gba/defines.h"
#undef IWRAM_CODE
#define IWRAM_CODE
/* video memory lives in host arrays so code that writes it directly (FillAroundBattleWindows ...) is harmless */
#include <stdint.h>
extern unsigned char gHbVram[0x18000], gHbPltt[0x400], gHbOam[0x400];
#undef PLTT
#undef VRAM
#undef OAM
#define PLTT ((uintptr_t)gHbPltt)
#define VRAM ((uintptr_t)gHbVram)
#define OAM  ((uintptr_t)gHbOam)
