/*
 * TEMPEST 2000 32X - Top-level Game State Machine
 */
#ifndef T2K_GAME_H
#define T2K_GAME_H

#include <stdint.h>

extern int g_state;
extern int g_state_timer;
extern int g_menu_sel;

void t2k_game_init(void);
void t2k_game_tick(uint16_t pad);
void t2k_game_render(void);

#endif /* T2K_GAME_H */
