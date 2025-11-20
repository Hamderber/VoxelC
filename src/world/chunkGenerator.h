#pragma once

#include "core/types/state_t.h"
#include "cmath/weightedMap_t.h"
#include "api/chunk/chunkAPI.h"
#include "world/voxel/block_t.h"

static float gStoneCDF[BLOCK_DEFS_STONE_COUNT];

void chunkGen_stoneNoise_init(WeightMaps_t *pWeightedMaps);

bool chunkGen_genChunk(const WeightMaps_t *pWEIGHTED_MAPS, const BlockDefinition_t *const *restrict pBLOCK_DEFINITIONS,
                       Chunk_t *restrict pChunk);