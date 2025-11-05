#pragma once

#include "core/types/state_t.h"
#include "weightedMap_t.h"
#include "world/chunkGenerator.h"

typedef enum
{
    WEIGHTED_MAP_STONE,
    WEIGHTED_MAP_COUNT,
} WeightedMapIndex_t;

static void weightedMaps_init(State_t *pState)
{
    pState->weightedMaps.pWeightMaps = calloc(WEIGHTED_MAP_COUNT, sizeof(WeightedMap_t));
    chunkGen_stoneNoise_init(pState);
}

static void weightedMaps_destroy(State_t *pState)
{
    // Don't free the weighted maps' CDFs because those are HEAP
    free(pState->weightedMaps.pWeightMaps);
    pState->weightedMaps.pWeightMaps = NULL;
}