#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cmath.h"

typedef struct
{
    uint32_t count;
    // sum of clamped weights
    float total;
    // length = count; strictly increasing if total>0 ON HEAP
    float *cdf;
} WeightedMap_t;

typedef struct
{
    uint32_t count;
    WeightedMap_t *pWeightMaps;
} WeightMaps_t;

/**
 * Build a CDF from weights[0..count-1].
 * Negative weights are treated as 0.
 * If all weights are zero, falls back to uniform.
 * Requires the map to be on the HEAP
 */
static inline bool weightedMap_bake(WeightedMap_t *pWm, const float *pWEIGHTS, uint32_t count)
{
    if (!pWm || !pWEIGHTS || count == 0)
        return false;

    pWm->count = count;

    // Build running sum (uses double for accumulation stability)
    double run = 0.0;
    for (uint32_t i = 0; i < count; ++i)
    {
        float w = pWEIGHTS[i];
        if (w < 0.0F)
            w = 0.0F;

        run += (double)w;
        // temporarily stores raw prefix sum
        pWm->cdf[i] = (float)run;
    }
    pWm->total = (float)run;

    if (pWm->total <= 0.0F)
    {
        // Fallback to uniform distribution
        for (uint32_t i = 0; i < count; ++i)
        {
            pWm->cdf[i] = (float)(i + 1);
        }
        pWm->total = (float)count;
    }

    return true;
}

static inline uint32_t weightedMap_pick(const WeightedMap_t *pWM, float v)
{
    // Preconditions guard
    if (!pWM || !pWM->cdf || pWM->count == 0)
        return 0;

    // Normalize [-1,1] -> [0,1)
    float t = v * 0.5f + 0.5f;
    if (t < 0.0f)
        t = 0.0f;
    if (t >= 1.0f)
    {
        // Nudge down so target < total (avoids selecting "count")
        t = nextafterf(1.0f, 0.0f); // the largest float < 1.0
    }

    // Scale to [0, total)
    const float target = t * pWM->total;

    // lower_bound: first i with cdf[i] >= target
    uint32_t lo = 0, hi = pWM->count; // search in [lo, hi)
    while (lo < hi)
    {
        uint32_t mid = lo + ((hi - lo) >> 1);
        if (pWM->cdf[mid] >= target)
            hi = mid;
        else
            lo = mid + 1;
    }

    // lo is in [0, count]; since target < total and cdf[count-1]==total, lo < count
    return (lo < pWM->count) ? lo : (pWM->count - 1);
}
