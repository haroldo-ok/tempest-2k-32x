/*
 * TEMPEST 2000 32X - 3D Geometric Webs / Tubes implementation
 */
#include "t2k_web.h"
#include "t2k_math.h"
#include "gfx.h"

const t2k_web_t *g_cur_web = 0;
int g_cur_level = 1;

static const t2k_web_t s_webs[16] = {
    /* 1: CYLINDER */
    {
        "CYLINDER", 16, 1,
        { 100, 92, 70, 38, 0, -38, -70, -92, -100, -92, -70, -38, 0, 38, 70, 92, 100 },
        { 0, 30, 56, 73, 80, 73, 56, 30, 0, -30, -56, -73, -80, -73, -56, -30, 0 },
        3, 1, 14
    },
    /* 2: SQUARE */
    {
        "SQUARE", 16, 1,
        { -90, -45, 0, 45, 90, 90, 90, 90, 90, 45, 0, -45, -90, -90, -90, -90, -90 },
        { -70, -70, -70, -70, -70, -35, 0, 35, 70, 70, 70, 70, 70, 35, 0, -35, -70 },
        5, 4, 15
    },
    /* 3: TRIANGLE */
    {
        "TRIANGLE", 15, 1,
        { 0, 20, 40, 60, 80, 100, 60, 20, -20, -60, -100, -80, -60, -40, -20, 0 },
        { -80, -50, -20, 10, 40, 70, 70, 70, 70, 70, 70, 40, 10, -20, -50, -80 },
        7, 8, 3
    },
    /* 4: FLAT RIBBON */
    {
        "FLAT RIBBON", 16, 0,
        { -120, -105, -90, -75, -60, -45, -30, -15, 0, 15, 30, 45, 60, 75, 90, 105, 120 },
        { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 },
        6, 9, 7
    },
    /* 5: DISTORTED W (Jeff Minter ASCII art web) */
    {
        "DISTORTED W", 16, 0,
        { -120, -105, -90, -75, -60, -45, -30, -15, 0, 15, 30, 45, 60, 75, 90, 105, 120 },
        { -60, -30, 0, 30, 60, 30, 0, -30, 0, -30, 0, 30, 60, 30, 0, -30, -60 },
        10, 9, 6
    },
    /* 6: PLUS TUBE */
    {
        "PLUS TUBE", 16, 1,
        { -30, 30, 30, 90, 90, 90, 30, 30, -30, -30, -90, -90, -90, -30, -30, -30, -30 },
        { -90, -90, -30, -30, 0, 30, 30, 90, 90, 30, 30, 0, -30, -30, -90, -90, -90 },
        14, 1, 3
    },
    /* 7: V-VALLEY */
    {
        "V-VALLEY", 16, 0,
        { -120, -105, -90, -75, -60, -45, -30, -15, 0, 15, 30, 45, 60, 75, 90, 105, 120 },
        { -60, -45, -30, -15, 0, 15, 30, 45, 60, 45, 30, 15, 0, -15, -30, -45, -60 },
        3, 2, 14
    },
    /* 8: STAR TUBE */
    {
        "STAR TUBE", 15, 1,
        { 0, 25, 95, 40, 60, 0, -60, -40, -95, -25, 0, 25, 95, 40, 60, 0 },
        { -90, -30, -30, 10, 70, 30, 70, 10, -30, -30, -90, -30, -30, 10, 70, -90 },
        5, 4, 10
    },
    /* 9: STAIRCASE */
    {
        "STAIRCASE", 16, 0,
        { -120, -105, -105, -90, -90, -75, -75, -60, -60, -45, -45, -30, -30, -15, -15, 0, 0 },
        { -60, -60, -45, -45, -30, -30, -15, -15, 0, 0, 15, 15, 30, 30, 45, 45, 60 },
        6, 9, 10
    },
    /* 10: INFINITY LOOP */
    {
        "INFINITY LOOP", 16, 1,
        { -80, -50, -20, 0, 20, 50, 80, 50, 20, 0, -20, -50, -80, -50, -20, 0, -80 },
        { 0, -40, -40, 0, 40, 40, 0, -40, -40, 0, 40, 40, 0, -40, -40, 0, 0 },
        14, 5, 3
    },
    /* 11: HEXAGON */
    {
        "HEXAGON", 16, 1,
        { 90, 45, 0, -45, -90, -90, -90, -45, 0, 45, 90, 90, 90, 45, 0, -45, 90 },
        { 0, 60, 70, 60, 0, -30, -60, -70, -60, -30, 0, 30, 60, 30, 0, -30, 0 },
        7, 8, 6
    },
    /* 12: DIAMOND */
    {
        "DIAMOND", 16, 1,
        { 0, 25, 50, 75, 100, 75, 50, 25, 0, -25, -50, -75, -100, -75, -50, -25, 0 },
        { -80, -60, -40, -20, 0, 20, 40, 60, 80, 60, 40, 20, 0, -20, -40, -60, -80 },
        3, 14, 11
    },
    /* 13: ZIGZAG */
    {
        "ZIGZAG", 16, 0,
        { -120, -105, -90, -75, -60, -45, -30, -15, 0, 15, 30, 45, 60, 75, 90, 105, 120 },
        { -40, 40, -40, 40, -40, 40, -40, 40, -40, 40, -40, 40, -40, 40, -40, 40, -40 },
        10, 6, 9
    },
    /* 14: HEART */
    {
        "HEART", 16, 1,
        { 0, 30, 70, 80, 60, 30, 0, -30, -60, -80, -70, -30, 0, 30, 60, 30, 0 },
        { -20, -60, -50, -10, 30, 60, 80, 60, 30, -10, -50, -60, -20, -60, -50, -10, -20 },
        5, 15, 11
    },
    /* 15: BOWTIE */
    {
        "BOWTIE", 16, 1,
        { -90, -60, -30, 0, 30, 60, 90, 60, 30, 0, -30, -60, -90, -60, -30, 0, -90 },
        { -60, -40, -20, 0, 20, 40, 60, -40, -20, 0, 20, 40, -60, 20, 40, 0, -60 },
        14, 3, 5
    },
    /* 16: WARP TUNNEL */
    {
        "WARP TUNNEL", 16, 1,
        { 100, 92, 70, 38, 0, -38, -70, -92, -100, -92, -70, -38, 0, 38, 70, 92, 100 },
        { 0, 30, 56, 73, 80, 73, 56, 30, 0, -30, -56, -73, -80, -73, -56, -30, 0 },
        3, 5, 11
    }
};

