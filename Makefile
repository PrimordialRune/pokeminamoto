# GBA rom header
TITLE       := POKEMON EMER
GAME_CODE   := BPEE
MAKER_CODE  := 01
REVISION    := 0
KEEP_TEMPS  ?= 0

# `File name`.gba ('_modern' will be appended to the modern builds)
FILE_NAME := pokeemerald
BUILD_DIR := build

# The only build is arm-none-eabi-gcc (pokeemerald_modern.gba); `make modern` is an
# alias for `make`.
# Release builds leave out the debug menus (see DEBUG_MENUS in include/config.h).
# They build in their own folder and get a _release suffix, so switching between
# release and normal builds never mixes objects.
RELEASE     ?= 0
ifeq (release,$(MAKECMDGOALS))
  RELEASE := 1
endif

# Default make rule
all: rom

# Toolchain selection
TOOLCHAIN := $(DEVKITARM)
# don't use dkP's base_tools anymore
# because the redefinition of $(CC) conflicts
# with when we want to use $(CC) to preprocess files
# thus, manually create the variables for the bin
# files, or use arm-none-eabi binaries on the system
# if dkP is not installed on this system
ifneq (,$(TOOLCHAIN))
  ifneq ($(wildcard $(TOOLCHAIN)/bin),)
    export PATH := $(TOOLCHAIN)/bin:$(PATH)
  endif
endif

PREFIX := arm-none-eabi-
OBJCOPY := $(PREFIX)objcopy
OBJDUMP := $(PREFIX)objdump
AS := $(PREFIX)as
LD := $(PREFIX)ld

EXE :=
ifeq ($(OS),Windows_NT)
  EXE := .exe
endif

# use arm-none-eabi-cpp for macOS
# as macOS's default compiler is clang
# and clang's preprocessor will warn on \u
# when preprocessing asm files, expecting a unicode literal
# we can't unconditionally use arm-none-eabi-cpp
# as installations which install binutils-arm-none-eabi
# don't come with it
CPP := $(PREFIX)cpp

ifeq ($(RELEASE),1)
  RELEASE_SUFFIX := _release
endif
# `make REGRESS=1` adds the test hooks tools/regress drives (include/regress.h).
# It builds in its own folder with a _regress suffix, like release builds.
REGRESS ?= 0
ifeq ($(REGRESS),1)
  RELEASE_SUFFIX := $(RELEASE_SUFFIX)_regress
endif
# `make TEST=1` builds the in-ROM test runner (test/, include/test_runner.h) into its own
# _test folder; `make check` builds it and runs it in gbaemu along with tools/check_data.py.
TEST ?= 0
ifeq ($(TEST),1)
  RELEASE_SUFFIX := $(RELEASE_SUFFIX)_test
  CPPFLAGS_TEST := -DTEST_BUILD
endif
ROM := $(FILE_NAME)_modern$(RELEASE_SUFFIX).gba
OBJ_DIR := $(BUILD_DIR)/modern$(RELEASE_SUFFIX)
ELF := $(ROM:.gba=.elf)
MAP := $(ROM:.gba=.map)
SYM := $(ROM:.gba=.sym)

# Commonly used directories
C_SUBDIR = src
ASM_SUBDIR = asm
DATA_SRC_SUBDIR = src/data
DATA_ASM_SUBDIR = data
SONG_SUBDIR = sound/songs
MID_SUBDIR = sound/songs/midi

C_BUILDDIR = $(OBJ_DIR)/$(C_SUBDIR)
ASM_BUILDDIR = $(OBJ_DIR)/$(ASM_SUBDIR)
DATA_ASM_BUILDDIR = $(OBJ_DIR)/$(DATA_ASM_SUBDIR)
SONG_BUILDDIR = $(OBJ_DIR)/$(SONG_SUBDIR)
MID_BUILDDIR = $(OBJ_DIR)/$(MID_SUBDIR)

SHELL := bash -o pipefail

# Set flags for tools
ASFLAGS := -mcpu=arm7tdmi

INCLUDE_DIRS := include
INCLUDE_CPP_ARGS := $(INCLUDE_DIRS:%=-iquote %)
INCLUDE_SCANINC_ARGS := $(INCLUDE_DIRS:%=-I %)

