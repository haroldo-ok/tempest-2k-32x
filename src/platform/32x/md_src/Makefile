# WIREFIGHT 32X - 68000 side (pad publisher, runs in work RAM)
ROOTDIR ?= /opt/toolchains/sega

PREFIX  = $(ROOTDIR)/m68k-elf/bin/m68k-elf-
CC      = $(PREFIX)gcc
AS      = $(PREFIX)as
OBJC    = $(PREFIX)objcopy

CCFLAGS = -m68000 -Wall -Os -c -fomit-frame-pointer -fno-builtin -ffreestanding
ASFLAGS = -m68000 --register-prefix-optional
LDFLAGS = -T $(ROOTDIR)/ldscripts/mars-md.ld -nostdlib -Wl,-Map=md.map

all: m68k.bin

m68k.bin: m68k.elf
	$(OBJC) -O binary $< $@

m68k.elf: crt0.o main.o
	$(CC) $(LDFLAGS) crt0.o main.o -lgcc -o $@

crt0.o: crt0.s
	$(AS) $(ASFLAGS) $< -o $@

main.o: main.c
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -f *.o *.bin *.elf md.map

.PHONY: all clean
