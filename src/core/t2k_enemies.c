/*
 * TEMPEST 2000 32X - Enemies, Spikes & Bullets implementation
 */
#include "t2k_enemies.h"
#include "t2k_player.h"
#include "t2k_math.h"
#include "t2k_particles.h"
#include "sound.h"
#include "gfx.h"

t2k_enemy_t g_enemies[MAX_ENEMIES];
t2k_bullet_t g_bullets[MAX_BULLETS];
int g_spike_len[MAX_LANES];

static uint32_t s_seed = 0x19942000;
static uint32_t t2k_rand(void)
{
    s_seed ^= s_seed << 13;
    s_seed ^= s_seed >> 17;
    s_seed ^= s_seed << 5;
    return s_seed;
}

void t2k_enemies_init(void)
{
    int i;
    for (i = 0; i < MAX_ENEMIES; i++)
        g_enemies[i].active = 0;
    for (i = 0; i < MAX_BULLETS; i++)
        g_bullets[i].active = 0;
    for (i = 0; i < MAX_LANES; i++)
        g_spike_len[i] = 0;
}

void t2k_enemies_spawn_wave(int level)
{
    int i, count;
    t2k_enemies_init();

    int n = g_cur_web->num_lanes;
    count = 6 + (level * 2);
    if (count > 28) count = 28;

    for (i = 0; i < count; i++) {
        g_enemies[i].active = 1;
        g_enemies[i].lane = (int)(t2k_rand() % n);
        g_enemies[i].depth = 480 + (int)(i * 40); /* Staggered spawn depth */
        g_enemies[i].flip_timer = 0;
        g_enemies[i].flip_dir = (t2k_rand() & 1) ? 1 : -1;
        g_enemies[i].pulse_timer = (int)(t2k_rand() % 60);
        g_enemies[i].anim_frame = 0;

        if (level == 1) {
            g_enemies[i].type = (i % 4 == 3) ? ENEMY_TANKER : ENEMY_FLIPPER;
        } else if (level == 2) {
            if (i % 5 == 0)      g_enemies[i].type = ENEMY_SPIKER;
            else if (i % 3 == 0) g_enemies[i].type = ENEMY_TANKER;
            else                 g_enemies[i].type = ENEMY_FLIPPER;
        } else {
            int r = (int)(t2k_rand() % 100);
            if (r < 45)      g_enemies[i].type = ENEMY_FLIPPER;
            else if (r < 70) g_enemies[i].type = ENEMY_TANKER;
            else if (r < 85) g_enemies[i].type = ENEMY_SPIKER;
            else if (r < 93) g_enemies[i].type = ENEMY_FUSEBALL;
            else             g_enemies[i].type = ENEMY_PULSAR;
        }
    }
}

int t2k_enemies_count(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].active)
            count++;
    }
    return count;
}

void t2k_bullets_fire(int lane)
{
    int i;
    for (i = 0; i < MAX_BULLETS; i++) {
        if (!g_bullets[i].active) {
            g_bullets[i].active = 1;
            g_bullets[i].lane = lane;
            g_bullets[i].depth = 0;
            snd_play(1, SND_LASER);
            break;
        }
    }
    /* If double fire powerup active, fire on adjacent lane too */
    if (g_player.has_laser) {
        int next_lane = (lane + 1) % g_cur_web->num_lanes;
        for (i = 0; i < MAX_BULLETS; i++) {
            if (!g_bullets[i].active) {
                g_bullets[i].active = 1;
                g_bullets[i].lane = next_lane;
                g_bullets[i].depth = 0;
                break;
            }
        }
    }
}

void t2k_enemies_zap_all(int full_zap)
{
    int i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (g_enemies[i].active) {
            if (full_zap || (i & 1)) {
                g_enemies[i].active = 0;
                g_player.score += 150;
                int x, y, z, sx, sy;
                t2k_web_get_lane_center(g_enemies[i].lane, g_enemies[i].depth, &x, &y, &z);
                t2k_project(x, y, z, &sx, &sy);
                t2k_particles_spawn_explosion(sx, sy, 6);
                if (!full_zap) break;
            }
        }
    }
}

static void spawn_flipper_at(int lane, int depth)
{
    int i;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active) {
            g_enemies[i].active = 1;
            g_enemies[i].type = ENEMY_FLIPPER;
            g_enemies[i].lane = lane;
            g_enemies[i].depth = depth;
            g_enemies[i].flip_dir = 1;
            g_enemies[i].flip_timer = 0;
            break;
        }
    }
}

