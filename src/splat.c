#include <libdragon.h>
#include <rspq_profile.h>

#include <t3d/t3d.h>
#include <t3d/t3ddebug.h>

#include <t3d/t3dmodel.h>
#include <t3d/tpx.h>

#include "splat.h"

typedef struct SplatPart {
    float pos[3];
    float scale[3];
    uint8_t color[4];
    uint8_t rotation[4];
} SplatPart;

float read_float_le(FILE *f) {
    char buf[4];
    fread(buf, 4, 1, f);

    char ret[4];
    ret[0] = buf[3];
    ret[1] = buf[2];
    ret[2] = buf[1];
    ret[3] = buf[0];

    return *(float *)ret;
}

uint8_t read_u8(FILE *f) {
    uint8_t buf[4];
    fread(buf, 1, 1, f);
    return buf[0];
}

int16_t convert_float(float f) {
    return 
}

/**
 * Static particles simulating grass.
 * This will create a random grid of 3 particles stacked on top of each other representing grass-blades.
 */
int read_splat(TPXParticleS16 *particles, char *path, uint32_t partCount) {
    FILE *f = fopen(path, "rb");

    uint32_t cursor = 0;

    while (!feof(f) && cursor < 200000) {
        SplatPart part;
        part.pos[0] = read_float_le(f);
        part.pos[1] = read_float_le(f);
        part.pos[2] = read_float_le(f);
        part.scale[0] = read_float_le(f);
        part.scale[1] = read_float_le(f);
        part.scale[2] = read_float_le(f);

        part.color[0] = read_u8(f);
        part.color[1] = read_u8(f);
        part.color[2] = read_u8(f);
        part.color[3] = read_u8(f);

        part.rotation[0] = read_u8(f);
        part.rotation[1] = read_u8(f);
        part.rotation[2] = read_u8(f);
        part.rotation[3] = read_u8(f);

        if ((cursor & 1) == 0) {
            particles[cursor].posA[0] = (int16_t)part.pos[0];
            particles[cursor].posA[1] = (int16_t)part.pos[1];
            particles[cursor].posA[2] = (int16_t)part.pos[2];
            particles[cursor].sizeA = 20;
            particles[cursor].colorA[0] = part.color[0];
            particles[cursor].colorA[1] = part.color[1];
            particles[cursor].colorA[2] = part.color[2];
            particles[cursor].colorA[3] = 8;
        } else {
            particles[cursor].posB[0] = (int16_t)part.pos[0];
            particles[cursor].posB[1] = (int16_t)part.pos[1];
            particles[cursor].posB[2] = (int16_t)part.pos[2];
            particles[cursor].sizeB = 20;
            particles[cursor].colorB[0] = part.color[0];
            particles[cursor].colorB[1] = part.color[1];
            particles[cursor].colorB[2] = part.color[2];
            particles[cursor].colorB[3] = 8;
        }

        cursor++;

        if ((cursor % 10000) == 0) {
            debugf("Processed %lu particles...\n", cursor);
        }
    }

    return cursor & ~1;
}

color_t blend_colors(color_t colorA, color_t colorB, float t) {
    color_t color;
    color.r = (uint8_t) (colorA.r * (1.0f - t) + colorB.r * t);
    color.g = (uint8_t) (colorA.g * (1.0f - t) + colorB.g * t);
    color.b = (uint8_t) (colorA.b * (1.0f - t) + colorB.b * t);
    color.a = (uint8_t) (colorA.a * (1.0f - t) + colorB.a * t);
    return color;
}