void t2k_web_init(void)
{
    g_cur_level = 1;
    g_cur_web = &s_webs[0];
}

void t2k_web_set_level(int level)
{
    int idx;
    if (level < 1) level = 1;
    if (level > 16) level = 16;
    g_cur_level = level;
    idx = (level - 1) & 15;
    g_cur_web = &s_webs[idx];
}

void t2k_web_get_vertex(int idx, int depth, int *x, int *y, int *z)
{
    if (idx < 0) idx = 0;
    if (idx > g_cur_web->num_lanes) idx = g_cur_web->num_lanes;
    *x = g_cur_web->vx[idx];
    *y = g_cur_web->vy[idx];
    *z = depth;
}

void t2k_web_get_lane_center(int lane, int depth, int *x, int *y, int *z)
{
    int i0 = lane;
    int i1 = (lane + 1) % (g_cur_web->num_lanes + (g_cur_web->is_closed ? 0 : 1));
    if (!g_cur_web->is_closed && i1 > g_cur_web->num_lanes) i1 = g_cur_web->num_lanes;
    *x = (g_cur_web->vx[i0] + g_cur_web->vx[i1]) / 2;
    *y = (g_cur_web->vy[i0] + g_cur_web->vy[i1]) / 2;
    *z = depth;
}

void t2k_web_get_lane_normal(int lane, int *nx, int *ny)
{
    int i0 = lane;
    int i1 = (lane + 1);
    int dx, dy, len;
    if (i1 > g_cur_web->num_lanes) i1 = 0;
    dx = g_cur_web->vx[i1] - g_cur_web->vx[i0];
    dy = g_cur_web->vy[i1] - g_cur_web->vy[i0];
    /* Outward normal is (-dy, dx) */
    *nx = -dy;
    *ny = dx;
    len = 1;
    if (*nx || *ny) {
        int ax = (*nx < 0) ? -*nx : *nx;
        int ay = (*ny < 0) ? -*ny : *ny;
        len = ax + ay;
        if (len == 0) len = 1;
    }
    *nx = (*nx * 100) / len;
    *ny = (*ny * 100) / len;
}

