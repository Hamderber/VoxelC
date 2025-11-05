#pragma once

#include <stdint.h>
#include "cmath/cmath.h"

void randomNoise_init(uint32_t worldSeed);

float randomNoise_stone_samplePackedPos(const Chunk_t *pC, uint16_t blockPosPacked12);

float randomNoise_carving_sampleXYZ(const Chunk_t *pC, uint16_t blockPosPacked12);
