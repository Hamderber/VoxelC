#include "core/logs.h"
#include "cmath/weightedMap_t.h"
#include "world/voxel/block_t.h"
#include "chunkGenerator.h"
#include "core/types/state_t.h"
#include "cmath/weightedMaps.h"

void chunkGen_stoneNoise_init(State_t *pState)
{
    // TODO:
    // Add stone pallet selection and a better mapping selection _t for associating what adjacencies and rarities
    // Add a very simple perlin worm carver
    // change gen order to be solid/not-solid carver and then painting only nonair
    float pStoneWeights[BLOCK_DEFS_STONE_COUNT] = {
        5.50F,
        4.50F,
        1.50F,
        1.50F,
        0.50F,
        0.75F,
        0.50F,
        0.25F,
        0.50F,
        0.75F,
        0.50F,
        1.50F,
        4.50F,
        5.50F,
    };

    WeightedMap_t *pWeightedMap = &pState->weightedMaps.pWeightMaps[WEIGHTED_MAP_STONE];
    pWeightedMap->cdf = gStoneCDF;
    if (!weightedMap_bake(pWeightedMap, pStoneWeights, BLOCK_DEFS_STONE_COUNT))
    {
        logs_log(LOG_ERROR, "Failed to bake stone weight map!");
        return;
    }

    logs_log(LOG_DEBUG, "Stone weight map baked (total weight = %.2f)", pWeightedMap->total);
    logs_log(LOG_DEBUG, "Stone CDF: count=%u total=%.3f first=%.3f last=%.3f",
             pWeightedMap->count, pWeightedMap->total,
             pWeightedMap->cdf[0], pWeightedMap->cdf[pWeightedMap->count - 1]);
}