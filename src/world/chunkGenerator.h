#pragma once

#include "core/types/state_t.h"
#include "cmath/weightedMap_t.h"
#include "api/chunk/chunkAPI.h"
#include "world/voxel/block_t.h"

static float gStoneCDF[BLOCK_DEFS_STONE_COUNT];

void chunkGen_stoneNoise_init(WeightMaps_t *pWeightedMaps);

static const BlockID_e mapNoiseToStone(const WeightMaps_t *pWEIGHTED_MAPS, const float NOISE);