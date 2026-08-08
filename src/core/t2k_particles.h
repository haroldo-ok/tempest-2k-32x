/*
 * TEMPEST 2000 32X - Starfield, Explosions & Powerups
 */
#ifndef T2K_PARTICLES_H
#define T2K_PARTICLES_H

#include <stdint.h>

#define MAX_STARS 96
#define MAX_SPARKS 32
#define MAX_POWERUPS 8

enum {
    POWERUP_LASER = 1,
    POWERUP_JUMP  = 2,
    POWERUP_DROID = 3,
    POWERUP_ZAP   = 4,
    POWERUP_2000  = 5,
};

typedef struct {
    int x, y, z;
} t2k_star_t;

typedef struct {
    int active;
    int x, y;
    int vx, vy;
    int life;
    uint8_t color;
} t2k_spark_t;

typedef struct {
    int active;
    int type;
    int lane;
    int depth;
} t2k_powerup_t;

extern t2k_star_t g_stars[MAX_STARS];
extern t2k_spark_t g_sparks[MAX_SPARKS];
extern t2k_powerup_t g_powerups[MAX_POWERUPS];

void t2k_particles_init(void);
void t2k_particles_spawn_explosion(int sx, int sy, int count);
void t2k_particles_spawn_powerup(int lane, int depth);
void t2k_particles_update(void);
void t2k_particles_render(void);

#endif /* T2K_PARTICLES_H */
