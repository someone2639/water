#pragma once

#include <math.h>

static inline float vmag(float *v) {
    return (v[0] * v[0]) + (v[1] * v[1]) + (v[2] * v[2]);
}

static inline float fscale(float f, float omin, float omax, float nmin, float nmax) {
    return (((f - omin) * (nmax - nmin)) / (omax - omin)) + nmin;
}
