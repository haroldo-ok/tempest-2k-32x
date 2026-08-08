/*
 * TEMPEST 2000 32X - SH-2 entry points (platform shell)
 */
#include "marsl.h"
#include "sound.h"
#include "t2k_game.h"

/* runs on the primary SH2 (cold start from crt0.s) */
int main(void)
{
    Mars_Init();
    Mars_InitVideo();
    snd_init();
    t2k_game_init();

    for (;;) {
        uint16_t pad = Mars_ReadController(0);
        t2k_game_tick(pad);
        t2k_game_render();
        Mars_FlipFrameBuffers(1);
    }
    return 0;
}

/* runs on the secondary SH2 */
void secondary(void)
{
    snd_slave();
}
