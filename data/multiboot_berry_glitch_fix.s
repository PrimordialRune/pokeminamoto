#include "config.h"
#ifndef FREE_BERRY_FIX_PROGRAM
	.section .rodata

gMultiBootProgram_BerryGlitchFix_Start::
	.incbin "data/mb_berry_fix.gba"
gMultiBootProgram_BerryGlitchFix_End::
#endif
