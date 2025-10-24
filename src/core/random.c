#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <float.h>
#include "cmath/cmath.h"

// Current PRNG state
static uint32_t s_state;
static uint32_t s_seed;

uint32_t random_stateGet(void) { return s_state; }

uint32_t random_seedGet(void) { return s_seed; }

uint32_t random_nextU32t(void)
{
    uint32_t x = s_state;

    // Bitwise linear feedback Xorshift32 by George Marsaglia (2003)
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    return s_state = x;
}

int32_t random_rangeNbit(int numBits)
{
    numBits = cmath_clampU32t(numBits, 1, 31);

    // Because of the mean re-centering below, numBits = 1 always returns 0. Correct that by just adding a bit to the call
    // and assigning it to -1 to maintain the distribution.
    if (numBits == 1)
        return (random_nextU32t() & 1u) ? 1 : -1;

    uint64_t value = 0;
    int bitsRemaining = numBits;
    int take;
    while (bitsRemaining > 0)
    {
        // Build an unsigned integer with exactly numBits of entropy by taking chunks from random uint32s
        uint32_t chunk = random_nextU32t();
        take = bitsRemaining > 32 ? 32 : bitsRemaining;
        value = (value << take) | (chunk & ((1U << take) - 1U));
        bitsRemaining -= take;
    }

    // Re-center the mean to 0 by mapping the extra negative bit to 0 for symmetry (causes slightly less variance)
    int64_t signedVal = (int64_t)(value) - ((int64_t)1 << (numBits - 1));
    if (signedVal == -(int64_t)(1 << (numBits - 1)))
        signedVal = 0;

    return (int32_t)signedVal;
}

uint32_t random_rangeU32(uint32_t min, uint32_t max)
{
    if (min > max)
    {
        uint32_t tmp = min;
        min = max;
        max = tmp;
    }

    // Due to the modulo, without adding 1 the range would be [min, max - 1]
    uint32_t range = max - min + 1u;

    // Avoid modulo bias by rejecting numbers outside the largest multiple of range
    uint32_t limit = UINT32_MAX - (UINT32_MAX % range);
    uint32_t r;
    do
    {
        r = random_nextU32t();
    } while (r >= limit);

    return min + (r % range);
}

int32_t random_rangeI32(int32_t min, int32_t max)
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
        r = random_nextU32t();
    } while (r >= limit);

    return min + (int32_t)(r % range);
}

float random_nextF32(void)
{
    // Convert 24 bits of entropy (enough for float mantissa) 1 / 2^24
    return (random_nextU32t() >> 8) * (1.0f / 16777216.0F);
}

double random_nextD64(void)
{
    // Combine 53 bits of randomness correctly (mantissa)
    uint64_t upper = (uint64_t)(random_nextU32t() & 0x001FFFFFu);   // 21 bits
    uint64_t lower = (uint64_t)(random_nextU32t() & 0x003FFFFFFFu); // 32 bits
    uint64_t combined = (upper << 32) | lower;                      // total 53 bits

    // 1 / 2^53
    return (double)combined * (1.0 / 9007199254740992.0);
}

float random_rangeF32(float min, float max)
{
    if (fabsf(max - min) < FLT_EPSILON)
        return min;

    if (min > max)
    {
        float tmp = min;
        min = max;
        max = tmp;
    }

    return min + (max - min) * random_nextF32();
}

double random_rangeD64(double min, double max)
{
    if (min > max)
    {
        double tmp = min;
        min = max;
        max = tmp;
    }

    return min + (max - min) * random_nextD64();
}

void random_init(uint32_t seed)
{
    if (seed == 0U)
    {
        // Use current time for a nondeterministic seed
        seed = (uint32_t)time(NULL);
    }

    s_state = seed;
    s_seed = seed;
}