O_LEVEL ?= 2
CPPFLAGS := $(INCLUDE_CPP_ARGS) -Wno-trigraphs
ifeq ($(RELEASE),1)
  CPPFLAGS += -DRELEASE
endif
ifeq ($(REGRESS),1)
  CPPFLAGS += -DREGRESS_HOOKS
endif
CPPFLAGS += $(CPPFLAGS_TEST)
# Warnings for the modern build, after pokeemerald-expansion's set. Most are reported
# without stopping the build because vanilla code still trips them; the ones listed
# with -Werror= are always real bugs, so they fail it.
MODERN_WARNINGS := -Wall -Wno-strict-aliasing -Wno-attribute-alias -Woverride-init -Wnonnull -Wenum-conversion \
                   -Wno-pointer-sign -Wno-missing-braces -Wno-array-parameter \
                   -Werror=return-type -Werror=implicit-function-declaration -Werror=implicit-int \
                   -Werror=int-conversion -Werror=incompatible-pointer-types -Werror=parentheses
# Stricter checks (-Wundef, array-bounds as errors), on by default; `make STRICT_WARNINGS=0` turns them off
STRICT_WARNINGS ?= 1
ifeq ($(STRICT_WARNINGS),1)
  MODERN_WARNINGS += -Werror=array-bounds
  CPPFLAGS += -Wundef
endif
MODERNCC := $(PREFIX)gcc
PATH_MODERNCC := PATH="$(PATH)" $(MODERNCC)
CC1 := $(shell $(PATH_MODERNCC) --print-prog-name=cc1) -quiet
override CFLAGS += -mthumb -mthumb-interwork -O$(O_LEVEL) -mabi=apcs-gnu -mtune=arm7tdmi -march=armv4t -fno-toplevel-reorder -Wno-pointer-to-int-cast $(MODERN_WARNINGS)
LIBPATH := -L "$(dir $(shell $(PATH_MODERNCC) -mthumb -print-file-name=libgcc.a))" -L "$(dir $(shell $(PATH_MODERNCC) -mthumb -print-file-name=libnosys.a))" -L "$(dir $(shell $(PATH_MODERNCC) -mthumb -print-file-name=libc.a))"
LIB := $(LIBPATH) -lc -lnosys -lgcc -L../../libagbsyscall -lagbsyscall
# Enable debug info if set
ifeq ($(DINFO),1)
  override CFLAGS += -g
endif

# Variable filled out in other make files
AUTO_GEN_TARGETS :=
include make_tools.mk
# Tool executables
GFX       := $(TOOLS_DIR)/gbagfx/gbagfx$(EXE)
WAV2AGB   := $(TOOLS_DIR)/wav2agb/wav2agb$(EXE)
MID       := $(TOOLS_DIR)/mid2agb/mid2agb$(EXE)
SCANINC   := $(TOOLS_DIR)/scaninc/scaninc$(EXE)
PREPROC   := $(TOOLS_DIR)/preproc/preproc$(EXE)
FIX       := $(TOOLS_DIR)/gbafix/gbafix$(EXE)
MAPJSON   := $(TOOLS_DIR)/mapjson/mapjson$(EXE)
JSONPROC  := $(TOOLS_DIR)/jsonproc/jsonproc$(EXE)

PORYSCRIPT     := tools/poryscript/poryscript
PORYSCRIPT_CC  := tools/poryscript/command_config.json
PORYSCRIPT_FC  := tools/poryscript/font_config.json
LANGUAGE       ?= ENGLISH

PORY_SRCS      := $(wildcard data/maps/*/scripts.pory) $(wildcard data/scripts/*.pory)
PORY_INCS      := $(PORY_SRCS:.pory=.inc)

# Per-language map text (docs/engineering/map-text.md): each data/maps/X/text.en.pory,
# with X/text.<code>.pory laid over it for LANGUAGE, compiles to X/text.inc.
LANG_CODE      := $(or $(word 2,$(subst :, ,$(filter $(LANGUAGE):%,ENGLISH:en ITALIAN:it FRENCH:fr GERMAN:de SPANISH:es JAPANESE:ja))),en)
TEXT_DIRS      := $(patsubst %/text.en.pory,%,$(wildcard data/maps/*/text.en.pory))
TEXT_INCS      := $(addsuffix /text.inc,$(TEXT_DIRS))
# Holds the LANGUAGE of the last build, so script text rebuilds when it changes
LANG_STAMP     := $(BUILD_DIR)/language

