#include <libdragon.h>
#include <rspq_profile.h>

#include <t3d/t3d.h>
#include <t3d/t3ddebug.h>

#include <t3d/t3dmodel.h>
#include <t3d/tpx.h>

#include "splat.h"
#include "mymath.h"

uint32_t sampleEveryNVertices = 1;

float debugLargestScale = 0.0;

#define PART_SCALE_MAX (0.2f)
#define REJECT_POINTS_FARTHER_THAN (100.0f)

static void storeLargestScale(float f) {
    if (f > debugLargestScale) {
        debugLargestScale = f;
    }
}

void print_splat_info(SplatPart *part) {
    debugf("{%f %f %f} {%f %f %f}\n",
        part->pos[0], part->pos[1], part->pos[2],
        part->scale[0], part->scale[1], part->scale[2]
    );
}

float read_float_le(FILE *f) {
    char buf[4];
    assert(f != NULL);
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

int is_denormal(float x) {
    return fpclassify(x) == FP_SUBNORMAL;
}

int16_t convert_float(float f) {
    if (is_denormal(f)) {
        return 0;
    }
    // return (int16_t) (f * (1000.f / 240.f));
    return (int16_t)(fscale(f, -240.0, 240.0, -256, 255));
}

/**
 * Static particles simulating grass.
 * This will create a random grid of 3 particles stacked on top of each other representing grass-blades.
 */
int read_splat(TPXParticleS16 *particles, char *iniPath, uint32_t partCount) {
    char path[256];
    uint32_t numVertices = 0;

    FILE *ini = fopen(iniPath, "r");
    fscanf(ini, "%s %lu", path, &numVertices);
    fclose(ini);

    char finalpath[296];
    sprintf(finalpath, "rom://%s", path);

    FILE *f = fopen(finalpath, "rb");

    uint32_t cursor = 0;

    for (uint32_t sample = 0; sample < (numVertices); sample++) {
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

        if ((sample % sampleEveryNVertices) == 0) {
            if (vmag(part.pos) < REJECT_POINTS_FARTHER_THAN) {
                if ((cursor & 1) == 0) {
                    particles[cursor].posA[0] = convert_float(part.pos[0]);
                    particles[cursor].posA[1] = convert_float(part.pos[1]);
                    particles[cursor].posA[2] = convert_float(part.pos[2]);
                    storeLargestScale(part.scale[0]);
                    storeLargestScale(part.scale[1]);
                    storeLargestScale(part.scale[2]);
                    particles[cursor].sizeA = fscale(vmag(part.scale), 0, PART_SCALE_MAX, 1, 20);
                    particles[cursor].colorA[0] = part.color[0];
                    particles[cursor].colorA[1] = part.color[1];
                    particles[cursor].colorA[2] = part.color[2];
                    particles[cursor].colorA[3] = 8;
                } else {
                    particles[cursor].posB[0] = convert_float(part.pos[0]);
                    particles[cursor].posB[1] = convert_float(part.pos[1]);
                    particles[cursor].posB[2] = convert_float(part.pos[2]);
                    storeLargestScale(part.scale[0]);
                    storeLargestScale(part.scale[1]);
                    storeLargestScale(part.scale[2]);
                    particles[cursor].sizeB = fscale(vmag(part.scale), 0, PART_SCALE_MAX, 1, 20);
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
        }
    }

    debugf("LARGEST SCALE: %f\n", debugLargestScale);

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
