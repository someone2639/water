#include <iostream>
#include <vector>
#include <ranges> // Required for std::views::zip
#include "load-spz.h"

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

float vmag(float a, float b, float c) {
    return sqrtf((a * a) + (b * b) + (c * c));
}

uint8_t convert_color(float f) {
    return (uint8_t)((0.5 + 0.282095 * f) * 256.0f);
}

int16_t convert_float(float f) {
    return (int16_t)f;
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
    // sizeA + null
    write8(f, chunkdat, 6);
    write8(f, null, 0);
    // posB
    write16BE(f, chunkdat, 8);
    write16BE(f, chunkdat, 10);
    write16BE(f, chunkdat, 12);
    // sizeB + null
    write8(f, chunkdat, 14);
    write8(f, null, 0);

    // colorA
    write8(f, chunkdat, 16);
    write8(f, chunkdat, 17);
    write8(f, chunkdat, 18);
    write8(f, null, 0);

    // colorB
    write8(f, chunkdat, 19);
    write8(f, chunkdat, 20);
    write8(f, chunkdat, 21);
    write8(f, null, 0);
}

int main(int argc, char **argv) {
    spz::GaussianCloud gc = spz::loadSpz(argv[1], spz::UnpackOptions());

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

        if (cursor & 1) {
            particleChunk.posA[0] = convert_float(gc.positions[(i * 3) + 0]);
            particleChunk.posA[1] = convert_float(gc.positions[(i * 3) + 1]);
            particleChunk.posA[2] = convert_float(gc.positions[(i * 3) + 2]);

            particleChunk.sizeA = vmag(
                gc.scales[(i * 3) + 0],
                gc.scales[(i * 3) + 1],
                gc.scales[(i * 3) + 2]
            );

            particleChunk.colorA[0] = convert_color(gc.colors[(i * 3) + 0]);
            particleChunk.colorA[1] = convert_color(gc.colors[(i * 3) + 1]);
            particleChunk.colorA[2] = convert_color(gc.colors[(i * 3) + 2]);
        } else {
            particleChunk.posB[0] = convert_float(gc.positions[(i * 3) + 0]);
            particleChunk.posB[1] = convert_float(gc.positions[(i * 3) + 1]);
            particleChunk.posB[2] = convert_float(gc.positions[(i * 3) + 2]);

            particleChunk.sizeB = vmag(
                gc.scales[(i * 3) + 0],
                gc.scales[(i * 3) + 1],
                gc.scales[(i * 3) + 2]
            );

            particleChunk.colorB[0] = convert_color(gc.colors[(i * 3) + 0]);
            particleChunk.colorB[1] = convert_color(gc.colors[(i * 3) + 1]);
            particleChunk.colorB[2] = convert_color(gc.colors[(i * 3) + 2]);

            writeChunk(f, &particleChunk);
        }
    }
}
