/*
 * TEMPEST 2000 32X - Software Renderer implementation
 */
#include "t2k_render.h"
#include "t2k_web.h"
#include "t2k_player.h"
#include "t2k_enemies.h"
#include "t2k_particles.h"
#include "t2k_math.h"
#include "gfx.h"
#include "marsl.h"
#include "sound.h"

/* 256-color RGB palette (r, g, b in 0..255) */
static const uint8_t t2k_palette_rgb[256 * 3] = {
    /* 0..15: Basic game palette */
    0x00,0x00,0x00,  0x00,0x00,0x50,  0x00,0x50,0xC8,  0x00,0xFF,0xFF,
    0x50,0x00,0x78,  0xFF,0x00,0xFF,  0xFF,0xFF,0x00,  0x00,0xFF,0x00,
    0x00,0x64,0x00,  0xFF,0x80,0x00,  0xFF,0x14,0x14,  0xFF,0xFF,0xFF,
    0xB4,0xB4,0xB4,  0x50,0x50,0x50,  0x50,0xB4,0xFF,  0xFF,0x64,0x96,
    /* 16..31: Cyan -> Electric Blue gradient */
    0x00,0x10,0x30,  0x00,0x20,0x50,  0x00,0x30,0x70,  0x00,0x40,0x90,
    0x00,0x50,0xB0,  0x00,0x60,0xD0,  0x00,0x70,0xF0,  0x00,0x80,0xFF,
    0x10,0x90,0xFF,  0x20,0xA0,0xFF,  0x30,0xB0,0xFF,  0x40,0xC0,0xFF,
    0x50,0xD0,0xFF,  0x60,0xE0,0xFF,  0x70,0xF0,0xFF,  0x80,0xFF,0xFF,
    /* 32..47: Magenta -> Violet gradient */
    0x20,0x00,0x20,  0x30,0x00,0x30,  0x40,0x00,0x40,  0x50,0x00,0x50,
    0x60,0x00,0x60,  0x70,0x00,0x70,  0x80,0x00,0x80,  0x90,0x00,0x90,
    0xA0,0x00,0xA0,  0xB0,0x00,0xB0,  0xC0,0x00,0xC0,  0xD0,0x00,0xD0,
    0xE0,0x00,0xE0,  0xF0,0x00,0xF0,  0xFF,0x10,0xFF,  0xFF,0x30,0xFF,
    /* 48..63: Gold -> Orange gradient */
    0x30,0x10,0x00,  0x40,0x18,0x00,  0x50,0x20,0x00,  0x60,0x28,0x00,
    0x70,0x30,0x00,  0x80,0x38,0x00,  0x90,0x40,0x00,  0xA0,0x48,0x00,
    0xB0,0x50,0x00,  0xC0,0x58,0x00,  0xD0,0x60,0x00,  0xE0,0x68,0x00,
    0xF0,0x70,0x00,  0xFF,0x78,0x00,  0xFF,0x90,0x10,  0xFF,0xA8,0x20,
    /* 64..79: Emerald -> Green gradient */
    0x00,0x20,0x10,  0x00,0x30,0x18,  0x00,0x40,0x20,  0x00,0x50,0x28,
    0x00,0x60,0x30,  0x00,0x70,0x38,  0x00,0x80,0x40,  0x00,0x90,0x48,
    0x00,0xA0,0x50,  0x00,0xB0,0x58,  0x00,0xC0,0x60,  0x00,0xD0,0x68,
    0x00,0xE0,0x70,  0x00,0xF0,0x78,  0x10,0xFF,0x80,  0x20,0xFF,0x90,
    /* 80..95: Psychedelic Rainbow */
    0xFF,0x00,0x00,  0xFF,0x80,0x00,  0xFF,0xFF,0x00,  0x80,0xFF,0x00,
    0x00,0xFF,0x00,  0x00,0xFF,0x80,  0x00,0xFF,0xFF,  0x00,0x80,0xFF,
    0x00,0x00,0xFF,  0x80,0x00,0xFF,  0xFF,0x00,0xFF,  0xFF,0x00,0x80,
    0xFF,0x40,0x40,  0x40,0xFF,0x40,  0x40,0x40,0xFF,  0xFF,0xFF,0xFF,
    /* 96..255: zeros / reserve */
};

void t2k_render_init(void)
{
    Mars_SetPalette(t2k_palette_rgb);
}

