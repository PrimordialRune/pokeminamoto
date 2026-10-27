/* HostPtr: map 32-bit pseudo addresses stored in assembled battle script bytecode to host pointers */
#include <stdint.h>
#include <string.h>
extern const unsigned char hb_battle_scripts_1[], hb_battle_scripts_2[], hb_battle_ai_scripts[];
extern const unsigned hb_battle_scripts_1_size, hb_battle_scripts_2_size, hb_battle_ai_scripts_size;
extern const struct { const char *name; void *ptr; } hb_ext[];
extern const unsigned hb_ext_count;
/* Script operands left as NULL (macro default arg=NULL) read BIOS memory on the GBA; give them a zero page */
static unsigned char sZeroPage[64];
void *HostPtr(unsigned a)
{
    if (a == 0) return sZeroPage;
    if (a >= 0x0F000000u) { unsigned i = (a - 0x0F000000u) >> 12; return i < hb_ext_count ? (char *)hb_ext[i].ptr + (a & 0xFFF) : 0; }
    if (a >= 0x08200000u) return (void *)(hb_battle_ai_scripts + (a - 0x08200000u));
    if (a >= 0x08100000u) return (void *)(hb_battle_scripts_2 + (a - 0x08100000u));
    if (a >= 0x08000000u) return (void *)(hb_battle_scripts_1 + (a - 0x08000000u));
    return (void *)(uintptr_t)a; /* small constants (e.g. item ids used as "pointers") */
}
