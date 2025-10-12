#pragma once

#include <stdbool.h>
#include <stdint.h>

uint32_t rand_seedGet(void);

bool rand_5050(void);

uint32_t rand_next_u32(void);

int8_t rand_range8bit(uint32_t numBits);

int16_t rand_range16bit(uint32_t numBits);

int32_t rand_range32bit(uint32_t numBits);

int64_t rand_range64bit(uint32_t numBits);

void rand_init(uint32_t seed);