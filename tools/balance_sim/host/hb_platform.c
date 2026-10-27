/* Hand-written host replacements for the GBA platform layer. Everything else the battle sources
   reference (graphics, sound, overworld, link) is auto-stubbed by gen.py link and pinned by
   stubs_allowlist.txt. Keep this file small: every function here is a place where the host can
   differ from the ROM (tools/balance_sim/host/oracle.py checks the ones that matter). */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "global.h"
#include "main.h"
#include "battle.h"
#include "constants/global.h"
#include "constants/battle.h"

extern u8 gBattlePartyCurrentOrder[PARTY_SIZE / 2];
#undef CpuSet
#undef CpuFastSet

/* ---- heap: libc, with a table of live blocks so the harness can prove nothing leaks ---- */
#define HB_MAX_BLOCKS 4096
static void *sBlocks[HB_MAX_BLOCKS];
static u32 sBlockSize[HB_MAX_BLOCKS];
static int sBlockCount;
static void Track(void *p, u32 size)
{
    if (p && sBlockCount < HB_MAX_BLOCKS) { sBlocks[sBlockCount] = p; sBlockSize[sBlockCount++] = size; }
}
void *Alloc(u32 size) { void *p = malloc(size); Track(p, size); return p; }
void *AllocZeroed(u32 size) { void *p = calloc(1, size); Track(p, size); return p; }
void Free(void *p)
{
    int i;
    for (i = sBlockCount - 1; i >= 0; i--)
        if (sBlocks[i] == p) { sBlocks[i] = sBlocks[--sBlockCount]; sBlockSize[i] = sBlockSize[sBlockCount]; break; }
    free(p);
}
void MoveSaveBlocks_ResetHeap(void) {}
int HbLiveBlocks(void) { return sBlockCount; }
unsigned HbLiveBytes(void) { unsigned n = 0; int i; for (i = 0; i < sBlockCount; i++) n += sBlockSize[i]; return n; }
__attribute__((visibility("default"))) int hb_live_sizes(unsigned *o, int n) { int i; for (i = 0; i < sBlockCount && i < n; i++) o[i] = sBlockSize[i]; return sBlockCount; }
/* frees every block still live; returns how many */
int HbSweepBlocks(void)
{
    int n = sBlockCount;
    while (sBlockCount > 0) free(sBlocks[--sBlockCount]);
    return n;
}

/* BIOS CpuSet: bit 24 = fill, bit 26 = 32-bit units; count in the low 21 bits.
   Writes to VRAM/IO/palette addresses (below 0x10000000 on the GBA) have no PPU to go to. */
void CpuSet(const void *src, void *dst, u32 ctl)
{
    u32 n = ctl & 0x1FFFFF, unit = (ctl & (1u << 26)) ? 4 : 2, i;
    if ((uintptr_t)dst < 0x10000000ul) return;
    if (ctl & (1u << 24)) { for (i = 0; i < n; i++) memcpy((char *)dst + i * unit, src, unit); }
    else memmove(dst, src, n * unit);
}
void CpuFastSet(const void *src, void *dst, u32 ctl) { CpuSet(src, dst, ctl | (1u << 26)); }
u16 Sqrt(u32 v) { u32 r = 0; while ((r + 1) * (r + 1) <= v) r++; return r; }

/* Copies of logic that lives in graphics/menu files (src/party_menu.c, src/battle_anim_mons.c) */
u8 GetPartyIdFromBattlePartyId(u8 id)
{
    u8 i, j;
    for (j = i = 0; i < (int)ARRAY_COUNT(gBattlePartyCurrentOrder); j++, i++) {
        if ((gBattlePartyCurrentOrder[i] >> 4) != id) { j++; if ((gBattlePartyCurrentOrder[i] & 0xF) == id) return j; }
        else return j;
    }
    return 0;
}
u8 GetBattlerSide(u8 battler) { return GET_BATTLER_SIDE2(battler); }
u8 GetBattlerPosition(u8 battler) { return gBattlerPositions[battler]; }
bool8 IsDoubleBattle(void) { return (gBattleTypeFlags & BATTLE_TYPE_DOUBLE) != 0; }
u8 GetBattlerAtPosition(u8 position)
{
    u8 i;
    for (i = 0; i < gBattlersCount; i++) if (gBattlerPositions[i] == position) break;
    return i;
}
unsigned char gHbVram[0x18000], gHbPltt[0x400], gHbOam[0x400];

/* Headless: every battler is driven by the opponent controller (generated hb_ctrl_gen.c) */
extern void SetControllerToOpponent(void);
void SetControllerToPlayer(void) { SetControllerToOpponent(); }
void SetControllerToPlayerPartner(void) { SetControllerToOpponent(); }
void SetControllerToWally(void) { SetControllerToOpponent(); }
void SetControllerToSafari(void) { SetControllerToOpponent(); }
void SetControllerToLinkOpponent(void) { SetControllerToOpponent(); }
void SetControllerToLinkPartner(void) { SetControllerToOpponent(); }

const u8 gGameVersion = VERSION_EMERALD;
const u8 gGameLanguage = GAME_LANGUAGE;