void t2k_enemies_update(void)
{
    int i, j;
    int n = g_cur_web->num_lanes;

    /* Update enemies */
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active)
            continue;

        t2k_enemy_t *e = &g_enemies[i];
        e->anim_frame++;

        /* Move toward rim if depth > 0 */
        if (e->depth > 0) {
            int speed = (e->type == ENEMY_FUSEBALL) ? 5 : 3;
            e->depth -= speed;
            if (e->depth < 0)
                e->depth = 0;

            /* Spiker creates spike trail */
            if (e->type == ENEMY_SPIKER && e->depth < 400) {
                int spike_d = 500 - e->depth;
                if (spike_d > g_spike_len[e->lane])
                    g_spike_len[e->lane] = spike_d;
            }
        } else {
            /* At the rim (depth == 0) */
            if (e->type == ENEMY_FLIPPER || e->type == ENEMY_TANKER) {
                e->flip_timer++;
                if (e->flip_timer >= 24) {
                    e->flip_timer = 0;
                    e->lane += e->flip_dir;
                    if (e->lane < 0) {
                        e->lane = (g_cur_web->is_closed) ? n - 1 : 0;
                        e->flip_dir = 1;
                    } else if (e->lane >= n) {
                        e->lane = (g_cur_web->is_closed) ? 0 : n - 1;
                        e->flip_dir = -1;
                    }
                    snd_play(3, SND_FLIPPER);
                }
            } else if (e->type == ENEMY_SPIKER) {
                /* Spiker retreats when it reaches rim */
                e->depth = 500;
            } else if (e->type == ENEMY_FUSEBALL) {
                e->lane = (e->lane + 1) % n;
            }
        }

        /* Check collision with player claw */
        if (e->lane == g_player.lane && e->depth == 0) {
            if (g_player.jump_height == 0) {
                t2k_player_die();
            }
        }
    }

    /* Update bullets & check collisions */
    for (j = 0; j < MAX_BULLETS; j++) {
        if (!g_bullets[j].active)
            continue;

        t2k_bullet_t *b = &g_bullets[j];
        b->depth += 32;

        /* Hit spike? */
        int spike_top = 500 - g_spike_len[b->lane];
        if (g_spike_len[b->lane] > 0 && b->depth >= spike_top) {
            g_spike_len[b->lane] -= 16;
            if (g_spike_len[b->lane] < 0)
                g_spike_len[b->lane] = 0;
            b->active = 0;
            continue;
        }

        if (b->depth >= 500) {
            b->active = 0;
            continue;
        }

        /* Hit enemy? */
        for (i = 0; i < MAX_ENEMIES; i++) {
            if (!g_enemies[i].active)
                continue;

            t2k_enemy_t *e = &g_enemies[i];
            if (e->lane == b->lane && t2k_abs(e->depth - b->depth) < 28) {
                b->active = 0;
                e->active = 0;
                g_player.score += (e->type * 100);

                int x, y, z, sx, sy;
                t2k_web_get_lane_center(e->lane, e->depth, &x, &y, &z);
                t2k_project(x, y, z, &sx, &sy);
                t2k_particles_spawn_explosion(sx, sy, 8);
                snd_play(1, SND_EXPLODE);

                /* Tanker splits into two flippers */
                if (e->type == ENEMY_TANKER) {
                    int l_prev = (e->lane - 1 + n) % n;
                    int l_next = (e->lane + 1) % n;
                    spawn_flipper_at(l_prev, e->depth);
                    spawn_flipper_at(l_next, e->depth);
                }

                /* 25% chance to drop a bonus capsule */
                if ((t2k_rand() % 4) == 0) {
                    t2k_particles_spawn_powerup(e->lane, e->depth);
                }
                break;
            }
        }
    }
}

void t2k_enemies_render(void)
{
    int i;
    /* Render spikes */
    for (i = 0; i < g_cur_web->num_lanes; i++) {
        if (g_spike_len[i] > 0) {
            int x, y, z, sx0, sy0, sx1, sy1;
            t2k_web_get_lane_center(i, 500, &x, &y, &z);
            t2k_project(x, y, 500, &sx0, &sy0);

            int top_z = 500 - g_spike_len[i];
            if (top_z < 0) top_z = 0;
            t2k_project(x, y, top_z, &sx1, &sy1);

            gfx_line(sx0, sy0, sx1, sy1, 7); /* Lime green spike */
            gfx_pset(sx1, sy1, 11);          /* White spike tip */
        }
    }

    /* Render bullets */
    for (i = 0; i < MAX_BULLETS; i++) {
        if (g_bullets[i].active) {
            int x, y, z, sx, sy;
            t2k_web_get_lane_center(g_bullets[i].lane, g_bullets[i].depth, &x, &y, &z);
            t2k_project(x, y, z, &sx, &sy);
            gfx_circle_fill(sx, sy, 2, 6); /* Yellow bullet */
            gfx_pset(sx, sy, 11);          /* White center */
        }
    }

    /* Render enemies */
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (!g_enemies[i].active)
            continue;

        t2k_enemy_t *e = &g_enemies[i];
        int x, y, z, sx, sy;
        t2k_web_get_lane_center(e->lane, e->depth, &x, &y, &z);
        int scale = t2k_project(x, y, z, &sx, &sy);
        int r = (scale >> 4) + 3;

        switch (e->type) {
        case ENEMY_FLIPPER:
            /* Red / Purple Bowtie */
            gfx_tri(sx - r, sy, sx, sy - r, sx + r, sy, 10);
            gfx_tri(sx - r, sy, sx, sy + r, sx + r, sy, 5);
            break;

        case ENEMY_TANKER:
            /* Cyan Diamond */
            gfx_tri(sx, sy - r, sx - r, sy, sx, sy + r, 3);
            gfx_tri(sx, sy - r, sx + r, sy, sx, sy + r, 14);
            break;

        case ENEMY_SPIKER:
            /* Lime Green Spinner */
            gfx_line(sx - r, sy - r, sx + r, sy + r, 7);
            gfx_line(sx - r, sy + r, sx + r, sy - r, 7);
            gfx_circle(sx, sy, r, 7);
            break;

        case ENEMY_FUSEBALL:
            /* Glittering white / rainbow spark */
            gfx_circle_fill(sx, sy, r - 1, 11);
            gfx_line(sx - r, sy, sx + r, sy, 6);
            gfx_line(sx, sy - r, sx, sy + r, 3);
            break;

        case ENEMY_PULSAR:
            /* Pink / Red lightning */
            gfx_line(sx - r, sy - r, sx + r, sy + r, 5);
            gfx_line(sx + r, sy - r, sx - r, sy + r, 10);
            gfx_circle(sx, sy, r, 11);
            break;
        }
    }
}
