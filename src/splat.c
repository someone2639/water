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
uint32_t numSubNormals = 0;

#define PART_SCALE_MAX (10.0f)
#define REJECT_POINTS_FARTHER_THAN (10000.0f)
// #define FIND_CENTER

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

int16_t convert_float(float f) {
    return (int16_t) (f);
    // return (int16_t)(fscale(f, -240.0, 240.0, -256, 255));
}

/**
 * Static particles simulating grass.
 * This will create a random grid of 3 particles stacked on top of each other representing grass-blades.
 */
int read_splat(TPXParticleS16 *particles, char *path) {
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    fread(particles, fsize, 1, f);

#ifdef FIND_CENTER
    float center[3] = {0, 0, 0};
    for (int i = 0; i < (fsize / sizeof(TPXParticleS16)); i++) {
        TPXParticleS16 *s = &particles[i];
        center[0] += s->posA[0];
        center[1] += s->posA[1];
        center[2] += s->posA[2];

        center[0] += s->posB[0];
        center[1] += s->posB[1];
        center[2] += s->posB[2];
    }

    center[0] /= (fsize / sizeof(TPXParticleS16));
    center[1] /= (fsize / sizeof(TPXParticleS16));
    center[2] /= (fsize / sizeof(TPXParticleS16));

    debugf("CENTER: %f %f %f\n", center[0], center[1], center[2]);
#endif

    return (fsize / sizeof(TPXParticleS16)) * 2;
}

