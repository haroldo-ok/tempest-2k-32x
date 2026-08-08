/*
 * TEMPEST 2000 32X - Top-level Game State Machine implementation
 */
#include "t2k_game.h"
#include "t2k_render.h"
#include "t2k_player.h"
#include "t2k_enemies.h"
#include "t2k_particles.h"
#include "t2k_web.h"
#include "t2k_math.h"
#include "sound.h"
#include "32x.h"

int g_state = STATE_TITLE;
int g_state_timer = 0;
int g_menu_sel = 0;

static uint16_t s_old_pad = 0;
static int s_fire_timer = 0;

void t2k_game_init(void)
{
    t2k_math_init();
    t2k_web_init();
    t2k_particles_init();
    t2k_enemies_init();
    t2k_player_init();
    t2k_render_init();

    g_state = STATE_TITLE;
    g_state_timer = 0;
    g_menu_sel = 0;
    snd_bgm(SND_SONG_TITLE);
}

void t2k_game_tick(uint16_t pad)
{
    snd_clear_fb_slave(0);
    t2k_particles_update();

    uint16_t pressed = pad & ~s_old_pad;
    s_old_pad = pad;
    g_state_timer++;

    if (s_fire_timer > 0)
        s_fire_timer--;

    if (g_state == STATE_TITLE) {
        if ((pressed & (SEGA_CTRL_START | SEGA_CTRL_A | SEGA_CTRL_B | SEGA_CTRL_C)) && g_state_timer > 20) {
            g_state = STATE_MENU;
            g_state_timer = 0;
            g_menu_sel = 0;
        }
    }
    else if (g_state == STATE_MENU) {
        if (pressed & SEGA_CTRL_DOWN) {
            g_menu_sel = (g_menu_sel + 1) & 3;
            snd_play(3, SND_FLIPPER);
        } else if (pressed & SEGA_CTRL_UP) {
            g_menu_sel = (g_menu_sel - 1) & 3;
            snd_play(3, SND_FLIPPER);
        }

        if ((pressed & (SEGA_CTRL_START | SEGA_CTRL_A | SEGA_CTRL_B | SEGA_CTRL_C)) && g_state_timer > 10) {
            if (g_menu_sel == 0) {
                /* START GAME */
                t2k_player_init();
                t2k_web_set_level(1);
                t2k_enemies_spawn_wave(1);
                g_state = STATE_READY;
                g_state_timer = 0;
                snd_bgm(SND_SONG_TECHNO);
            } else if (g_menu_sel == 1) {
                /* SELECT WEB */
                int next_lvl = (g_player.level % 16) + 1;
                t2k_player_reset_for_level(next_lvl);
                t2k_web_set_level(next_lvl);
                snd_play(2, SND_POWERUP);
            } else if (g_menu_sel == 2) {
                /* MUSIC TEST */
                snd_bgm(SND_SONG_TECHNO);
            } else if (g_menu_sel == 3) {
                /* CREDITS */
                snd_play(3, SND_LIFE);
            }
        }
    }
    else if (g_state == STATE_READY) {
        if (g_state_timer >= 60 || ((pressed & (SEGA_CTRL_START | SEGA_CTRL_A | SEGA_CTRL_B | SEGA_CTRL_C)) && g_state_timer > 15)) {
            g_state = STATE_PLAY;
            g_state_timer = 0;
        }
    }
    else if (g_state == STATE_PLAY) {
        t2k_player_update(pad);
        t2k_enemies_update();

        /* Firing bullets */
        if ((pad & (SEGA_CTRL_A | SEGA_CTRL_B)) && s_fire_timer == 0 && !g_player.is_dead) {
            t2k_bullets_fire(g_player.lane);
            s_fire_timer = g_player.has_laser ? 3 : 5;
        }

        /* Check level complete */
        if (t2k_enemies_count() == 0 && !g_player.is_dead) {
            g_state = STATE_WARP;
            g_state_timer = 0;
            snd_play(2, SND_WARP);
        }

        /* Check game over */
        if (g_player.lives <= 0 && !g_player.is_dead) {
            g_state = STATE_GAMEOVER;
            g_state_timer = 0;
            snd_bgm(SND_SONG_NONE);
        }
    }
    else if (g_state == STATE_WARP) {
        if (g_state_timer >= 90) {
            int next_lvl = g_player.level + 1;
            g_player.score += 5000;
            t2k_player_reset_for_level(next_lvl);
            t2k_web_set_level(next_lvl);
            t2k_enemies_spawn_wave(next_lvl);
            g_state = STATE_READY;
            g_state_timer = 0;
        }
    }
    else if (g_state == STATE_GAMEOVER) {
        if (g_state_timer >= 180 || ((pressed & SEGA_CTRL_START) && g_state_timer > 40)) {
            g_state = STATE_TITLE;
            g_state_timer = 0;
            snd_bgm(SND_SONG_TITLE);
        }
    }
}

void t2k_game_render(void)
{
    t2k_render_frame(g_state, g_state_timer, g_menu_sel);
}
