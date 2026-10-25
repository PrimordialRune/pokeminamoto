#include "config.h"
#ifndef FREE_GCN_LINK
	.section .rodata

gMultiBootProgram_PokemonColosseum_Start::
	.incbin "data/mb_colosseum.gba"
gMultiBootProgram_PokemonColosseum_End::
#endif
