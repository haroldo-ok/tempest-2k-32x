/*
 * TEMPEST 2000 32X - Player Crawler Claw implementation
 */
#include "t2k_player.h"
#include "t2k_web.h"
#include "t2k_math.h"
#include "t2k_enemies.h"
#include "t2k_particles.h"
#include "sound.h"
#include "gfx.h"
#include "32x.h"

t2k_player_t g_player;

void t2k_player_init(void)
{
    g_player.lane = 0;
    g_player.sub_lane = 0;
    g_player.score = 0;
    g_player.lives = 3;
    g_player.level = 1;
    g_player.zap_state = 2;
    g_player.zap_timer = 0;
    g_player.has_laser = 0;
    g_player.has_jump = 0;
    g_player.has_droid = 0;
    g_player.jump_height = 0;
    g_player.jump_vel = 0;
    g_player.droid_angle = 0;
    g_player.is_dead = 0;
    g_player.death_timer = 0;
}

void t2k_player_reset_for_level(int level)
{
    g_player.level = level;
    g_player.lane = 0;
    g_player.sub_lane = 0;
    g_player.zap_state = 2;
    g_player.zap_timer = 0;
    g_player.jump_height = 0;
    g_player.jump_vel = 0;
    g_player.is_dead = 0;
    g_player.death_timer = 0;
}

void t2k_player_die(void)
{
    if (g_player.is_dead)
        return;

    g_player.is_dead = 1;
    g_player.death_timer = 60;
    g_player.lives--;
    g_player.has_laser = 0;
    g_player.has_jump = 0;
    g_player.has_droid = 0;

    int x, y, z, sx, sy;
    t2k_web_get_lane_center(g_player.lane, 0, &x, &y, &z);
    t2k_project(x, y, 0, &sx, &sy);
    t2k_particles_spawn_explosion(sx, sy, 10);
    snd_play(1, SND_EXPLODE);
}

void t2k_player_update(uint16_t pad)
{
    int n = g_cur_web->num_lanes;

    if (g_player.is_dead) {
        if (g_player.death_timer > 0) {
            g_player.death_timer--;
        } else if (g_player.lives > 0) {
            g_player.is_dead = 0;
            g_player.lane = 0;
            g_player.sub_lane = 0;
            g_player.zap_state = 2;
        }
        return;
    }

    if (g_player.zap_timer > 0) {
        g_player.zap_timer--;
    }

    /* Left / Right movement */
    if (pad & SEGA_CTRL_LEFT) {
        g_player.sub_lane -= 32;
        if (g_player.sub_lane < 0) {
            g_player.sub_lane += 256;
            g_player.lane--;
            if (g_player.lane < 0) {
                if (g_cur_web->is_closed)
                    g_player.lane = n - 1;
                else {
                    g_player.lane = 0;
                    g_player.sub_lane = 0;
                }
            }
        }
    } else if (pad & SEGA_CTRL_RIGHT) {
        g_player.sub_lane += 32;
        if (g_player.sub_lane >= 256) {
            g_player.sub_lane -= 256;
            g_player.lane++;
            if (g_player.lane >= n) {
                if (g_cur_web->is_closed)
                    g_player.lane = 0;
                else {
                    g_player.lane = n - 1;
                    g_player.sub_lane = 255;
                }
            }
        }
    }

    /* Jump off rim */
    if ((pad & SEGA_CTRL_UP) && g_player.has_jump && g_player.jump_height == 0) {
        g_player.jump_vel = 12;
        g_player.jump_height = 1;
    }

    if (g_player.jump_height > 0) {
        g_player.jump_height += g_player.jump_vel;
        g_player.jump_vel -= 1;
        if (g_player.jump_height <= 0) {
            g_player.jump_height = 0;
            g_player.jump_vel = 0;
        }
    }

    /* Superzapper (C button or START during gameplay) */
    if ((pad & (SEGA_CTRL_C | SEGA_CTRL_START)) && g_player.zap_state > 0 && g_player.zap_timer == 0) {
        g_player.zap_timer = 20;
        snd_play(2, SND_ZAP);
        t2k_enemies_zap_all(g_player.zap_state == 2);
        g_player.zap_state--;
    }

    /* Update AI Droid */
    if (g_player.has_droid) {
        g_player.droid_angle = (g_player.droid_angle + 4) & 255;
    }
}

void t2k_player_render(void)
{
    if (g_player.is_dead)
        return;

    int x0, y0, z0, x1, y1, z1;
    int sx0, sy0, sx1, sy1;
    int n = g_cur_web->num_lanes;
    int l0 = g_player.lane;
    int l1 = (l0 + 1) % n;
    if (!g_cur_web->is_closed && l0 == n - 1) l1 = l0;

    t2k_web_get_vertex(l0, 0, &x0, &y0, &z0);
    t2k_web_get_vertex(l1, 0, &x1, &y1, &z1);

    t2k_project(x0, y0, 0, &sx0, &sy0);
    t2k_project(x1, y1, 0, &sx1, &sy1);

    /* Interpolate claw position on rim segment */
    int t = g_player.sub_lane;
    int cx = sx0 + (((sx1 - sx0) * t) >> 8);
    int cy = sy0 + (((sy1 - sy0) * t) >> 8);

    /* Offset for jump height */
    cy -= (g_player.jump_height >> 1);

    /* Draw Tempest Crawler Claw (Yellow wings, Cyan core) */
    int w = 8;
    int h = 6;
    int nx, ny;
    t2k_web_get_lane_normal(l0, &nx, &ny);
    int px = cx - ((nx * h) / 100);
    int py = cy - ((ny * h) / 100);

    /* Yellow wings */
    gfx_line(cx - w, cy - 3, px, py, 6);
    gfx_line(cx + w, cy - 3, px, py, 6);
    gfx_line(cx - w, cy - 3, cx + w, cy - 3, 6);

    /* Cyan core */
    gfx_circle_fill(cx, cy - 1, 2, 3);
}

void t2k_droid_render(void)
{
    if (!g_player.has_droid || g_player.is_dead)
        return;

    int r = 90;
    int x = (int)((int64_t)r * t2k_cos(g_player.droid_angle) >> 16);
    int y = (int)((int64_t)r * t2k_sin(g_player.droid_angle) >> 16);
    int sx, sy;
    t2k_project(x, y, 50, &sx, &sy);
    gfx_circle_fill(sx, sy, 4, 7); /* Bright lime green sphere */
    gfx_circle(sx, sy, 5, 11);     /* White outline */
}
