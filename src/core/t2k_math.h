/*
 * TEMPEST 2000 32X - Fixed point math & projection helpers
 */
#ifndef T2K_MATH_H
#define T2K_MATH_H

#include <stdint.h>

typedef int32_t fx;
#define FX_ONE 65536
#define FX(n)  ((fx)((n) * 65536))

static inline fx fx_mul(fx a, fx b) {
    return (fx)(((int64_t)a * b) >> 16);
}

static inline int t2k_abs(int x) {
    return (x < 0) ? -x : x;
}

void t2k_math_init(void);
fx t2k_sin(int angle);  /* angle: 0..255 */
fx t2k_cos(int angle);  /* angle: 0..255 */

/* Projects a 3D point (x, y, z) where z=0 is outer rim and z=500 is deep end
 * into screen coordinates (sx, sy) on 320x224 screen.
 * x, y are in arbitrary world coordinates (-160..160, -112..112).
 * Returns scale factor (0..256) for sprite/point sizing. */
int t2k_project(int x, int y, int z, int *sx, int *sy);

#endif /* T2K_MATH_H */
