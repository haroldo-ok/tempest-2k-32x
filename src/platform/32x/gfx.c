/*
 * WIREFIGHT 32X - framebuffer drawing primitives
 */
#include <stdint.h>
#ifdef WF_HOST_TRACE
#include <stdio.h>
#endif
#include "marsl.h"
#include "gfx.h"
#include "font.h"

#define VIEW_W  320
#define VIEW_H  224
#define VIEW_YOFF 0

#ifdef WF_HOST
/* host test build: framebuffer lives in host memory (see host_render.c) */
extern volatile uint16_t *host_fb_direct, *host_fb_overw;
#define FB_DIRECT host_fb_direct
#define FB_OVERW  host_fb_overw
#else
#define FB_DIRECT ((volatile uint16_t *)0x24000000)
#define FB_OVERW  ((volatile uint16_t *)0x24020000)
#endif

static uint32_t rng_state = 0x12345678;

uint32_t gfx_rand(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

void gfx_srand(uint32_t s)
{
    rng_state = s ? s : 1;
}

static inline volatile uint16_t *pixword(int x, int yabs, volatile uint16_t *base)
{
    /* line table formula: words[y] = y * 160 + 0x100 */
    return base + (uint32_t)yabs * (SCREEN_W / 2) + 0x100 + (x >> 1);
}

static inline void plot(int x, int y, uint8_t col)
{
    volatile uint16_t *w;

    if ((unsigned)x >= (unsigned)VIEW_W || (unsigned)y >= (unsigned)VIEW_H)
        return;
    w = pixword(x, y + VIEW_YOFF, FB_OVERW);
    if (x & 1)
        *w = col;
    else
        *w = (uint16_t)(col << 8);
}

void gfx_pset(int x, int y, uint8_t col)
{
    plot(x, y, col);
}

void gfx_pset_abs(int x, int y, uint8_t col)
{
    volatile uint16_t *w;
    if ((unsigned)x >= (unsigned)SCREEN_W || (unsigned)y >= (unsigned)FRAMEBUFFER_HEIGHT)
        return;
    w = pixword(x, y, FB_OVERW);
    if (x & 1)
        *w = col;
    else
        *w = (uint16_t)(col << 8);
}

/* Cohen-Sutherland clip codes for the view rectangle */
#define CS_L 1
#define CS_R 2
#define CS_T 4
#define CS_B 8

static int cs_code(int x, int y)
{
    int c = 0;
    if (x < 0) c = CS_L;
    else if (x >= VIEW_W) c = CS_R;
    if (y < 0) c |= CS_T;
    else if (y >= VIEW_H) c |= CS_B;
    return c;
}

/* clip a segment to the view rectangle; returns 0 if fully outside */
static int cs_clip(int *px1, int *py1, int *px2, int *py2)
{
    int x1 = *px1, y1 = *py1, x2 = *px2, y2 = *py2;
    int c1 = cs_code(x1, y1), c2 = cs_code(x2, y2);
    int guard = 0;

    for (;;) {
        if (++guard > 24)
            return 0;   /* numerical paranoia, never take more than that */
        if (!(c1 | c2)) {
            *px1 = x1; *py1 = y1; *px2 = x2; *py2 = y2;
            return 1;
        }
        if (c1 & c2)
            return 0;
        {
            int co = c1 ? c1 : c2;
            int x, y;
            /* guard the divisions: near-plane clipped segments can have
               horizontal/vertical extents of thousands of pixels */
            if (co & CS_B) {
                x = x1 + (x2 - x1) * (VIEW_H - 1 - y1) / (y2 - y1);
                y = VIEW_H - 1;
            } else if (co & CS_T) {
                x = x1 + (x2 - x1) * (-y1) / (y2 - y1);
                y = 0;
            } else if (co & CS_R) {
                y = y1 + (y2 - y1) * (VIEW_W - 1 - x1) / (x2 - x1);
                x = VIEW_W - 1;
            } else {
                y = y1 + (y2 - y1) * (-x1) / (x2 - x1);
                x = 0;
            }
            if (co == c1) { x1 = x; y1 = y; c1 = cs_code(x1, y1); }
            else { x2 = x; y2 = y; c2 = cs_code(x2, y2); }
        }
    }
}

void gfx_line(int x1, int y1, int x2, int y2, uint8_t col)
{
    int dx, sx, dy, sy, err, e2;

    /* clip first: near-plane intersections generate endpoints thousands
       of pixels off screen, and iterating them would eat whole frames */

    if (!cs_clip(&x1, &y1, &x2, &y2))
        return;

    /* canonical Bresenham, all cases (note dy is negative) */
    dx = x2 - x1; if (dx < 0) dx = -dx;
    sx = x1 < x2 ? 1 : -1;
    dy = y1 - y2; if (dy > 0) dy = -dy;
    sy = y1 < y2 ? 1 : -1;
    err = dx + dy;

    for (;;) {
        plot(x1, y1, col);
        if (x1 == x2 && y1 == y2)
            break;
        e2 = err * 2;
        if (e2 >= dy) {
            if (x1 == x2)
                break;
            err += dy;
            x1 += sx;
        }
        if (e2 <= dx) {
            if (y1 == y2)
                break;
            err += dx;
            y1 += sy;
        }
    }
}

void gfx_rect(int x, int y, int w, int h, uint8_t col)
{
    int yy, x0, x1, limx, limy;
    volatile uint16_t *wp;
    uint16_t both = (uint16_t)(col | (col << 8));

    if (w <= 0 || h <= 0)
        return;
    limx = x + w;
    limy = y + h;
    if (limx <= 0 || limy <= 0 || x >= VIEW_W || y >= VIEW_H)
        return;
    if (limx > VIEW_W) limx = VIEW_W;
    if (limy > VIEW_H) limy = VIEW_H;
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    for (yy = y; yy < limy; yy++) {
        wp = pixword(0, yy + VIEW_YOFF, FB_DIRECT);
        x0 = x;
        x1 = limx;
        while (x0 < x1) {
            if (x0 & 1) {              /* odd edge: only even x1 - 1 word touch */
                uint16_t v = (uint16_t)(wp[x0 >> 1] & 0xFF00);
                wp[x0 >> 1] = (uint16_t)(v | col);
                x0++;
            } else if (x1 == x0 + 1) { /* trailing even pixel */
                uint16_t v = (uint16_t)(wp[x0 >> 1] & 0x00FF);
                wp[x0 >> 1] = (uint16_t)(v | (col << 8));
                x0++;
            } else {
                wp[x0 >> 1] = both;
                x0 += 2;
            }
        }
    }
}

void gfx_rectb(int x, int y, int w, int h, uint8_t col)
{
    gfx_rect(x, y, w, 1, col);
    gfx_rect(x, y + h - 1, w, 1, col);
    gfx_rect(x, y + 1, 1, h - 2, col);
    gfx_rect(x + w - 1, y + 1, 1, h - 2, col);
}

static const uint8_t *glyph(char c)
{
    int i = (unsigned char)c - 32;
    if ((unsigned)i >= 95)
        i = 0;
    return wf_font7[i];
}

void gfx_text(int x, int y, const char *s, uint8_t col)
{
    while (*s) {
        const uint8_t *g = glyph(*s++);
        int r, c;
        for (r = 0; r < 7; r++) {
            uint8_t bits = g[r];
            if (!bits)
                continue;
            for (c = 0; c < 5; c++)
                if (bits & (0x10 >> c))
                    plot(x + c, y + r, col);
        }
        x += 6;
    }
}

void gfx_bigtext(int x, int y, const char *s, int scale, const uint8_t grad[7], uint8_t outline_col)
{
    int ci, r, c;
    const char *t;

    /* outline pass */
    for (ci = 0, t = s; *t; t++, ci++) {
        const uint8_t *g = glyph(*t);
        int gx = x + ci * 6 * scale;
        for (r = 0; r < 7; r++) {
            for (c = 0; c < 5; c++)
                if (g[r] & (0x10 >> c))
                    gfx_rect(gx + c * scale - 1, y + r * scale - 1,
                             scale + 2, scale + 2, outline_col);
        }
    }
    /* fill pass */
    for (ci = 0, t = s; *t; t++, ci++) {
        const uint8_t *g = glyph(*t);
        int gx = x + ci * 6 * scale;
        for (r = 0; r < 7; r++) {
            for (c = 0; c < 5; c++)
                if (g[r] & (0x10 >> c))
                    gfx_rect(gx + c * scale, y + r * scale,
                             scale, scale, grad[r]);
        }
    }
}

void gfx_tri(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t col)
{
    int tx, ty;
    if (y0 > y1) { tx=x0;x0=x1;x1=tx; ty=y0;y0=y1;y1=ty; }
    if (y0 > y2) { tx=x0;x0=x2;x2=tx; ty=y0;y0=y2;y2=ty; }
    if (y1 > y2) { tx=x1;x1=x2;x2=tx; ty=y1;y1=y2;y2=ty; }
    if (y2 == y0) return;

    for (int y = (y0 < 0 ? 0 : y0); y <= y2 && y < VIEW_H; y++) {
        int xa = x0 + (int)((long long)(x2 - x0) * (y - y0) / (y2 - y0));
        int xb;
        if (y < y1 && y1 != y0)
            xb = x0 + (int)((long long)(x1 - x0) * (y - y0) / (y1 - y0));
        else if (y2 != y1)
            xb = x1 + (int)((long long)(x2 - x1) * (y - y1) / (y2 - y1));
        else
            xb = x1;
        if (xa > xb) { int t=xa; xa=xb; xb=t; }
        if (xa < 0) xa = 0;
        if (xb >= VIEW_W) xb = VIEW_W - 1;
        if (xa <= xb) {
            gfx_rect(xa, y, xb - xa + 1, 1, col);
        }
    }
}

void gfx_circle(int cx, int cy, int r, uint8_t col)
{
    int x = r, y = 0, err = 0;
    while (x >= y) {
        gfx_pset(cx + x, cy + y, col);
        gfx_pset(cx + y, cy + x, col);
        gfx_pset(cx - y, cy + x, col);
        gfx_pset(cx - x, cy + y, col);
        gfx_pset(cx - x, cy - y, col);
        gfx_pset(cx - y, cy - x, col);
        gfx_pset(cx + y, cy - x, col);
        gfx_pset(cx + x, cy - y, col);
        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void gfx_circle_fill(int cx, int cy, int r, uint8_t col)
{
    for (int y = -r; y <= r; y++) {
        for (int x = -r; x <= r; x++) {
            if (x*x + y*y <= r*r) {
                gfx_pset(cx + x, cy + y, col);
            }
        }
    }
}
