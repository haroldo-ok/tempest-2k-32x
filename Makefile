# ======================================================================
# Tempest 2000 Sega 32X Port Makefile
# Chilly Willy 32XDK 20220418, GCC 12.1
# ======================================================================

GENDEV ?= /opt/toolchains/sega
TARGET  := tempest2k
TITLE   := TEMPEST 2000
BUILD   := build
ROM     := rom/$(TARGET).32x

# ---- toolchains ------------------------------------------------------
SHPREFIX := $(GENDEV)/sh-elf/bin/sh-elf-
MDPREFIX := $(GENDEV)/m68k-elf/bin/m68k-elf-
CC   := $(SHPREFIX)gcc
AS   := $(SHPREFIX)as
OBJC := $(SHPREFIX)objcopy
SIZE := $(SHPREFIX)size

GCCVER := 12.1.0

# ---- SH-2 flags ------------------------------------------------------
INCPATH := -Isrc/core -Isrc/platform/32x \
           -I$(GENDEV)/sh-elf/include -I$(GENDEV)/sh-elf/sh-elf/include
SHFLAGS := -c -std=c11 -m2 -mb -mtas -Os -fomit-frame-pointer \
           -ffunction-sections -fdata-sections -fno-common -flto \
           -Wall -Wextra -Wno-unused-parameter -D__32X__ -DMARS $(INCPATH)

LDFLAGS := -T src/platform/32x/mars.ld -nostdlib --specs=nosys.specs \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map -Os -flto
LIBS    := -L$(GENDEV)/sh-elf/sh-elf/lib \
           -L$(GENDEV)/sh-elf/lib/gcc/sh-elf/$(GCCVER) -lc -lgcc -lnosys

# ---- objects ---------------------------------------------------------
CORE_OBJS := $(BUILD)/t2k_math.o \
             $(BUILD)/t2k_web.o \
             $(BUILD)/t2k_player.o \
             $(BUILD)/t2k_enemies.o \
             $(BUILD)/t2k_particles.o \
             $(BUILD)/t2k_game.o \
             $(BUILD)/t2k_render.o

PLAT_OBJS := $(BUILD)/marsl.o \
             $(BUILD)/gfx.o \
             $(BUILD)/font.o \
             $(BUILD)/sound.o \
             $(BUILD)/sintab.o \
             $(BUILD)/main_32x.o

SH_OBJS   := $(BUILD)/crt0.o $(CORE_OBJS) $(PLAT_OBJS)

MARKERS := --min-text 16384 \
           --title "$(TITLE)" \
           --marker "TEMPEST 2000" \
           --marker "PRESS START" \
           --marker "LEVEL COMPLETE" \
           --marker "SUPERZAPPER"

.PHONY: all check clean md_side

all: $(ROM)

$(BUILD) rom:
	mkdir -p $@

md_side:
	$(MAKE) -C src/platform/32x/md_src ROOTDIR=$(GENDEV)

# ---- SH-2 startup embeds the 68000 binary ----
$(BUILD)/crt0.o: src/platform/32x/crt0.s | md_side $(BUILD)
	$(AS) --big $< -o $@

# ---- core + platform C ----
$(BUILD)/%.o: src/core/%.c | $(BUILD)
	$(CC) $(SHFLAGS) $< -o $@
$(BUILD)/%.o: src/platform/32x/%.c | $(BUILD)
	$(CC) $(SHFLAGS) $< -o $@

# ---- link, objcopy, pad, header-fix ----
$(BUILD)/$(TARGET).elf: $(SH_OBJS)
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@
	$(SIZE) $@
	@grep -E 'bss_end|__stack' $(BUILD)/$(TARGET).map || true

$(ROM): $(BUILD)/$(TARGET).elf | rom
	$(OBJC) -O binary $< $(BUILD)/$(TARGET).raw
	dd if=$(BUILD)/$(TARGET).raw of=$@ bs=8192 conv=sync status=none
	python3 tools/romfix.py $@ --title "$(TITLE)"
	mkdir -p /home/user/rom
	cp $@ /home/user/rom/tempest2k.32x
	cp $@ /home/user/tempest2k.32x
	@echo "=============================================="
	@echo " ROM built and copied to reachable user paths:"
	@echo "   $@"
	@echo "   /home/user/rom/tempest2k.32x"
	@echo "   /home/user/tempest2k.32x"
	@echo "=============================================="

check: $(ROM)
	python3 tests/verify_rom.py $(ROM) $(BUILD)/$(TARGET).elf $(MARKERS)
	python3 tests/run_tests.py

clean:
	rm -rf $(BUILD) rom
	$(MAKE) -C src/platform/32x/md_src clean
