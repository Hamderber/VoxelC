#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <float.h>
#include "c_math/c_math.h"

static uint32_t state;
static uint32_t rand_seed;

/// @brief Gets the current rng state value (changes each query)
/// @param void
/// @return uint32_t
uint32_t rand_stateGet(void)
{
    return state;
}

/// @brief Gets the original rng seed
/// @param  void
/// @return uint32_t
uint32_t rand_seedGet(void)
{
    return rand_seed;
}

uint32_t rand_nextU32t(void)
{
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return state = x;
}

int64_t rand_rangeNbit(int numBits)
{
    numBits = cm_clampu32t(numBits, 1, 31);

    // Fix always getting 0 because of the shifting correction from larger numbers
    if (numBits == 1)
    {
        return (rand_nextU32t() & 1u) ? 1 : -1;
    }

    // Build an unsigned integer with exactly numBits of entropy
    uint64_t value = 0;
    int bitsRemaining = numBits;
    while (bitsRemaining > 0)
    {
        // Take chunks from rand_nextU32t() 32 bits at a time
        uint32_t chunk = rand_nextU32t();
        int take = bitsRemaining > 32 ? 32 : bitsRemaining;
        value = (value << take) | (chunk & ((1u << take) - 1u));
        bitsRemaining -= take;
    }

    // Re-center around 0, mapping the extra negative bin to 0 for symmetry (slightly less variance)
    int64_t signedVal = (int64_t)(value) - ((int64_t)1 << (numBits - 1));
    if (signedVal == -(int64_t)(1 << (numBits - 1)))
        signedVal = 0;
    return signedVal;
}

bool rand_5050(void)
{
    return rand_rangeNbit(1);
}

/// @brief Returns a random unsigned 32-bit integer in [min, max]
/// @note If min > max, they will be swapped.
/// @param min inclusive lower bound
/// @param max inclusive upper bound
/// @return uint32_t random integer
uint32_t rand_rangeU32(uint32_t min, uint32_t max)
{
    if (min > max)
    {
        uint32_t tmp = min;
        min = max;
        max = tmp;
    }

    uint32_t range = max - min + 1u;

    // Avoid modulo bias by rejecting numbers outside the largest multiple of range
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);
    uint32_t r;
    do
    {
        r = rand_nextU32t();
    } while (r >= limit);

    return min + (r % range);
}

/// @brief Returns a random signed 32-bit integer in [min, max]
/// @note Handles negative ranges and min > max gracefully
/// @param min inclusive lower bound
/// @param max inclusive upper bound
/// @return int32_t random integer
int32_t rand_rangeI32(int32_t min, int32_t max)
{
    if (min > max)
    {
        int32_t tmp = min;
        min = max;
        max = tmp;
    }

    // Cast to uint32_t and use uniform unsigned method to avoid sign bias
    uint32_t range = (uint32_t)((int64_t)max - (int64_t)min + 1);
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);
    uint32_t r;
    do
    {
        r = rand_nextU32t();
    } while (r >= limit);

    return min + (int32_t)(r % range);
}

/// @brief Returns a random float in [0, 1)
/// @note Based on 32 bits of entropy mapped to float precision
/// @return float uniform random
float rand_nextF32(void)
{
    // Convert 24 bits of entropy (enough for float mantissa)
    return (rand_nextU32t() >> 8) * (1.0f / 16777216.0F); // 1/2^24
}

/// @brief Returns a random double in [0, 1)
/// @note Provides full 53-bit mantissa precision
/// @return double uniform random
double rand_nextF64(void)
{
    // Combine 53 bits of randomness correctly
    uint64_t upper = (uint64_t)(rand_nextU32t() & 0x001FFFFFu);   // 21 bits
    uint64_t lower = (uint64_t)(rand_nextU32t() & 0x003FFFFFFFu); // 32 bits
    uint64_t combined = (upper << 32) | lower;                    // total 53 bits

    return (double)combined * (1.0 / 9007199254740992.0); // 1 / 2^53
}

/// @brief Returns a random float in [min, max)
/// @param min lower inclusive
/// @param max upper exclusive
/// @return float
float rand_rangeF32(float min, float max)
{
    if (fabsf(max - min) < FLT_EPSILON)
        return min;

    if (min > max)
    {
        float tmp = min;
        min = max;
        max = tmp;
    }

    return min + (max - min) * rand_nextF32();
}

/// @brief Returns a random double in [min, max)
/// @param min lower inclusive
/// @param max upper exclusive
/// @return double
double rand_rangeF64(double min, double max)
{
    if (min > max)
    {
        double tmp = min;
        min = max;
        max = tmp;
    }

    return min + (max - min) * rand_nextF64();
}

/// @brief Generates a random vec3f with axes within the provided bounds [min, max)
/// @param xMin
/// @param xMax
/// @param yMin
/// @param yMax
/// @param zMin
/// @param zMax
/// @return Vec3f_t
Vec3f_t rand_vec3f(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    return (Vec3f_t){
        .x = rand_rangeF32(xMin, xMax),
        .y = rand_rangeF32(yMin, yMax),
        .z = rand_rangeF32(zMin, zMax),
    };
}

/// @brief Initializes deterministic random using the provided seed or current time if seed = 0
/// @param seed
void rand_init(uint32_t seed)
{
    if (seed == 0)
    {
        // Use current time for a nondeterministic seed
        seed = (uint32_t)time(NULL);
    }

    state = seed;
    rand_seed = seed;
}