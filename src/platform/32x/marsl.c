/*
 * WIREFIGHT 32X - minimal 32X support layer
 */
#include <stdint.h>
#include <stddef.h>
#include "marsl.h"

volatile unsigned mars_vblank_count = 0;
volatile unsigned mars_pwdt_ovf_count = 0;
volatile unsigned mars_swdt_ovf_count = 0;
volatile uint16_t mars_pad[2] = { 0, 0 };
uint16_t mars_refresh_hz = 60;

#define MARS_ACTIVE_SCREEN mars_activescreen
static volatile uint16_t mars_activescreen = 0;
static const uint8_t *mars_newpalette = NULL;

/* ------------------------------------------------------------------ */
/* interrupt handlers (called from crt0.s, live in SDRAM)              */
/* ------------------------------------------------------------------ */

static char Mars_UploadPalette(const uint8_t *palette) MARS_ATTR_DATA_CACHE_ALIGN;

void pri_vbi_handler(void)
{
    mars_vblank_count++;
    /* latch controller state published by the 68000
     * (COMM12/COMM14 - PicoDrive mirrors the bootrom checksum onto COMM8) */
    mars_pad[0] = (uint16_t)MARS_SYS_COMM12;
    mars_pad[1] = (uint16_t)MARS_SYS_COMM14;
    if (mars_newpalette != NULL) {
        if (Mars_UploadPalette(mars_newpalette))
            mars_newpalette = NULL;
    }
}

void pri_cmd_handler(void)
{
    /* no 68000 -> SH2 commands in use */
}

void sec_cmd_handler(void)
{
}

void sec_dma1_handler(void)
{
}

/* ------------------------------------------------------------------ */
/* init                                                                */
/* ------------------------------------------------------------------ */

void Mars_Init(void)
{
    /* turn DMA channels off, we drive the PWM FIFO by CPU */
    SH2_DMA_CHCR0 = 0;
    SH2_DMA_CHCR1 = 0;

    /* WDT timer off */
    SH2_WDT_WTCSR_TCNT = 0xA500;
}

void Mars_WaitFrameBuffersFlip(void)
{
    while ((MARS_VDP_FBCTL & MARS_VDP_FS) != MARS_ACTIVE_SCREEN);
}

void Mars_FlipFrameBuffers(char wait)
{
    MARS_ACTIVE_SCREEN = !(MARS_ACTIVE_SCREEN & MARS_VDP_FS);
    MARS_VDP_FBCTL = MARS_ACTIVE_SCREEN;
    if (wait)
        Mars_WaitFrameBuffersFlip();
}

void Mars_InitLineTable(void)
{
    int j;
    uint16_t *lines = (uint16_t *)&MARS_FRAMEBUFFER;

    for (j = 0; j < FRAMEBUFFER_HEIGHT; j++)
        lines[j] = j * (SCREEN_W / 2) + 0x100;

    /* set the rest of the line table to a blank line */
    for (; j < 256; j++)
        lines[j] = 224 * (SCREEN_W / 2) + 0x100;

    /* clear the blank line */
    for (j = 224 * (SCREEN_W / 2); j < 224 * (SCREEN_W / 2) + SCREEN_W / 2; j++)
        lines[j] = 0;
}

void Mars_InitVideo(void)
{
    int i;
    uint16_t NTSC;

    while ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0);

    MARS_VDP_DISPMODE = MARS_224_LINES | MARS_VDP_MODE_256;
    NTSC = (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT) != 0;
    mars_refresh_hz = NTSC ? 60 : 50;

    MARS_ACTIVE_SCREEN = MARS_VDP_FBCTL;
    Mars_FlipFrameBuffers(1);

    for (i = 0; i < 2; i++) {
        uint16_t *p, *p_end;

        Mars_InitLineTable();

        p = (uint16_t *)(&MARS_FRAMEBUFFER + 0x100);
        p_end = p + (SCREEN_W / 2) * FRAMEBUFFER_HEIGHT;
        do {
            *p = 0;
        } while (++p < p_end);

        Mars_FlipFrameBuffers(1);
    }
}

char Mars_FramebuffersFlipped(void)
{
    return (MARS_VDP_FBCTL & MARS_VDP_FS) == MARS_ACTIVE_SCREEN;
}

int Mars_BackBuffer(void)
{
    return MARS_ACTIVE_SCREEN;
}

void Mars_SetPalette(const uint8_t *palette)
{
    mars_newpalette = palette;
}

static char Mars_UploadPalette(const uint8_t *palette)
{
    int i;
    uint16_t *cram = (uint16_t *)&MARS_CRAM;

    if ((MARS_SYS_INTMSK & MARS_SH2_ACCESS_VDP) == 0)
        return 0;

    for (i = 0; i < 256; i++) {
        unsigned r = *palette++;
        unsigned g = *palette++;
        unsigned b = *palette++;

        cram[i] = (uint16_t)(0x8000 | ((r >> 3) & 0x1f)
                             | (((g >> 3) & 0x1f) << 5)
                             | (((b >> 3) & 0x1f) << 10));
    }
    return 1;
}

uint16_t *Mars_FrameBufferLines(void)
{
    return (uint16_t *)&MARS_FRAMEBUFFER;
}

/* ------------------------------------------------------------------ */
/* VDP fill engine                                                     */
/* ------------------------------------------------------------------ */

char Mars_FillBusy(void)
{
    return (MARS_VDP_FBCTL & MARS_VDP_FEN) != 0;
}

void Mars_WaitFill(void)
{
    while (MARS_VDP_FBCTL & MARS_VDP_FEN);
}

/* the fill engine wraps inside a 256-word page, so we can only fill
 * within one line at a time */
void Mars_FillLine(int y, int x0, int x1, uint8_t color)
{
    uint16_t fill, len;
    const uint16_t *lines = Mars_FrameBufferLines();

    if (x1 <= x0)
        return;
    fill = (uint16_t)(color | (color << 8));
    len = (uint16_t)((x1 - x0) >> 1);
    if (len == 0)
        return;

    Mars_WaitFill();
    MARS_VDP_FILADR = (uint16_t)(lines[y & 0xFF] + (x0 >> 1));
    MARS_VDP_FILLEN = len;
    MARS_VDP_FILDAT = fill;
}

void Mars_ClearScreen(uint8_t color)
{
    int y;
    uint16_t fill = (uint16_t)(color | (color << 8));
    const uint16_t *lines = Mars_FrameBufferLines();

    for (y = 0; y < FRAMEBUFFER_HEIGHT; y++) {
        Mars_WaitFill();
        MARS_VDP_FILADR = lines[y];
        MARS_VDP_FILLEN = SCREEN_W / 2;
        MARS_VDP_FILDAT = fill;
    }
    Mars_WaitFill();
}

void Mars_WaitTicks(int ticks)
{
    unsigned start = mars_vblank_count;
    while ((int)(mars_vblank_count - start) < ticks);
}
