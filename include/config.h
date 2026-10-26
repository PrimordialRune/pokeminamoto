#ifndef GUARD_CONFIG_H
#define GUARD_CONFIG_H

// In the Generation 3 games, Asserts were used in various debug builds.
// Ruby/Sapphire and Emerald do not have these asserts while Fire Red
// still has them in the ROM. This is because the developers forgot
// to define NDEBUG before release, however this has been changed as
// Ruby's actual debug build does not use the AGBPrint features.
#define NDEBUG

// To enable printf debugging, comment out "#define NDEBUG". This allows
// the various AGBPrint functions to be used. (See include/gba/isagbprint.h).
// See below for enabling different pretty printing versions.

#ifndef NDEBUG

#define PRETTY_PRINT_MINI_PRINTF (0)
#define PRETTY_PRINT_LIBC (1)

#define LOG_HANDLER_AGB_PRINT (0)
#define LOG_HANDLER_NOCASH_PRINT (1)
#define LOG_HANDLER_MGBA_PRINT (2)

// Use this switch to choose a handler for pretty printing.
// NOTE: mini_printf supports a custom pretty printing formatter to display preproc encoded strings. (%S)
//       some libc distributions (especially dkp arm-libc) will fail to link pretty printing.
#define PRETTY_PRINT_HANDLER (PRETTY_PRINT_MINI_PRINTF)

// Use this switch to choose a handler for printf output.
// NOTE: These will only work on the respective emulators and should not be used in a productive environment.
//       Some emulators or real hardware might (and is allowed to) crash if they are used.
//       AGB_PRINT is supported on respective debug units.

#define LOG_HANDLER (LOG_HANDLER_MGBA_PRINT)
#endif

#define ENGLISH

#ifdef ENGLISH
#define UNITS_IMPERIAL
#define CHAR_DEC_SEPARATOR CHAR_PERIOD // Period is used as a decimal separator only in the UK and the US.
#else
#define UNITS_METRIC
#define CHAR_DEC_SEPARATOR CHAR_COMMA
#endif

// Fixes the bugs identified in vanilla (see every #ifdef BUGFIX)
#define BUGFIX

// Save only the flash sectors whose data changed, copy-on-write with a commit
// record written last (src/save.c, above WriteSaveData), instead of Emerald's
// two save slots that rewrite all 13 sectors every save. Comment it out to go
// back to the two slots; each scheme reads the other's saves as incompatible.
#define SAVE_COPY_ON_WRITE

// Rayquaza cutscene, Mt. Chimney cable car, Mirage Tower collapse: their maps are gone
#define FREE_HOENN_SET_PIECES

// Debug menus (overworld: hold R + START, battle: hold L + SELECT). Every modern
// build has them except `make release`, which defines RELEASE. See include/debug_menu.h.
#if !defined(RELEASE)
#define DEBUG_MENUS
#endif

// Various undefined behavior bugs may or may not prevent compilation with
// newer compilers. So always fix them when using a modern compiler.
#ifndef UBFIX
#define UBFIX
#endif

#endif // GUARD_CONFIG_H
