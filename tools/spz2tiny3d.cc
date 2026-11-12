#include <fstream>
#include <iostream>
#include <vector>
#include "load-spz.h"

typedef enum {
    PT_NORMAL = 0,
    PT_OBLONG,
    PT_OBTALL,
} PartType;

typedef struct {
  int16_t posA[3];
  int8_t sizeA;
  uint8_t texOffsetA;
  int16_t posB[3];
  int8_t sizeB;
  uint8_t texOffsetB;
  uint8_t colorA[4];
  uint8_t colorB[4];
} __attribute__((packed, aligned(8))) TPXParticleS16;

using namespace std;

#define REJECT_POSITIONS_LARGER_THAN (1000.0f)
#define YSCALE (2)
#define YOFF (0)
#define LARGEST_SCALE (240.0f)
#define PARTICLE_WIDTH (150.0f)

typedef struct {
    int16_t minX; int16_t maxX;
    int16_t minY; int16_t maxY;
    int16_t minZ; int16_t maxZ;
} BoundingBox;

BoundingBox clipPlane = {
    .minX = -50, .maxX = 50,
    .minY = -50, .maxY = 50,
    .minZ = -50, .maxZ = 50,
};

bool isInBox(int16_t *pos) {
    if ((pos[0] < clipPlane.minX) || pos[0] > clipPlane.maxX) {
        return false;
    }
    if ((pos[1] < clipPlane.minY) || pos[1] > clipPlane.maxY) {
        return false;
    }
    if ((pos[2] < clipPlane.minZ) || pos[2] > clipPlane.maxZ) {
        return false;
    }


    return true;
}

uint8_t selectType(float *scale) {
    float avgXY = (scale[0] + scale[1]) / 2;
    float avgXZ = (scale[0] + scale[2]) / 2;
    float avgYZ = (scale[1] + scale[2]) / 2;

    #define SCALE_THRESHOLD (1.1f)

    if ((scale[2] * SCALE_THRESHOLD) > avgXY) {
        return (uint8_t)PT_OBTALL;
    }
    if ((scale[0] * SCALE_THRESHOLD) > avgYZ) {
        return (uint8_t)PT_OBLONG;
    }
    if ((scale[1] * SCALE_THRESHOLD) > avgXZ) {
        return (uint8_t)PT_OBLONG;
    }

    return (uint8_t)PT_NORMAL;
}

float vmag(float a, float b, float c) {
    return sqrtf((a * a) + (b * b) + (c * c));
}

float fscale(float f, float omin, float omax, float nmin, float nmax) {
    return (((f - omin) * (nmax - nmin)) / (omax - omin)) + nmin;
}

int8_t convert_size(float f) {
    return (int8_t)fscale(f, 0, LARGEST_SCALE, 1, PARTICLE_WIDTH);
}

uint8_t convert_color(float f) {
    float newf = (((0.5 + (0.28209479177387814f * f)) * 255.0f));
    if (newf < 0) newf = 0;
    if (newf > 255) newf = 255;

    return (uint8_t)newf;
}

int16_t convert_position(float f) {
    return (int16_t)(f * 1000.0f);
}

void write16BE(FILE *f, uint8_t *data, uint32_t offset) {
    fwrite(&data[offset + 1], 1, 1, f);
    fwrite(&data[offset + 0], 1, 1, f);
}

void write8(FILE *f, uint8_t *data, uint32_t offset) {
    fwrite(&data[offset], 1, 1, f);
}

void writeChunk(FILE *f, void *chunk) {
    uint8_t *chunkdat = (uint8_t *)chunk;
    uint8_t null[] = {0, 0, 0, 0};
    // posA
    write16BE(f, chunkdat, 0);
    write16BE(f, chunkdat, 2);
    write16BE(f, chunkdat, 4);
    // sizeA + type
    write8(f, chunkdat, 6);
    write8(f, chunkdat, 7);
    // posB
    write16BE(f, chunkdat, 8);
    write16BE(f, chunkdat, 10);
    write16BE(f, chunkdat, 12);
    // sizeB + null
    write8(f, chunkdat, 14);
    write8(f, chunkdat, 15);

    // colorA
    write8(f, chunkdat, 16);
    write8(f, chunkdat, 17);
    write8(f, chunkdat, 18);
    write8(f, null, 0);

    // colorB
    write8(f, chunkdat, 20);
    write8(f, chunkdat, 21);
    write8(f, chunkdat, 22);
    write8(f, null, 0);
}

int main(int argc, char **argv) {
    spz::GaussianCloud gc = spz::loadSpz(
        argv[1],
        spz::UnpackOptions(
            // spz::CoordinateSystem::RDF
        )
    );

    char *outputFile = argv[2];
    FILE *f = fopen(outputFile, "wb+");
    if (f == NULL) {
        cerr << "file fail!" << endl;
        return 1;
    }

    int cursor = 0;

    for (int i = 0; i < gc.numPoints; i++) {
        TPXParticleS16 particleChunk;
        auto &pos = gc.positions[i];
        auto &scale = gc.positions[i];
        auto &color = gc.colors[i];

        float posmag = vmag(
            gc.positions[(i * 3) + 0],
            gc.positions[(i * 3) + 1],
            gc.positions[(i * 3) + 2]
        );

        int16_t pos_converted[3] = {
            convert_position(gc.positions[(i * 3) + 0]),
            convert_position(gc.positions[(i * 3) + 1]),
            convert_position(gc.positions[(i * 3) + 2]),
        };

        if (posmag < REJECT_POSITIONS_LARGER_THAN) {
            if ((cursor & 1) == 0) {
                particleChunk.posA[0] = convert_position(gc.positions[(i * 3) + 0]);
                particleChunk.posA[1] = (convert_position(gc.positions[(i * 3) + 1]) / YSCALE) + YOFF;
                particleChunk.posA[2] = convert_position(gc.positions[(i * 3) + 2]);

                particleChunk.sizeA = convert_size(
                    vmag(
                        gc.scales[(i * 3) + 0],
                        gc.scales[(i * 3) + 1],
                        gc.scales[(i * 3) + 2]
                    )
                );

                particleChunk.texOffsetA = selectType(&gc.scales[(i * 3)]) * 32;

                particleChunk.colorA[0] = convert_color(gc.colors[(i * 3) + 0]);
                particleChunk.colorA[1] = convert_color(gc.colors[(i * 3) + 1]);
                particleChunk.colorA[2] = convert_color(gc.colors[(i * 3) + 2]);
                particleChunk.colorA[3] = particleChunk.texOffsetA;
            } else {
                particleChunk.posB[0] = convert_position(gc.positions[(i * 3) + 0]);
                particleChunk.posB[1] = (convert_position(gc.positions[(i * 3) + 1]) / YSCALE) + YOFF;
                particleChunk.posB[2] = convert_position(gc.positions[(i * 3) + 2]);

                particleChunk.sizeB = convert_size(
                    vmag(
                        gc.scales[(i * 3) + 0],
                        gc.scales[(i * 3) + 1],
                        gc.scales[(i * 3) + 2]
                    )
                );

                particleChunk.texOffsetB = selectType(&gc.scales[(i * 3)]) * 32;

                particleChunk.colorB[0] = convert_color(gc.colors[(i * 3) + 0]);
                particleChunk.colorB[1] = convert_color(gc.colors[(i * 3) + 1]);
                particleChunk.colorB[2] = convert_color(gc.colors[(i * 3) + 2]);
                particleChunk.colorB[3] = particleChunk.texOffsetB;

                writeChunk(f, &particleChunk);
            }

            cursor++;
        }
    }
}
