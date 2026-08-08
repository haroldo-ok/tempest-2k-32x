/*
 * TEMPEST 2000 32X - Fixed point math & projection implementation
 */
#include "t2k_math.h"

extern const int32_t wf_sin_tab[1280];

static uint32_t recip_tab[1024];

void t2k_math_init(void)
{
    int z;
    for (z = 0; z < 1024; z++) {
        /* Scale factor for projection: scale = (32000 / (z + 100)) */
        recip_tab[z] = (uint32_t)(32000 / (z + 100));
    }
}

fx t2k_sin(int angle)
{
    return wf_sin_tab[(angle * 4) & 1023];
}

fx t2k_cos(int angle)
{
    return wf_sin_tab[((angle * 4) + 256) & 1023];
}

int t2k_project(int x, int y, int z, int *sx, int *sy)
{
    uint32_t scale;
    if (z < 0)    z = 0;
    if (z > 1023) z = 1023;
    scale = recip_tab[z];

    *sx = 160 + ((x * (int)scale) >> 8);
    *sy = 112 + ((y * (int)scale) >> 8);

    return (int)scale;
}
