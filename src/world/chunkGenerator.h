#pragma once

#include "core/types/state_t.h"
#include "cmath/weightedMap_t.h"
#include "api/chunk/chunkAPI.h"

static float gStoneCDF[BLOCK_DEFS_STONE_COUNT];

void chunkGen_stoneNoise_init(State_t *pState);

bool chunkGen_genChunk(const State_t *pSTATE, const BlockDefinition_t *const *pBLOCK_DEFINITIONS, Chunk_t *pChunk);