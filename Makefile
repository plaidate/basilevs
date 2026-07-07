# Basilevs on the Playdate, built against the raylib-pd fork.
#
#   make            build basilevs.pdx (universal: simulator + device)
#   make run        build + open in the Playdate Simulator
#   make clean
#
# Asset staging follows the raylib-pd convention: resources-src/ is copied
# into the Source dir BEFORE pdc runs (bullet.wav -> bullet.pda), while
# resources/ (raw PNGs + music.mp3) is merged into the pdx AFTER pdc, since
# pdc would otherwise compile the PNGs into .pdi files.

SDK ?= $(HOME)/Developer/PlaydateSDK
PDC ?= pdc
SIMULATOR ?= $(SDK)/bin/Playdate Simulator.app

RAYLIB ?= ../raylib-pd
# Absolute: rlsw.h's `#include __FILE__` template trick embeds this path in
# the include string, and only an absolute path re-resolves from anywhere
RAYLIB := $(abspath $(RAYLIB))
RAYLIB_SRC := $(RAYLIB)/src
BUILD := build
GAME := basilevs

SRCS := src/main.c src/entities.c src/world.c
RL_MODULES := rcore rshapes rtextures rtext rmodels pd_raudio

CC := clang
ARM_CC ?= arm-none-eabi-gcc

# ---- simulator (host dylib) ----

DEFS := -DPLATFORM_PLAYDATE -DGRAPHICS_API_OPENGL_SOFTWARE \
        -DTARGET_SIMULATOR=1 -DTARGET_EXTENSION=1 \
        -DPD_FRAMEDUMP

CFLAGS := -std=gnu11 -g -O2 -fPIC \
    -I$(RAYLIB_SRC) -I$(RAYLIB)/playdate -I$(SDK)/C_API -Isrc \
    -Wall -Wno-unused-parameter \
    -fno-strict-aliasing \
    $(DEFS)

RL_OBJS := $(addprefix $(BUILD)/obj/,$(addsuffix .o,$(RL_MODULES)))

$(BUILD)/obj/%.o: $(RAYLIB_SRC)/%.c
	@mkdir -p $(BUILD)/obj
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/obj/rcore.o: $(RAYLIB_SRC)/platforms/rcore_playdate.c $(RAYLIB_SRC)/external/rlsw.h $(RAYLIB_SRC)/rlgl.h

$(BUILD)/Source/pdex.dylib: $(SRCS) src/game.h $(RL_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -dynamiclib -rdynamic $(CFLAGS) -Dmain=rl_playdate_main $(SRCS) $(RL_OBJS) -o $@

# ---- device (ARM Cortex-M7) ----

MCFLAGS := -mthumb -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-sp-d16 -D__FPU_USED=1

DEV_DEFS := -DPLATFORM_PLAYDATE -DGRAPHICS_API_OPENGL_SOFTWARE \
            -DTARGET_PLAYDATE=1 -DTARGET_EXTENSION=1 \
            -DPD_FRAMEDUMP

DEV_CFLAGS := -std=gnu11 $(MCFLAGS) -O2 -falign-functions=16 -fomit-frame-pointer \
    -gdwarf-2 -mword-relocations -fno-common -ffunction-sections -fdata-sections \
    -fno-math-errno -fsingle-precision-constant -ffp-contract=fast \
    -I$(RAYLIB_SRC) -I$(RAYLIB)/playdate -I$(SDK)/C_API -Isrc \
    -Wall -Wno-unused-parameter -Wno-unknown-pragmas \
    -fno-strict-aliasing \
    $(DEV_DEFS)

DEV_OBJS := $(addprefix $(BUILD)/obj-arm/,$(addsuffix .o,$(RL_MODULES))) $(BUILD)/obj-arm/setup.o

$(BUILD)/obj-arm/%.o: $(RAYLIB_SRC)/%.c
	@mkdir -p $(BUILD)/obj-arm
	$(ARM_CC) -c $(DEV_CFLAGS) $< -o $@

$(BUILD)/obj-arm/rcore.o: $(RAYLIB_SRC)/platforms/rcore_playdate.c $(RAYLIB_SRC)/external/rlsw.h $(RAYLIB_SRC)/rlgl.h

# The rasterizer-heavy core object gets -O3 (as in raylib-pd's own Makefile)
$(BUILD)/obj-arm/rcore.o: DEV_CFLAGS += -O3

$(BUILD)/obj-arm/setup.o: $(SDK)/C_API/buildsupport/setup.c
	@mkdir -p $(BUILD)/obj-arm
	$(ARM_CC) -c $(DEV_CFLAGS) $< -o $@

$(BUILD)/Source/pdex.elf: $(SRCS) src/game.h $(DEV_OBJS)
	@mkdir -p $(dir $@)
	$(ARM_CC) $(DEV_CFLAGS) -Dmain=rl_playdate_main $(SRCS) $(DEV_OBJS) \
	    -nostartfiles -T$(SDK)/C_API/buildsupport/link_map.ld \
	    -Wl,-Map=$(BUILD)/pdex.map,--cref,--gc-sections,--no-warn-mismatch,--emit-relocs \
	    -lm -o $@

# ---- bundle ----

$(BUILD)/Source/pdxinfo:
	@mkdir -p $(dir $@)
	@printf 'name=Basilevs\nauthor=bredlej (Playdate port: sdwfrost)\ndescription=1-bit bullet hell\nbundleID=com.sdwfrost.basilevs\nversion=0.1\nbuildNumber=1\n' > $@

$(GAME).pdx: $(BUILD)/Source/pdex.dylib $(BUILD)/Source/pdex.elf $(BUILD)/Source/pdxinfo
	@mkdir -p $(BUILD)/Source/resources
	cp -R resources-src/. $(BUILD)/Source/resources/
	$(PDC) $(BUILD)/Source $@
	@mkdir -p $@/resources
	cp -R resources/. $@/resources/

all: $(GAME).pdx

run: $(GAME).pdx
	open -a "$(SIMULATOR)" $(GAME).pdx

clean:
	rm -rf $(BUILD) $(GAME).pdx

.PHONY: all run clean
.DEFAULT_GOAL := $(GAME).pdx
