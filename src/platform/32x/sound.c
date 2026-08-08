/*
 * TEMPEST 2000 32X - PWM sound synthesizer (slave SH2)
 */
#include <stdint.h>
#include <stddef.h>
#include "32x.h"
#include "sound.h"

#define SND_RATE 11025

/* shared control block, accessed uncached from both SH2s */
static inline uint32_t f2i(uint32_t f)
{
    return (uint32_t)(((uint64_t)f << 32) / SND_RATE);
}

typedef struct {
    volatile int32_t bgm;        /* SND_SONG_*, -1 = uninitialised   */
    volatile int32_t sfx[3];     /* 0 = idle, else SND_* trigger     */
    volatile int32_t started;    /* set once master side is ready    */
} snd_ctl_t;

#define SND_CTL ((snd_ctl_t *)0x2603C000)   /* uncached SDRAM window  */
static snd_ctl_t * const ctl = SND_CTL;

/* ------------------------------------------------------------------ */
/* master side                                                         */
/* ------------------------------------------------------------------ */

static int pwm_cycle = 2088;
static int pwm_center = 1044;

void snd_init(void)
{
    uint16_t sample = 1, ix;
    uint32_t rep = 4;

    ctl->bgm = -1;
    ctl->sfx[0] = ctl->sfx[1] = ctl->sfx[2] = 0;
    ctl->started = 0;

    if (MARS_VDP_DISPMODE & MARS_NTSC_FORMAT)
        pwm_cycle = (int)(((23011361u << 1) / SND_RATE + 1) >> 1) + 1;
    else
        pwm_cycle = (int)(((22801467u << 1) / SND_RATE + 1) >> 1) + 1;
    pwm_center = pwm_cycle / 2;

    MARS_PWM_MONO = 1;
    MARS_PWM_MONO = 1;
    MARS_PWM_MONO = 1;
    MARS_PWM_CYCLE = (uint16_t)pwm_cycle;
    MARS_PWM_CTRL = 0x0185;   /* TM = 1, RTP, RMD = right, LMD = left */

    {
        int steps = 128, step = (pwm_center + steps - 1) / steps;
        while (sample < pwm_center) {
            for (ix = 0; ix < rep; ix++) {
                while (MARS_PWM_MONO & 0x8000);
                MARS_PWM_MONO = sample;
            }
            sample = (uint16_t)(sample + step);
        }
    }
    MARS_PWM_MONO = (uint16_t)pwm_center;
    ctl->started = 1;
}

void snd_bgm(int song)
{
    if (ctl->started)
        ctl->bgm = song;
}

void snd_play(int ch, int id)
{
    if (ch < 1 || ch > 3 || !ctl->started)
        return;
    ctl->sfx[ch - 1] = id;
}

/* ------------------------------------------------------------------ */
/* slave side: the synthesizer                                         */
/* ------------------------------------------------------------------ */

static uint32_t note_inc[128];

static void init_notes(void)
{
    int n;
    for (n = 0; n < 128; n++) {
        int semis = n - 69;
        int oct = semis < 0 ? -((-semis) / 12 + 1) : semis / 12;
        int st = semis - oct * 12;
        static const uint32_t semi[12] = {
            65536, 69433, 73562, 77936, 82570, 87490,
            92724, 98268, 104096, 110327, 116912, 123879,
        };
        uint32_t inc = (uint32_t)(((uint64_t)440 * semi[st]) >> 16);
        if (oct > 0)
            inc <<= oct;
        else if (oct < 0)
            inc >>= -oct;
        note_inc[n] = f2i(inc);
    }
}

static inline uint32_t n2i(int n)
{
    if (n < 0)
        n = 0;
    if (n > 127)
        n = 127;
    return note_inc[n];
}

typedef struct {
    uint32_t ph;      /* phase accumulator */
    uint32_t gate;    /* samples remaining */
    int      note;
    int      vol;     /* 0..32767 */
} voice_t;

typedef struct {
    uint32_t seed;
} noise_t;

static inline int32_t nz(noise_t *n)
{
    uint32_t x = n->seed;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    n->seed = x;
    return (int32_t)(x >> 16) - 32768;
}

