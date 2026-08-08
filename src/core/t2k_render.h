/*
 * TEMPEST 2000 32X - Software Renderer
 */
#ifndef T2K_RENDER_H
#define T2K_RENDER_H

#include <stdint.h>

enum {
    STATE_TITLE    = 0,
    STATE_MENU     = 1,
    STATE_READY    = 2,
    STATE_PLAY     = 3,
    STATE_WARP     = 4,
    STATE_GAMEOVER = 5,
};

void t2k_render_init(void);
void t2k_render_frame(int state, int state_timer, int menu_sel);

#endif /* T2K_RENDER_H */
