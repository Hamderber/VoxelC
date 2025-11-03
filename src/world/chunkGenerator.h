#pragma once

#include "core/types/state_t.h"
#include "cmath/weightedMap_t.h"

static float gStoneCDF[BLOCK_DEFS_STONE_COUNT];

void chunkGen_stoneNoise_init(State_t *pState);