PERL := perl

MAKEFLAGS += --no-print-directory

# Clear the default suffixes
.SUFFIXES:
# Don't delete intermediate files
.SECONDARY:
# Delete files that weren't built properly
.DELETE_ON_ERROR:

RULES_NO_SCAN += libagbsyscall clean clean-assets tidy generated clean-generated balance-lib balance-check
.PHONY: all rom modern release
.PHONY: $(RULES_NO_SCAN)

infoshell = $(foreach line, $(shell $1 | sed "s/ /__SPACE__/g"), $(info $(subst __SPACE__, ,$(line))))

# Check if we need to scan dependencies based on the chosen rule OR user preference
NODEP ?= 0
# Check if we need to pre-build tools and generate assets based on the chosen rule.
SETUP_PREREQS ?= 1
# Disable dependency scanning for rules that don't need it.
ifneq (,$(MAKECMDGOALS))
  ifeq (,$(filter-out $(RULES_NO_SCAN),$(MAKECMDGOALS)))
    NODEP := 1
    SETUP_PREREQS := 0
  endif
endif

.SHELLSTATUS ?= 0

ifeq ($(SETUP_PREREQS),1)
  # If set on: Default target or a rule requiring a scan
  # Forcibly execute `make tools` since we need them for what we are doing.
  $(foreach line, $(shell $(MAKE) -f make_tools.mk | sed "s/ /__SPACE__/g"), $(info $(subst __SPACE__, ,$(line))))
  ifneq ($(.SHELLSTATUS),0)
    $(error Errors occurred while building tools. See error messages above for more details)
  endif
  # Oh and also generate mapjson sources before we use `SCANINC`.
  $(foreach line, $(shell $(MAKE) generated | sed "s/ /__SPACE__/g"), $(info $(subst __SPACE__, ,$(line))))
  ifneq ($(.SHELLSTATUS),0)
    $(error Errors occurred while generating map-related sources. See error messages above for more details)
  endif
endif

# Collect sources
C_SRCS_IN := $(wildcard $(C_SUBDIR)/*.c $(C_SUBDIR)/*/*.c $(C_SUBDIR)/*/*/*.c)
C_SRCS := $(foreach src,$(C_SRCS_IN),$(if $(findstring .inc.c,$(src)),,$(src)))
ifneq ($(REGRESS),1)
  # Keep the test hooks out entirely, so normal ROMs link exactly as before
  C_SRCS := $(filter-out $(C_SUBDIR)/regress.c,$(C_SRCS))
endif
ifneq ($(TEST),1)
  C_SRCS := $(filter-out $(C_SUBDIR)/test/%,$(C_SRCS))
endif
C_OBJS := $(patsubst $(C_SUBDIR)/%.c,$(C_BUILDDIR)/%.o,$(C_SRCS))

C_ASM_SRCS := $(wildcard $(C_SUBDIR)/*.s $(C_SUBDIR)/*/*.s $(C_SUBDIR)/*/*/*.s)
C_ASM_OBJS := $(patsubst $(C_SUBDIR)/%.s,$(C_BUILDDIR)/%.o,$(C_ASM_SRCS))

