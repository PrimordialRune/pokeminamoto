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

// Save-block cleanup: each define below removes save data for a Hoenn or
// e-Reader system that Minamoto never reaches, freeing EWRAM and flash.
// Comment one out to bring that system's data back. Changing any of these
// changes the save layout, so older saves stop loading.
#define FREE_MYSTERY_EVENT_BUFFERS      // ramScript, Mystery Event scripts, record mixing gift item (1120 B)
#define FREE_MYSTERY_GIFT               // Wonder Cards/News and their flags (876 B)
#define FREE_RECORD_MIXING_HALL_RECORDS // Battle Frontier Ranking Hall records (1032 B)
#define FREE_UNION_ROOM_CHAT            // Union Room chat registered texts (210 B)
#define FREE_BATTLE_TOWER_E_READER      // e-Reader Battle Tower trainer (188 B)
#define FREE_EXTRA_SEEN_FLAGS           // the two anti-cheat copies of Pokedex seen flags (104 B)
#define FREE_ENIGMA_BERRY               // e-Reader Enigma Berry (52 B)
#define FREE_GCN_LINK                   // data Colosseum/XD read and write over the GameCube link (45 B)
#define FREE_EASY_CHAT_PHRASES          // editable trainer card profile and link battle lines, now fixed (48 B)
#define FREE_APPRENTICE                 // Battle Tower Apprentice and the 4 saved ones (316 B)

// Debug menus (overworld: hold R + START, battle: hold L + SELECT). Every modern
// build has them except `make release`, which defines RELEASE. See include/debug_menu.h.
#if MODERN && !defined(RELEASE)
#define DEBUG_MENUS
#endif

// Various undefined behavior bugs may or may not prevent compilation with
// newer compilers. So always fix them when using a modern compiler.
#if MODERN || defined(BUGFIX)
#ifndef UBFIX
#define UBFIX
#endif
#endif

#endif // GUARD_CONFIG_H
