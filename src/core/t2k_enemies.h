/*
 * TEMPEST 2000 32X - Enemies, Spikes & Bullets
 */
#ifndef T2K_ENEMIES_H
#define T2K_ENEMIES_H

#include <stdint.h>
#include "t2k_web.h"

enum {
    ENEMY_FLIPPER  = 1,
    ENEMY_TANKER   = 2,
    ENEMY_SPIKER   = 3,
    ENEMY_FUSEBALL = 4,
    ENEMY_PULSAR   = 5,
};

#define MAX_ENEMIES 32
#define MAX_BULLETS 24

typedef struct {
    int active;
    int type;
    int lane;
    int depth;     /* 0..500 (500 = deep end, 0 = outer rim) */
    int flip_timer;
    int flip_dir;  /* -1 or +1 */
    int pulse_timer;
    int anim_frame;
} t2k_enemy_t;

typedef struct {
    int active;
    int lane;
    int depth;     /* 0..500 */
} t2k_bullet_t;

extern t2k_enemy_t g_enemies[MAX_ENEMIES];
extern t2k_bullet_t g_bullets[MAX_BULLETS];
extern int g_spike_len[MAX_LANES];

void t2k_enemies_init(void);
void t2k_enemies_spawn_wave(int level);
void t2k_enemies_update(void);
void t2k_enemies_render(void);
void t2k_bullets_fire(int lane);
void t2k_enemies_zap_all(int full_zap);
int  t2k_enemies_count(void);

#endif /* T2K_ENEMIES_H */
