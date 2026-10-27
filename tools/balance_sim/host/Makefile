# Host (x86-64) build of the real battle code: build/host/libminasim.so
# Invoked by `make balance-lib` in the repo root. Sources are run through the same cpp -> preproc
# pipeline as the ROM and compiled by the host gcc; shadow headers in hostinc/ and the generated
# files in build/host/gen/ replace the platform layer. No tracked game source is edited.
HOST  := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
ROOT  := $(abspath $(HOST)/../../..)
B     := $(ROOT)/build/host
OBJ   := $(B)/obj
GEN   := $(B)/gen
LIB   := $(B)/libminasim.so

# Real sources compiled unchanged (src/NAME.c)
SRCS := battle_gfx_sfx_util trig battle_main battle_script_commands battle_util battle_util2 pokemon random \
        battle_ai_script_commands battle_ai_switch_items battle_talk battle_controllers battle_message \
        strings data string_util util math_util event_data item special_move
# Host layer (this directory)
HOSTSRCS := hb_ptr hb_platform hb_api

CPP      ?= cpp
CPPFLAGS := -std=gnu11 -iquote $(HOST)/hostinc -iquote include -Wno-trigraphs -DRELEASE
CFLAGS   := -m64 -fPIC -O2 -std=gnu11 -ffunction-sections -fdata-sections -fno-strict-aliasing -fwrapv \
            -fcommon -fvisibility=hidden -Werror=implicit-function-declaration -Werror=int-conversion
ifneq ($(HB_DEBUG),)
CFLAGS += -g -O0
endif
HOSTCF   := $(CFLAGS) -iquote $(HOST)/hostinc -iquote include

# data.c: gTrainers must be writable so the harness can build a trainer for foe parties given explicitly
SEDX_data := -e 's/const struct Trainer gTrainers/struct Trainer gTrainers/'

ALLOBJ := $(SRCS:%=$(OBJ)/%.o) $(OBJ)/hb_ctrl_gen.o $(OBJ)/hb_blob.o $(HOSTSRCS:%=$(OBJ)/%.o)

all: $(LIB)
.PHONY: all clean

$(shell mkdir -p $(OBJ) $(GEN))

# cpp -> sed INCBIN (no graphics on the host) -> preproc (charmap strings) -> gcc
define PIPE
cd $(ROOT) && $(CPP) $(CPPFLAGS) -MMD -MF $(OBJ)/$(1).d -MT $(OBJ)/$(1).o $(2) 2>/dev/null \
  | sed -E -e 's/INCBIN_[SU](8|16|32)\([^)]*\)/{0}/g' $(SEDX_$(1)) > $(OBJ)/$(1).cpp.i
cd $(ROOT) && tools/preproc/preproc -i $(2) charmap.txt < $(OBJ)/$(1).cpp.i > $(OBJ)/$(1).i
cd $(ROOT) && gcc $(CFLAGS) -c -x c $(OBJ)/$(1).i -o $@ 2> $(OBJ)/$(1).log || { cat $(OBJ)/$(1).log; exit 1; }
endef

$(OBJ)/%.o: $(ROOT)/src/%.c
	@echo "host cc $*"
	@$(call PIPE,$*,src/$*.c)

$(OBJ)/hb_ctrl_gen.o: $(GEN)/hb_ctrl_gen.c
	@echo "host cc hb_ctrl_gen"
	@$(call PIPE,hb_ctrl_gen,$(GEN)/hb_ctrl_gen.c)

$(GEN)/hb_ctrl_gen.c: $(ROOT)/src/battle_controller_opponent.c $(HOST)/hb_ctrl_tail.c $(HOST)/gen.py
	@python3 $(HOST)/gen.py ctrl

$(GEN)/hb_blob.c: $(ROOT)/data/battle_scripts_1.s $(ROOT)/data/battle_scripts_2.s $(ROOT)/data/battle_ai_scripts.s \
                  $(wildcard $(ROOT)/include/constants/*.h $(ROOT)/asm/macros/*.inc $(ROOT)/asm/macros.inc) $(HOST)/gen.py
	@echo "host scripts blob"
	@python3 $(HOST)/gen.py blob

$(OBJ)/hb_blob.o: $(GEN)/hb_blob.c
	@cd $(ROOT) && gcc $(HOSTCF) -c $< -o $@

$(OBJ)/%.o: $(HOST)/%.c
	@echo "host cc $*"
	@cd $(ROOT) && gcc $(HOSTCF) -MMD -MF $(OBJ)/$*.d -c $< -o $@

$(LIB): $(ALLOBJ) $(HOST)/gen.py $(HOST)/exports.map
	@python3 $(HOST)/gen.py link

clean:
	rm -rf $(B)

-include $(wildcard $(OBJ)/*.d)
# run from the repo root: make -C <root> -f tools/balance_sim/host/Makefile