void t2k_web_render(int pulse, int active_lane)
{
    int i, n;
    int sx0_near, sy0_near, sx1_near, sy1_near;
    int sx0_far,  sy0_far,  sx1_far,  sy1_far;
    int sx0_mid,  sy0_mid,  sx1_mid,  sy1_mid;
    uint8_t col_near = g_cur_web->color_near;
    uint8_t col_far  = g_cur_web->color_far;

    if (pulse & 8) {
        col_near = g_cur_web->color_lane;
    }

    n = g_cur_web->num_lanes;

    for (i = 0; i <= n; i++) {
        if (!g_cur_web->is_closed && i == n)
            break;

        int i_next = (i + 1);
        if (g_cur_web->is_closed && i_next == n) {
            i_next = 0;
        } else if (i_next > n) {
            break;
        }

        int vx0 = g_cur_web->vx[i], vy0 = g_cur_web->vy[i];
        int vx1 = g_cur_web->vx[i_next], vy1 = g_cur_web->vy[i_next];
        if (active_lane == -1) {
            int angle = (pulse * 2) & 255;
            int c = t2k_cos(angle), s = t2k_sin(angle);
            int tx = (int)(((int64_t)vx0 * c - (int64_t)vy0 * s) >> 16);
            int ty = (int)(((int64_t)vx0 * s + (int64_t)vy0 * c) >> 16);
            vx0 = tx; vy0 = ty;
            tx = (int)(((int64_t)vx1 * c - (int64_t)vy1 * s) >> 16);
            ty = (int)(((int64_t)vx1 * s + (int64_t)vy1 * c) >> 16);
            vx1 = tx; vy1 = ty;
        }

        /* Project inner (far, z=500), middle (z=250), and outer (near, z=0) */
        t2k_project(vx0, vy0, 500, &sx0_far,  &sy0_far);
        t2k_project(vx1, vy1, 500, &sx1_far,  &sy1_far);

        t2k_project(vx0, vy0, 250, &sx0_mid,  &sy0_mid);
        t2k_project(vx1, vy1, 250, &sx1_mid,  &sy1_mid);

        t2k_project(vx0, vy0, 0,   &sx0_near, &sy0_near);
        t2k_project(vx1, vy1, 0,   &sx1_near, &sy1_near);

        /* Fill translucent/shaded web lane polygon */
        uint8_t fill_col = (i & 1) ? 1 : 4; /* Alternating dark blue & dark purple */
        if (i == active_lane) fill_col = 2; /* Highlight player lane with medium blue */
        gfx_tri(sx0_far, sy0_far, sx1_far, sy1_far, sx0_near, sy0_near, fill_col);
        gfx_tri(sx1_far, sy1_far, sx1_near, sy1_near, sx0_near, sy0_near, fill_col);

        /* Draw radial lane divider line from far to near */
        uint8_t line_col = col_far;
        if (i == active_lane || (i == active_lane + 1)) {
            line_col = g_cur_web->color_lane;
        }
        gfx_line(sx0_far, sy0_far, sx0_near, sy0_near, line_col);

        /* Draw far ring segment (deep end of tube) */
        gfx_line(sx0_far, sy0_far, sx1_far, sy1_far, col_far);

        /* Draw middle ring segment */
        gfx_line(sx0_mid, sy0_mid, sx1_mid, sy1_mid, col_far);

        /* Draw near rim segment */
        uint8_t rim_col = (i == active_lane) ? g_cur_web->color_lane : col_near;
        gfx_line(sx0_near, sy0_near, sx1_near, sy1_near, rim_col);
    }
}
