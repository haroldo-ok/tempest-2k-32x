/*
 * WIREFIGHT 32X - minimal 32X support layer (master/slave SH2 + VDP + PWM)
 *
 * Loosely based on Chilly Willy's 32X SDK and Victor Luchits' marshw.c
 * from d32xr (MIT licensed).
 */
#ifndef WF_MARSL_H
#define WF_MARSL_H

#include <stdint.h>
#include "32x.h"

#define MARS_ATTR_DATA_CACHE_ALIGN __attribute__((section(".sdata"), aligned(16), optimize("O1")))

#define FRAMEBUFFER_HEIGHT 224
#define SCREEN_W           320
#define SCREEN_H           FRAMEBUFFER_HEIGHT

extern volatile unsigned mars_vblank_count;
extern volatile unsigned mars_pwdt_ovf_count;
extern volatile unsigned mars_swdt_ovf_count;
extern uint16_t mars_refresh_hz;

void Mars_Init(void);
void Mars_InitVideo(void);

void Mars_FlipFrameBuffers(char wait);
void Mars_WaitFrameBuffersFlip(void);
char Mars_FramebuffersFlipped(void);
int  Mars_BackBuffer(void);

void Mars_SetPalette(const uint8_t *palette);  /* 256 * 3 bytes, queued for next vblank */
uint16_t *Mars_FrameBufferLines(void);

/* VDP fill engine helpers (target the back buffer) */
void Mars_InitLineTable(void);
void Mars_FillLine(int y, int x0, int x1, uint8_t color);
void Mars_ClearScreen(uint8_t color);
void Mars_WaitFill(void);
char Mars_FillBusy(void);

/* joypad values latched every vblank (SEGA_CTRL_* bits from 32x.h) */
extern volatile uint16_t mars_pad[2];
static inline uint16_t Mars_ReadController(int i) { return mars_pad[i & 1]; }

void Mars_WaitTicks(int ticks);

#define Mars_ClearCacheLine(addr) *(volatile uintptr_t *)(((uintptr_t)addr) | 0x40000000) = 0
#define Mars_ClearCache() \
	do { \
		CacheControl(0); \
		CacheControl(SH2_CCTL_CP | SH2_CCTL_CE); \
	} while (0)

/* interrupt handlers wired from crt0.s */
void pri_vbi_handler(void) MARS_ATTR_DATA_CACHE_ALIGN;
void pri_cmd_handler(void) MARS_ATTR_DATA_CACHE_ALIGN;
void sec_cmd_handler(void) MARS_ATTR_DATA_CACHE_ALIGN;
void sec_dma1_handler(void) MARS_ATTR_DATA_CACHE_ALIGN;

void CacheControl(int mode);

#endif /* WF_MARSL_H */
