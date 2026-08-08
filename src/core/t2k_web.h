/*
 * TEMPEST 2000 32X - 3D Geometric Webs / Tubes
 */
#ifndef T2K_WEB_H
#define T2K_WEB_H

#include <stdint.h>

#define MAX_LANES  18
#define WEB_DEPTH  500

typedef struct {
    const char *name;
    int num_lanes;
    int is_closed;
    int vx[MAX_LANES + 1];
    int vy[MAX_LANES + 1];
    uint8_t color_near;
    uint8_t color_far;
    uint8_t color_lane;
} t2k_web_t;

extern const t2k_web_t *g_cur_web;
extern int g_cur_level;

void t2k_web_init(void);
void t2k_web_set_level(int level); /* level: 1..16 */
void t2k_web_get_vertex(int idx, int depth, int *x, int *y, int *z);
void t2k_web_get_lane_center(int lane, int depth, int *x, int *y, int *z);
void t2k_web_get_lane_normal(int lane, int *nx, int *ny);
void t2k_web_render(int pulse, int active_lane);

#endif /* T2K_WEB_H */
