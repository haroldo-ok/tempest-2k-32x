/*
 * TEMPEST 2000 32X - 2D Graphics library
 */
#ifndef T2K_GFX_H
#define T2K_GFX_H

#include <stdint.h>

void gfx_pset(int x, int y, uint8_t col);
void gfx_pset_abs(int x, int y, uint8_t col);
void gfx_line(int x1, int y1, int x2, int y2, uint8_t col);
void gfx_rect(int x, int y, int w, int h, uint8_t col);
void gfx_rectb(int x, int y, int w, int h, uint8_t col);
void gfx_text(int x, int y, const char *s, uint8_t col);
void gfx_bigtext(int x, int y, const char *s, int scale, const uint8_t grad[7], uint8_t outline_col);
void gfx_tri(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t col);
void gfx_circle(int cx, int cy, int r, uint8_t col);
void gfx_circle_fill(int cx, int cy, int r, uint8_t col);
uint32_t gfx_rand(void);
void gfx_srand(uint32_t s);

#endif /* T2K_GFX_H */
