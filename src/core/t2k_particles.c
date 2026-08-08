/*
 * TEMPEST 2000 32X - Starfield, Explosions & Powerups implementation
 */
#include "t2k_particles.h"
#include "t2k_web.h"
#include "t2k_player.h"
#include "t2k_math.h"
#include "sound.h"
#include "gfx.h"

t2k_star_t g_stars[MAX_STARS];
t2k_spark_t g_sparks[MAX_SPARKS];
t2k_powerup_t g_powerups[MAX_POWERUPS];

static uint32_t s_prng = 0x8899AABB;
static uint32_t t2k_rnd(void)
{
    s_prng ^= s_prng << 13;
    s_prng ^= s_prng >> 17;
    s_prng ^= s_prng << 5;
    return s_prng;
}

void t2k_particles_init(void)
{
    int i;
    for (i = 0; i < MAX_STARS; i++) {
        g_stars[i].x = (int)(t2k_rnd() % 400) - 200;
        g_stars[i].y = (int)(t2k_rnd() % 300) - 150;
        g_stars[i].z = 20 + (int)(t2k_rnd() % 480);
    }
    for (i = 0; i < MAX_SPARKS; i++)
        g_sparks[i].active = 0;
    for (i = 0; i < MAX_POWERUPS; i++)
        g_powerups[i].active = 0;
}

void t2k_particles_spawn_explosion(int sx, int sy, int count)
{
    int i, n = 0;
    for (i = 0; i < MAX_SPARKS && n < count; i++) {
        if (!g_sparks[i].active) {
            g_sparks[i].active = 1;
            g_sparks[i].x = sx;
            g_sparks[i].y = sy;
            int angle = (int)(t2k_rnd() & 255);
            int spd = 2 + (int)(t2k_rnd() % 6);
            g_sparks[i].vx = (int)((int64_t)spd * t2k_cos(angle) >> 16);
            g_sparks[i].vy = (int)((int64_t)spd * t2k_sin(angle) >> 16);
            g_sparks[i].life = 15 + (int)(t2k_rnd() % 20);
            g_sparks[i].color = 6 + (uint8_t)(t2k_rnd() % 6); /* Yellow/Red/White */
            n++;
        }
    }
}

void t2k_particles_spawn_powerup(int lane, int depth)
{
    int i;
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (!g_powerups[i].active) {
            g_powerups[i].active = 1;
            g_powerups[i].lane = lane;
            g_powerups[i].depth = depth;
            g_powerups[i].type = 1 + (int)(t2k_rnd() % 5);
            break;
        }
    }
}

void t2k_particles_update(void)
{
    int i;

    /* Update stars */
    for (i = 0; i < MAX_STARS; i++) {
        g_stars[i].z -= 6;
        if (g_stars[i].z <= 15) {
            g_stars[i].z = 500;
            g_stars[i].x = (int)(t2k_rnd() % 400) - 200;
            g_stars[i].y = (int)(t2k_rnd() % 300) - 150;
        }
    }

    /* Update explosion sparks */
    for (i = 0; i < MAX_SPARKS; i++) {
        if (g_sparks[i].active) {
            g_sparks[i].x += g_sparks[i].vx;
            g_sparks[i].y += g_sparks[i].vy;
            g_sparks[i].life--;
            if (g_sparks[i].life <= 0)
                g_sparks[i].active = 0;
        }
    }

    /* Update powerups */
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (!g_powerups[i].active)
            continue;

        g_powerups[i].depth -= 4;
        if (g_powerups[i].depth <= 0) {
            g_powerups[i].depth = 0;
            /* Check if player caught it */
            if (g_powerups[i].lane == g_player.lane) {
                g_powerups[i].active = 0;
                g_player.score += 2000;
                snd_play(2, SND_POWERUP);

                switch (g_powerups[i].type) {
                case POWERUP_LASER: g_player.has_laser = 1; break;
                case POWERUP_JUMP:  g_player.has_jump  = 1; break;
                case POWERUP_DROID: g_player.has_droid = 1; break;
                case POWERUP_ZAP:   g_player.zap_state = 2; break;
                case POWERUP_2000:  g_player.score += 2000; break;
                }
            } else {
                g_powerups[i].active = 0;
            }
        }
    }
}

void t2k_particles_render(void)
{
    int i;

    /* Render starfield */
    for (i = 0; i < MAX_STARS; i++) {
        int sx, sy;
        int scale = t2k_project(g_stars[i].x, g_stars[i].y, g_stars[i].z, &sx, &sy);
        uint8_t col = (scale > 100) ? 11 : ((scale > 60) ? 12 : 13);
        gfx_pset(sx, sy, col);
    }

    /* Render powerups */
    for (i = 0; i < MAX_POWERUPS; i++) {
        if (g_powerups[i].active) {
            int x, y, z, sx, sy;
            t2k_web_get_lane_center(g_powerups[i].lane, g_powerups[i].depth, &x, &y, &z);
            t2k_project(x, y, z, &sx, &sy);
            gfx_circle_fill(sx, sy, 4, 6); /* Yellow capsule */
            gfx_circle(sx, sy, 5, 11);     /* White ring */
        }
    }

    /* Render explosion sparks */
    for (i = 0; i < MAX_SPARKS; i++) {
        if (g_sparks[i].active) {
            gfx_pset(g_sparks[i].x, g_sparks[i].y, g_sparks[i].color);
            gfx_pset(g_sparks[i].x + 1, g_sparks[i].y, g_sparks[i].color);
        }
    }
}