static inline int32_t voice_render(voice_t *v, int duty)
{
    int32_t s;
    if (!v->gate)
        return 0;
    v->ph += n2i(v->note);
    s = (v->ph < (uint32_t)(0x100000000ull * duty / 256)) ? v->vol : -v->vol;
    v->gate--;
    return s;
}

/* ---------------- sfx state machines ---------------- */

typedef struct {
    int id;
    int32_t len;   /* remaining samples */
    int32_t t;     /* position */
    uint32_t ph, ph2;
    uint32_t inc, inc2;
    noise_t n;
} sfx_t;

static sfx_t sfx[3];

static void sfx_start(sfx_t *s, int id)
{
    s->id = id;
    s->t = 0;
    s->ph = s->ph2 = 0;
    s->n.seed = 0x1234567u + (uint32_t)id * 7777u;
    switch (id) {
    case SND_LASER:   s->inc = f2i(2200); break;
    case SND_EXPLODE: s->inc = f2i(60);   break;
    case SND_FLIPPER: s->inc = f2i(330);  break;
    case SND_ZAP:     s->inc = f2i(3500); s->inc2 = f2i(220); break;
    case SND_POWERUP: s->inc = f2i(523);  break;
    case SND_WARP:    s->inc = f2i(100);  break;
    case SND_LIFE:    s->inc = f2i(523);  break;
    default:          s->inc = 0; s->inc2 = 0; break;
    }
    switch (id) {
    case SND_LASER:   s->len = SND_RATE / 6;  break;
    case SND_EXPLODE: s->len = SND_RATE * 2 / 3; break;
    case SND_FLIPPER: s->len = SND_RATE / 25; break;
    case SND_ZAP:     s->len = SND_RATE * 4 / 5; break;
    case SND_POWERUP: s->len = SND_RATE * 2 / 5; break;
    case SND_WARP:    s->len = SND_RATE * 4 / 5; break;
    case SND_LIFE:    s->len = SND_RATE * 3 / 5; break;
    default:          s->len = 0; break;
    }
}

static int32_t sfx_render(sfx_t *s)
{
    uint32_t k = (uint32_t)s->t;
    int32_t out = 0;
    uint32_t env;

    switch (s->id) {
    case SND_LASER: {
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        s->ph += s->inc;
        if (s->inc > f2i(440)) s->inc -= f2i(15);
        out = ((s->ph < 0x80000000u) ? 18000 : -18000);
        out = (int32_t)(((int64_t)out * (int32_t)env) >> 16);
        break;
    }
    case SND_EXPLODE: {
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        s->ph += s->inc;
        out = ((s->ph < 0x80000000u) ? 14000 : -14000);
        out += nz(&s->n);
        out = (int32_t)(((int64_t)out * (int32_t)env * (int32_t)env) >> 32);
        break;
    }
    case SND_FLIPPER: {
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        s->ph += s->inc;
        out = ((s->ph < 0x80000000u) ? 10000 : -10000);
        out = (int32_t)(((int64_t)out * (int32_t)env) >> 16);
        break;
    }
    case SND_ZAP: {
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        s->ph += s->inc;
        if (s->inc > s->inc2) s->inc -= f2i(35);
        out = ((s->ph < 0x80000000u) ? 20000 : -20000);
        out += (nz(&s->n) >> 2);
        out = (int32_t)(((int64_t)out * (int32_t)env) >> 16);
        break;
    }
    case SND_POWERUP: {
        static const int notes[4] = { 72, 76, 79, 84 };
        uint32_t step = (k * 4) / (uint32_t)s->len;
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        if (step > 3) step = 3;
        s->ph += n2i(notes[step]);
        out = ((s->ph < 0x80000000u) ? 15000 : -15000);
        out = (int32_t)(((int64_t)out * (int32_t)env) >> 16);
        break;
    }
    case SND_WARP: {
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        s->ph += s->inc;
        s->inc += f2i(20);
        out = ((s->ph < 0x80000000u) ? 12000 : -12000) + (nz(&s->n) >> 1);
        out = (int32_t)(((int64_t)out * (int32_t)env) >> 16);
        break;
    }
    case SND_LIFE: {
        static const int notes[4] = { 72, 79, 84, 88 };
        uint32_t step = (k * 4) / (uint32_t)s->len;
        env = 65536 - (k * 65536) / (uint32_t)s->len;
        if (step > 3) step = 3;
        s->ph += n2i(notes[step]);
        out = ((s->ph < 0x80000000u) ? 18000 : -18000);
        out = (int32_t)(((int64_t)out * (int32_t)env) >> 16);
        break;
    }
    default:
        break;
    }
    s->t++;
    if (s->t >= s->len)
        s->id = 0;
    return out;
}

