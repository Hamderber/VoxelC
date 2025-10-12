#include <stdint.h>
#include <stdbool.h>
#include <time.h>
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