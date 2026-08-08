/*
 * TEMPEST 2000 32X - sound (PWM, mixed on the slave SH2)
 */
#ifndef T2K_SOUND_H
#define T2K_SOUND_H

enum {
    SND_LASER   = 1,   /* chirp / pew laser */
    SND_EXPLODE = 2,   /* enemy explosion noise */
    SND_FLIPPER = 3,   /* flipper movement step */
    SND_ZAP     = 4,   /* superzapper frequency sweep */
    SND_POWERUP = 5,   /* bonus capsule chime */
    SND_WARP    = 6,   /* warp tunnel zoom */
    SND_LIFE    = 7,   /* 1UP jingle */
};

enum {
    SND_SONG_NONE   = 0,
    SND_SONG_TITLE  = 1,
    SND_SONG_TECHNO = 2,
};

void snd_init(void);          /* called once on master before slave starts */
void snd_bgm(int song);       /* switch background music */
void snd_play(int ch, int id);/* trigger one-shot effect on slot ch (1..3) */
void snd_clear_fb_slave(uint8_t color); /* offload FB clear to Slave SH-2 */
void snd_wait_clear_slave(void);        /* wait for Slave SH-2 clear to finish */
void snd_slave(void);         /* mixing loop; runs on slave SH2 */

#endif /* T2K_SOUND_H */