/* ---------------- bgm sequencer ---------------- */

typedef struct {
    int8_t bass[32];   /* midi notes, -1 = rest */
    int8_t arp[32];
    uint8_t duty;
    int tempo;         /* steps per second */
} song_t;

static song_t song_title = {
    { 45,-1,45,-1, 48,-1,45,-1, 50,-1,50,-1, 48,-1,55,-1,
      45,-1,45,-1, 48,-1,45,-1, 50,-1,50,-1, 48,-1,43,-1 },
    { 69,-1,72,-1, 76,-1,72,-1, 69,-1,72,-1, 76,-1,72,-1,
      69,-1,72,-1, 76,-1,74,-1, 72,-1,70,-1, 67,-1,70,-1 },
    128, 7,
};

static song_t song_techno = {
    { 36,36,-1,36, 36,36,-1,36, 39,-1,39,39, 36,-1,36,-1,
      36,36,-1,36, 36,36,-1,36, 41,-1,41,41, 39,-1,39,-1 },
    { 60,-1,60,-1, 63,-1,60,-1, 65,-1,63,-1, 67,-1,65,-1,
      60,-1,60,-1, 63,-1,60,-1, 70,-1,68,-1, 67,-1,65,-1 },
    128, 10,
};

typedef struct {
    const song_t *song;
    int step;
    int32_t step_len, step_pos;
    voice_t vbass, varp;
    noise_t hat;
    int song_id;
} bgm_t;

static bgm_t bgm;

static void bgm_set(int id)
{
    bgm.song_id = id;
    bgm.step = 0;
    bgm.step_pos = 0;
    bgm.vbass.gate = 0;
    bgm.varp.gate = 0;
    switch (id) {
    case SND_SONG_TITLE:  bgm.song = &song_title;  break;
    case SND_SONG_TECHNO: bgm.song = &song_techno; break;
    default:              bgm.song = NULL;        break;
    }
    if (bgm.song)
        bgm.step_len = SND_RATE / bgm.song->tempo;
}

static int32_t bgm_render(void)
{
    int32_t s;
    if (!bgm.song)
        return 0;

    if (bgm.step_pos == 0) {
        int note = bgm.song->bass[bgm.step];
        if (note >= 0) {
            bgm.vbass.note = note;
            bgm.vbass.vol  = 8000;
            bgm.vbass.gate = bgm.step_len * 9 / 10;
        }
        note = bgm.song->arp[bgm.step];
        if (note >= 0) {
            bgm.varp.note = note;
            bgm.varp.vol  = 4000;
            bgm.varp.gate = bgm.step_len * 7 / 10;
        }
        bgm.step = (bgm.step + 1) & 31;
    }

    s  = voice_render(&bgm.vbass, 128);
    s += voice_render(&bgm.varp,  bgm.song->duty);
    if ((bgm.step & 1) == 0 && bgm.step_pos < bgm.step_len / 6)
        s += nz(&bgm.hat) >> 3;

    bgm.step_pos++;
    if (bgm.step_pos >= bgm.step_len)
        bgm.step_pos = 0;
    return s;
}

/* ---------------- main mixing loop (secondary SH2) ---------------- */

void snd_slave(void)
{
    init_notes();
    bgm.hat.seed = 0x87654321u;

    for (;;) {
        int32_t req, out;
        uint16_t sample;

        while (MARS_PWM_MONO & 0x8000)
            ;

        req = ctl->bgm;
        if (req != -1) {
            ctl->bgm = -1;
            bgm_set(req);
        }

        out = bgm_render();
        for (int i = 0; i < 3; i++) {
            req = ctl->sfx[i];
            if (req) {
                ctl->sfx[i] = 0;
                sfx_start(&sfx[i], req);
            }
            if (sfx[i].id)
                out += sfx_render(&sfx[i]);
        }

        if (out > 32767)  out = 32767;
        if (out < -32768) out = -32768;

        sample = (uint16_t)(pwm_center + ((out * pwm_center) >> 15));
        MARS_PWM_MONO = sample;
    }
}
