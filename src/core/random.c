#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "c_math/c_math.h"

static uint32_t rng_state;
static uint32_t rng_seed;

uint32_t rand_seedGet(void)
{
    return rng_state;
}

uint32_t rand_next_u32(void)
{
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return rng_state = x;
}

int8_t rand_range8bit(uint32_t numBits)
{
    numBits = cm_clampu32t(numBits, 1, 8);
    uint8_t range = (uint8_t)(1U << numBits);
    uint8_t r = (uint8_t)rand_next_u32();
    return (int8_t)((r % range) - (range >> 1));
}

bool rand_5050(void)
{
    return rand_range8bit(1);
}

int16_t rand_range16bit(uint32_t numBits)
{
    numBits = cm_clampu32t(numBits, 1, 16);
    uint16_t range = (uint16_t)(1U << numBits);
    uint16_t r = (uint16_t)rand_next_u32();
    return (int16_t)((r % range) - (range >> 1));
}

int32_t rand_range32bit(uint32_t numBits)
{
    numBits = cm_clampu32t(numBits, 1, 32);
    uint32_t range = (numBits == 32) ? UINT32_MAX : (1U << numBits);
    uint32_t r = rand_next_u32();
    return (int32_t)((r % range) - (range >> 1));
}

int64_t rand_range64bit(uint32_t numBits)
{
    numBits = cm_clampu32t(numBits, 1, 64);
    uint64_t range = (numBits == 64) ? UINT64_MAX : (1ULL << numBits);
    // Mix two xorshift32 values to get 64-bit output
    uint64_t r = ((uint64_t)rand_next_u32() << 32) | rand_next_u32();
    return (int64_t)((r % range) - (range >> 1));
}

void rand_init(uint32_t seed)
{
    if (seed == 0)
    {
        // Use current time for a nondeterministic seed
        seed = (uint32_t)time(NULL);
    }

    rng_state = seed;
    rng_seed = seed;
}