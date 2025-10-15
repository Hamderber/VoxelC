#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "c_math/c_math.h"

uint32_t rand_stateGet(void);

uint32_t rand_seedGet(void);

uint32_t rand_nextU32t(void);

bool rand_5050(void);

int64_t rand_rangeNbit(int numBits);

static inline int8_t rand_range7bit(int n) { return (int8_t)rand_rangeNbit(n); }
static inline int16_t rand_range15bit(int n) { return (int16_t)rand_rangeNbit(n); }
static inline int32_t rand_range31bit(int n) { return (int32_t)rand_rangeNbit(n); }

uint32_t rand_rangeU32(uint32_t min, uint32_t max);

int32_t rand_rangeI32(int32_t min, int32_t max);

float rand_nextF32(void);

double rand_nextF64(void);

float rand_rangeF32(float min, float max);

double rand_rangeF64(double min, double max);

Vec3f_t rand_vec3f(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);

void rand_init(uint32_t seed);