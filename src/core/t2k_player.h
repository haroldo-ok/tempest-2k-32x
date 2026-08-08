/*
 * TEMPEST 2000 32X - Player Crawler Claw & AI Droid
 */
#ifndef T2K_PLAYER_H
#define T2K_PLAYER_H

#include <stdint.h>

typedef struct {
    int lane;
    int sub_lane;
    int score;
    int lives;
    int level;
    int zap_state; /* 2=full screen zap, 1=single enemy zap, 0=used */
    int zap_timer;
    int has_laser;
    int has_jump;
    int has_droid;
    int jump_height;
    int jump_vel;
    int droid_angle;
    int is_dead;
    int death_timer;
} t2k_player_t;

extern t2k_player_t g_player;

void t2k_player_init(void);
void t2k_player_reset_for_level(int level);
void t2k_player_update(uint16_t pad);
void t2k_player_die(void);
void t2k_player_render(void);
void t2k_droid_render(void);

#endif /* T2K_PLAYER_H */
