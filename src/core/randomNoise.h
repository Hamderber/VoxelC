#pragma once

#include <stdint.h>
#include "cmath/cmath.h"

void randomNoise_init(uint32_t worldSeed);

float randomNoise_stoneSampleXYZ(const Chunk_t *pC, int lx, int ly, int lz);
