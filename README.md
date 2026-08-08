# Tempest 2000 — Sega 32X Port (`tempest2k-32x`)

A native Sega 32X (Sega Mars) software-3D port of Jeff Minter's 1994 classic **Tempest 2000**, written in portable C11 with a thin Sega 32X hardware shell and dual-SH2 / 68000 support.

## Features

- **16 Authentic 3D Webs:** Cylinder, Square, Triangle, Flat Ribbon, Jeff Minter's *"Distorted W"*, Plus Tube, V-Valley, Star Tube, Staircase, Infinity Loop, Hexagon, Diamond, Zigzag, Heart, Bowtie, and Warp Tunnel.
- **Translucent Shaded Lane Polygons & 3D Perspective:** Shaded lane quads with vector outline overlay, depth rings, active lane highlight, and 96-star 3D starfield.
- **Full Gameplay & Enemies:** Yellow & Cyan Crawler Claw, Flippers, Tankers (split into 2 flippers on destruction), Spikers & Spikes, Fuseballs, Pulsars, floating Bonus Capsules, Jump ability, A.I. Droid companion, and Superzapper screen flash.
- **Dual-SH2 PWM Audio Synthesizer:** Master SH-2 handles game logic & 3D rendering; Slave SH-2 runs a real-time PWM audio synthesizer generating Laser chirps, Flipper clicks, Explosions, Superzapper frequency sweeps, and a 140 BPM techno/rave background loop.
- **68000 Resident Controller Masking:** 3-button joypad masking to guard against 6-button extended-nibble mirroring bugs.

## Building the ROM

Requires Chilly Willy's **32XDK** (`sh-elf-gcc` and `m68k-elf-gcc` 12.1) installed at `/opt/toolchains/sega` (override with `GENDEV=<path>`):

```sh
make
```

This compiles:
1. `src/platform/32x/md_src/` (Genesis 68000 resident helper binary `m68k.bin`)
2. `src/core/` (portable C11 game engine)
3. `src/platform/32x/` (SH-2 master/slave initialization, VDP, line drawing, PWM synthesizer)
4. Links and pads to `rom/tempest2k.32x` (512 KiB cartridge) and runs `romfix.py` to write the Genesis header and checksum.

## Running Automated Tests

Headless PicoDrive (libretro) automated point-to-point test scripts are included in `tests/scripts/`:

```sh
make check
```

This runs static structural verification (`verify_rom.py`) and boots the ROM in headless PicoDrive to assert lit, colourful, and dynamic frames across Boot (`01_boot.txt`), Menu (`02_menu.txt`), Gameplay (`03_gameplay.txt`), and Action/Superzapper (`04_action.txt`).

## Project Layout

```
tempest2k-32x/
├── Makefile                # SH-2 + 68000 build pipeline
├── README.md               # This document
├── src/
│   ├── core/               # Portable C11 Tempest 2000 engine (no OS calls or floats)
│   │   ├── t2k_math.c/.h   # Fixed-point math & 1/z perspective projection table
│   │   ├── t2k_web.c/.h    # 16 3D geometric webs & lane rendering
│   │   ├── t2k_player.c/.h # Crawler Claw, Jump, and AI Droid
│   │   ├── t2k_enemies.c/.h# Flippers, Tankers, Spikers, Spikes, Fuseballs, Pulsars, Bullets
│   │   ├── t2k_particles.c/.h # Starfield, explosions, powerup capsules
│   │   ├── t2k_render.c/.h # 256-color palette, HUD, state rendering
│   │   └── t2k_game.c/.h   # State machine (Title, Menu, Ready, Play, Warp, GameOver)
│   └── platform/
│       └── 32x/            # Sega 32X hardware shell
│           ├── crt0.s      # Dual SH-2 startup & vectors
│           ├── mars.ld     # SH-2 linker script (SDRAM memory map)
│           ├── marsl.c/.h  # VDP 256-color mode, double buffering, CRAM palette upload
│           ├── gfx.c/.h    # Bresenham line drawing, triangle fill, fonts
│           ├── sound.c/.h  # PWM audio synthesizer (Slave SH-2)
│           └── md_src/     # Genesis 68000 resident code & Makefile
├── tools/
│   └── romfix.py           # Genesis header & checksum fixup tool
├── tests/
│   ├── verify_rom.py       # Static ROM structural checker
│   ├── run_tests.py        # PicoDrive libretro runner & black-screen guard
│   └── scripts/            # Point-to-point input scripts
└── rom/
    └── tempest2k.32x       # Compiled playable 32X ROM
```