ASM_SRCS := $(wildcard $(ASM_SUBDIR)/*.s)
ASM_OBJS := $(patsubst $(ASM_SUBDIR)/%.s,$(ASM_BUILDDIR)/%.o,$(ASM_SRCS))

# get all the data/*.s files EXCEPT the ones with specific rules
REGULAR_DATA_ASM_SRCS := $(filter-out $(DATA_ASM_SUBDIR)/maps.s $(DATA_ASM_SUBDIR)/map_events.s, $(wildcard $(DATA_ASM_SUBDIR)/*.s))

DATA_ASM_SRCS := $(wildcard $(DATA_ASM_SUBDIR)/*.s)
DATA_ASM_OBJS := $(patsubst $(DATA_ASM_SUBDIR)/%.s,$(DATA_ASM_BUILDDIR)/%.o,$(DATA_ASM_SRCS))

SONG_SRCS := $(wildcard $(SONG_SUBDIR)/*.s)
SONG_OBJS := $(patsubst $(SONG_SUBDIR)/%.s,$(SONG_BUILDDIR)/%.o,$(SONG_SRCS))

MID_SRCS := $(wildcard $(MID_SUBDIR)/*.mid)
MID_OBJS := $(patsubst $(MID_SUBDIR)/%.mid,$(MID_BUILDDIR)/%.o,$(MID_SRCS))

OBJS     := $(C_OBJS) $(C_ASM_OBJS) $(ASM_OBJS) $(DATA_ASM_OBJS) $(SONG_OBJS) $(MID_OBJS)
OBJS_REL := $(patsubst $(OBJ_DIR)/%,%,$(OBJS))

SUBDIRS  := $(sort $(dir $(OBJS)))
$(shell mkdir -p $(SUBDIRS))

# Pretend rules that are actually flags defer to `make all`
modern: all
release: all

# Other rules
rom: $(ROM)

# Balance simulator engine (tools/balance_sim/README.md): the real battle code host-compiled into
# build/host/libminasim.so. Needs the generated sources (species, trainers, data) first.
.PHONY: balance-lib balance-check
balance-lib:
	@$(MAKE) -f make_tools.mk > /dev/null
	@$(MAKE) generated > /dev/null
	@$(MAKE) -f tools/balance_sim/host/Makefile

# Fails when the set of auto-stubbed host symbols changes (tools/balance_sim/host/stubs_allowlist.txt)
balance-check: balance-lib
	python3 tools/balance_sim/host/check_stubs.py

# Rule tests: data checks, then the test ROM in gbaemu (tools/test/run_tests.py)
.PHONY: check
check:
	python3 tools/check_data.py
	python3 tools/balance/costcurve.py --check
	$(MAKE) balance-check
	$(MAKE) release TEST=1
	python3 tools/test/run_tests.py $(FILE_NAME)_modern_release_test.gba

syms: $(SYM)

clean: tidy clean-tools clean-generated clean-assets
	@$(MAKE) clean -C libagbsyscall

clean-assets:
	rm -f $(MID_SUBDIR)/*.s
	rm -f $(DATA_ASM_SUBDIR)/layouts/layouts.inc $(DATA_ASM_SUBDIR)/layouts/layouts_table.inc
	rm -f $(DATA_ASM_SUBDIR)/maps/connections.inc $(DATA_ASM_SUBDIR)/maps/events.inc $(DATA_ASM_SUBDIR)/maps/groups.inc $(DATA_ASM_SUBDIR)/maps/headers.inc
	find sound -iname '*.bin' -exec rm {} +
	find . \( -iname '*.1bpp' -o -iname '*.4bpp' -o -iname '*.8bpp' -o -iname '*.gbapal' -o -iname '*.lz' -o -iname '*.rl' -o -iname '*.latfont' -o -iname '*.hwjpnfont' -o -iname '*.fwjpnfont' \) -exec rm {} +
	find $(DATA_ASM_SUBDIR)/maps \( -iname 'connections.inc' -o -iname 'events.inc' -o -iname 'header.inc' \) -exec rm {} +

tidy:
	rm -f $(ROM) $(ELF) $(MAP)
	rm -rf $(OBJ_DIR)
	@$(if $(PORY_INCS),rm -f $(PORY_INCS))
	@$(if $(TEXT_INCS),rm -f $(TEXT_INCS))

# Other rules
include graphics_file_rules.mk
include map_data_rules.mk
include spritesheet_rules.mk
include spritesheet_rules_frlg.mk
include json_data_rules.mk
include generated_data_rules.mk
include audio_rules.mk

# NOTE: Tools must have been built prior (FIXME)
# so you can't really call this rule directly
generated: $(AUTO_GEN_TARGETS)
	@: # Silence the "Nothing to be done for `generated'" message, which some people were confusing for an error.


%.s:   ;
%.png: ;
%.pal: ;
%.wav: ;

%.1bpp:   %.png  ; $(GFX) $< $@
%.4bpp:   %.png  ; $(GFX) $< $@
%.8bpp:   %.png  ; $(GFX) $< $@
%.gbapal: %.pal  ; $(GFX) $< $@
%.gbapal: %.png  ; $(GFX) $< $@
%.lz:     %      ; $(GFX) $< $@
%.rl:     %      ; $(GFX) $< $@

# The pinned binary is downloaded on first use (tools/poryscript/fetch.sh)
$(PORYSCRIPT):
	@tools/poryscript/fetch.sh

%.inc: %.pory $(PORYSCRIPT_CC) $(PORYSCRIPT_FC) $(LANG_STAMP) | $(PORYSCRIPT)
	$(PORYSCRIPT) -i $< -o $@ -cc $(PORYSCRIPT_CC) -fc $(PORYSCRIPT_FC) -lm=false -s LANGUAGE=$(LANGUAGE)

$(LANG_STAMP): FORCE
	@mkdir -p $(@D); [ "$$(cat $@ 2>/dev/null)" = "$(LANGUAGE)" ] || echo "$(LANGUAGE)" > $@

$(foreach d,$(TEXT_DIRS),$(eval $d/text.inc: $d/text.en.pory $(wildcard $d/text.$(LANG_CODE).pory)))
$(TEXT_INCS): %/text.inc: tools/textlang.py $(PORYSCRIPT_CC) $(PORYSCRIPT_FC) $(LANG_STAMP) | $(PORYSCRIPT)
	@python3 tools/textlang.py merge $* $(LANG_CODE) -o $(BUILD_DIR)/text/$(subst /,_,$*).pory
	$(PORYSCRIPT) -i $(BUILD_DIR)/text/$(subst /,_,$*).pory -o $@ -cc $(PORYSCRIPT_CC) -fc $(PORYSCRIPT_FC) -lm=false -s LANGUAGE=$(LANGUAGE)

clean-generated:
	@rm -f $(AUTO_GEN_TARGETS)
	@echo "rm -f <AUTO_GEN_TARGETS>"

$(C_BUILDDIR)/librfu_intr.o: CFLAGS := -mthumb-interwork -O2 -mabi=apcs-gnu -mtune=arm7tdmi -march=armv4t -fno-toplevel-reorder -Wno-pointer-to-int-cast

# Dependency rules (for the *.c & *.s sources to .o files)
# Have to be explicit or else missing files won't be reported.

# As a side effect, they're evaluated immediately instead of when the rule is invoked.
# It doesn't look like $(shell) can be deferred so there might not be a better way (Icedude_907: there is soon).

$(C_BUILDDIR)/%.o: $(C_SUBDIR)/%.c
ifneq ($(KEEP_TEMPS),1)
	@echo "$(CC1) <flags> -o $@ $<"
	@$(CPP) $(CPPFLAGS) $< | $(PREPROC) -i $< charmap.txt | $(CC1) $(CFLAGS) -o - - | cat - <(echo -e ".text\n\t.align\t2, 0") | $(AS) $(ASFLAGS) -o $@ -
else
	@$(CPP) $(CPPFLAGS) $< -o $(C_BUILDDIR)/$*.i
	@$(PREPROC) $(C_BUILDDIR)/$*.i charmap.txt | $(CC1) $(CFLAGS) -o $(C_BUILDDIR)/$*.s
	@echo -e ".text\n\t.align\t2, 0\n" >> $(C_BUILDDIR)/$*.s
	$(AS) $(ASFLAGS) -o $@ $(C_BUILDDIR)/$*.s
endif

$(C_BUILDDIR)/%.d: $(C_SUBDIR)/%.c
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) $<

ifneq ($(NODEP),1)
-include $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.d))
endif

$(ASM_BUILDDIR)/%.o: $(ASM_SUBDIR)/%.s
	$(AS) $(ASFLAGS) -o $@ $<

$(ASM_BUILDDIR)/%.d: $(ASM_SUBDIR)/%.s
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) -I "" $<

ifneq ($(NODEP),1)
-include $(addprefix $(OBJ_DIR)/,$(ASM_SRCS:.s=.d))
endif

$(C_BUILDDIR)/%.o: $(C_SUBDIR)/%.s
	$(PREPROC) $< charmap.txt | $(CPP) $(INCLUDE_SCANINC_ARGS) - | $(PREPROC) -ie $< charmap.txt | $(AS) $(ASFLAGS) -o $@

$(C_BUILDDIR)/%.d: $(C_SUBDIR)/%.s
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) -I "" $<

ifneq ($(NODEP),1)
-include $(addprefix $(OBJ_DIR)/,$(C_ASM_SRCS:.s=.d))
endif

$(DATA_ASM_BUILDDIR)/%.o: $(DATA_ASM_SUBDIR)/%.s
	$(PREPROC) $< charmap.txt | $(CPP) $(INCLUDE_SCANINC_ARGS) $(if $(filter 1,$(RELEASE)),-DRELEASE) - | $(PREPROC) -ie $< charmap.txt | $(AS) $(ASFLAGS) -o $@

$(DATA_ASM_BUILDDIR)/%.d: $(DATA_ASM_SUBDIR)/%.s
	$(SCANINC) -M $@ $(INCLUDE_SCANINC_ARGS) -I "" $<

ifneq ($(NODEP),1)
-include $(addprefix $(OBJ_DIR)/,$(REGULAR_DATA_ASM_SRCS:.s=.d))
# maps.s and map_events.s have their own rules (map_data_rules.mk) but still
# need rebuilding when a constants header they include changes
-include $(OBJ_DIR)/$(DATA_ASM_SUBDIR)/maps.d $(OBJ_DIR)/$(DATA_ASM_SUBDIR)/map_events.d
endif

# Ensure all .pory sources are compiled to .inc before event_scripts.o is assembled
$(DATA_ASM_BUILDDIR)/event_scripts.o: $(PORY_INCS) $(TEXT_INCS)

# Linker script
LD_SCRIPT := ld_script_modern.ld

# Final rules

# A real file target, so an unchanged library doesn't relink the ELF on every make
LIBAGBSYSCALL := libagbsyscall/libagbsyscall.a
libagbsyscall: $(LIBAGBSYSCALL)
$(LIBAGBSYSCALL): libagbsyscall/libagbsyscall.s libagbsyscall/Makefile
	@$(MAKE) -C libagbsyscall TOOLCHAIN=$(TOOLCHAIN)

# Elf from object files
LDFLAGS = -Map ../../$(MAP)
# .SECONDARY above makes make ignore a deleted object when the ELF is newer than
# its sources, so relink (rebuilding them) whenever one is missing
MISSING_OBJS := $(filter-out $(wildcard $(OBJS)),$(OBJS))
.PHONY: FORCE
FORCE:
$(ELF): $(LD_SCRIPT) $(OBJS) $(LIBAGBSYSCALL) $(if $(MISSING_OBJS),FORCE)
	@cd $(OBJ_DIR) && $(LD) $(LDFLAGS) -T ../../$< --print-memory-usage -o ../../$@ $(OBJS_REL) $(LIB) | cat
	@echo "cd $(OBJ_DIR) && $(LD) $(LDFLAGS) -T ../../$< --print-memory-usage -o ../../$@ <objs> <libs> | cat"
	$(FIX) $@ -t"$(TITLE)" -c$(GAME_CODE) -m$(MAKER_CODE) -r$(REVISION) --silent

# Builds the rom from the elf file
$(ROM): $(ELF)
	$(OBJCOPY) -O binary $< $@
	$(FIX) $@ -p --silent

# Symbol file (`make syms`)
$(SYM): $(ELF)
	$(OBJDUMP) -t $< | sort -u | grep -E "^0[2389]" | $(PERL) -p -e 's/^(\w{8}) (\w).{6} \S+\t(\w{8}) (\S+)$$/\1 \2 \3 \4/g' > $@
