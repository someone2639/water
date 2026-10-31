#pragma once

#include <t3d/tpx.h>

typedef struct SplatPart {
    float pos[3];
    float scale[3];
    uint8_t color[4];
    uint8_t rotation[4];
} SplatPart;

color_t blend_colors(color_t colorA, color_t colorB, float t);
int read_splat(TPXParticleS16 *particles, char *path, uint32_t partCount);
