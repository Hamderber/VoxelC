#pragma once

#include "core/types/state_t.h"
#include "weightedMap_t.h"
#include "world/chunkGenerator.h"

typedef enum WeightedMapIndex_e
{
    WEIGHTED_MAP_STONE,
    WEIGHTED_MAP_COUNT,
} WeightedMapIndex_e;

WeightMaps_t *weightedMaps_get(void);

void weightedMaps_destroy(void);

void weightedMaps_instantiate(void);