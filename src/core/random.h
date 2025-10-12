#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t rand_stateGet(void);

uint32_t rand_seedGet(void);

uint32_t rand_nextU32t(void);

bool rand_5050(void);

int64_t rand_rangeNbit(int numBits);

static inline int8_t rand_range7bit(int n) { return (int8_t)rand_rangeNbit(n); }
static inline int16_t rand_range15bit(int n) { return (int16_t)rand_rangeNbit(n); }
static inline int32_t rand_range31bit(int n) { return (int32_t)rand_rangeNbit(n); }

void rand_init(uint32_t seed);