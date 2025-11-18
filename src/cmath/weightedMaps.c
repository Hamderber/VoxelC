#include <stdlib.h>
#include "core/types/state_t.h"
#include "weightedMaps.h"

static WeightMaps_t *pWeightedMaps = NULL;

void weightedMaps_destroy(void)
{
    if (!pWeightedMaps)
        return;

    if (!pWeightedMaps->pWeightMaps)
        free(pWeightedMaps->pWeightMaps);

    free(pWeightedMaps);
}

static void weightedMaps_bake(void)
{
    pWeightedMaps->pWeightMaps = calloc(WEIGHTED_MAP_COUNT, sizeof(WeightedMap_t));
    chunkGen_stoneNoise_init(pWeightedMaps);
}

void weightedMaps_instantiate(void)
{
    logs_log(LOG_DEBUG, "Baking %d weighted maps...", WEIGHTED_MAP_COUNT);

    pWeightedMaps = malloc(sizeof(WeightMaps_t));
    *pWeightedMaps = (WeightMaps_t){
        .count = WEIGHTED_MAP_COUNT,
        .pWeightMaps = NULL};

    weightedMaps_bake();
}