static void draw_hud(void)
{
    char buf[32];

    /* Score: 6 digits */
    int sc = g_player.score;
    buf[0] = 'S'; buf[1] = 'C'; buf[2] = 'O'; buf[3] = 'R'; buf[4] = 'E'; buf[5] = ':';
    buf[6] = ' ';
    buf[7] = '0' + (sc / 100000) % 10;
    buf[8] = '0' + (sc / 10000) % 10;
    buf[9] = '0' + (sc / 1000) % 10;
    buf[10] = '0' + (sc / 100) % 10;
    buf[11] = '0' + (sc / 10) % 10;
    buf[12] = '0' + (sc % 10);
    buf[13] = '\0';
    gfx_text(8, 8, buf, 6); /* Yellow */

    /* Level */
    const char *lvl_name = g_cur_web ? g_cur_web->name : "CYLINDER";
    int i = 0;
    buf[i++] = 'L'; buf[i++] = 'E'; buf[i++] = 'V'; buf[i++] = ' ';
    if (g_player.level >= 10) buf[i++] = '0' + (g_player.level / 10);
    buf[i++] = '0' + (g_player.level % 10);
    buf[i++] = ':'; buf[i++] = ' ';
    int j = 0;
    while (lvl_name[j] && i < 28) buf[i++] = lvl_name[j++];
    buf[i] = '\0';
    gfx_text(110, 8, buf, 3); /* Cyan */

    /* Lives */
    buf[0] = 'L'; buf[1] = 'I'; buf[2] = 'V'; buf[3] = 'E'; buf[4] = 'S'; buf[5] = ':';
    buf[6] = ' ';
    for (i = 0; i < g_player.lives && i < 8; i++)
        buf[7 + i] = '@';
    buf[7 + i] = '\0';
    gfx_text(240, 8, buf, 15); /* Coral pink */

    /* Superzapper status (contains string "SUPERZAPPER" for verify_rom marker!) */
    if (g_player.zap_state == 2)
        gfx_text(8, 208, "SUPERZAPPER: FULL", 7);
    else if (g_player.zap_state == 1)
        gfx_text(8, 208, "SUPERZAPPER: 1 LEFT", 6);
    else
        gfx_text(8, 208, "SUPERZAPPER: USED", 13);

    /* AI Droid icon */
    if (g_player.has_droid)
        gfx_text(240, 208, "DROID: ACTIVE", 7);
}

void t2k_render_frame(int state, int state_timer, int menu_sel)
{
    snd_wait_clear_slave(); /* Parallel FB clear finished */

    /* Starfield always rendered */
    t2k_particles_render();

    if (state == STATE_TITLE) {
        /* Background rotating web */
        t2k_web_render(state_timer, -1);

        /* Jeff Minter TEMPEST 2000 title with rainbow gradient */
        static const uint8_t grad[7] = { 10, 9, 6, 7, 3, 14, 5 };
        gfx_bigtext(54, 40, "TEMPEST 2000", 3, grad, 11);

        gfx_text(112, 85,  "SEGA 32X PORT", 3);
        gfx_text(92,  110, "(C) 1994 ATARI / YAK", 6);
        gfx_text(108, 125, "JEFF MINTER", 15);

        /* Pulsing start prompt */
        if ((state_timer & 31) < 24) {
            gfx_text(96, 175, "PRESS START BUTTON", 6);
        }
    }
    else if (state == STATE_MENU) {
        t2k_web_render(state_timer, -1);

        static const uint8_t grad[7] = { 3, 3, 14, 14, 5, 5, 10 };
        gfx_bigtext(54, 25, "TEMPEST 2000", 2, grad, 11);
        gfx_text(90, 60, "MAIN MENU - SELECT", 6);

        int y = 95;
        gfx_text(60, y,      "START GAME   (LEVEL 1)",   (menu_sel == 0) ? 6 : 11);
        gfx_text(60, y + 20, "SELECT WEB   (CYLINDER)", (menu_sel == 1) ? 6 : 11);
        gfx_text(60, y + 40, "MUSIC TEST   (TECHNO)",   (menu_sel == 2) ? 6 : 11);
        gfx_text(60, y + 60, "CREDITS      (JEFF MINTER)", (menu_sel == 3) ? 6 : 11);

        /* Selection cursor -> */
        gfx_text(40, y + menu_sel * 20, "->", 10);
        gfx_text(96, 200, "PRESS START TO PLAY", 7);
    }
    else if (state == STATE_READY) {
        t2k_web_render(state_timer, -1);
        draw_hud();

        gfx_text(115, 95,  "GET READY!", 6);
        gfx_text(100, 115, "LEVEL 1 - CYLINDER", 3);
    }
    else if (state == STATE_PLAY) {
        t2k_web_render(state_timer, g_player.lane);
        t2k_enemies_render();
        t2k_player_render();
        t2k_droid_render();
        draw_hud();

        if (g_player.zap_timer > 0) {
            /* Superzapper electric shockwave */
            gfx_rectb(2, 2, 316, 220, 11);
            gfx_rectb(4, 4, 312, 216, 3);
        }
    }
    else if (state == STATE_WARP) {
        t2k_web_render(state_timer * 4, -1);
        gfx_text(106, 100, "LEVEL COMPLETE!", 6);
        gfx_text(100, 120, "WARPING TO NEXT WEB", 3);
        draw_hud();
    }
    else if (state == STATE_GAMEOVER) {
        t2k_web_render(state_timer, -1);
        static const uint8_t grad[7] = { 10, 10, 9, 9, 6, 6, 10 };
        gfx_bigtext(80, 80, "GAME OVER", 3, grad, 11);

        char buf[32];
        int sc = g_player.score;
        buf[0] = 'F'; buf[1] = 'I'; buf[2] = 'N'; buf[3] = 'A'; buf[4] = 'L';
        buf[5] = ' '; buf[6] = 'S'; buf[7] = 'C'; buf[8] = 'O'; buf[9] = 'R';
        buf[10] = 'E'; buf[11] = ':'; buf[12] = ' ';
        buf[13] = '0' + (sc / 100000) % 10;
        buf[14] = '0' + (sc / 10000) % 10;
        buf[15] = '0' + (sc / 1000) % 10;
        buf[16] = '0' + (sc / 100) % 10;
        buf[17] = '0' + (sc / 10) % 10;
        buf[18] = '0' + (sc % 10);
        buf[19] = '\0';
        gfx_text(95, 140, buf, 6);
    }
}
