#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "cmath/cmath.h"
#define _USE_MATH_DEFINES

/// @brief Gets the current PRNG state (changes each query)
/// @return uint32_t
uint32_t random_stateGet(void);

/// @brief Gets the original rng seed
/// @return uint32_t
uint32_t random_seedGet(void);

/// @brief Get a new pseudo-random 32-bit unsigned integer
/// @return uint32_t
uint32_t random_nextU32t(void);

/**
 * @brief Get a new pseudo-random n-bit signed integer means-corrected to zero in the range [-2^(n-1)-1, 2^(n-1)+1] =>
 * [-1073741823, 1073741823]
 * @param numBits Clamped to [1, 31] (last bit reserved for sign)
 * @return int32_t
 */
int32_t random_rangeNbit(int numBits);

/// @brief Get a pesudo-random unsigned 32-bit integer in range [min, max]
/// @return uint32_t
uint32_t random_rangeU32(uint32_t min, uint32_t max);

/// @brief Returns a random signed 32-bit integer in [min, max]
/// @return int32_t
int32_t random_rangeI32(int32_t min, int32_t max);

/// @brief Returns a bit-uniform pseudo-random float in [0, 1)
/// @return float
float random_nextF32(void);

/// @brief Returns a random float in [min, max)
/// @return float
float random_rangeF32(float min, float max);

/// @brief Returns a uniform pseudo-random double in [0, 1)
/// @return double
double random_nextD64(void);

/// @brief Returns a random double in [min, max)
/// @return double
double random_rangeD64(double min, double max);

/// @brief Flip a coin.
/// @return bool
static inline bool random_5050(void) { return random_rangeNbit(1); }

/// @brief Wrapper for @ref random_rangeNbit for the correct int size
/// @return int8_t
static inline int8_t random_range7bit(int n) { return (int8_t)random_rangeNbit(n); }

/// @brief Wrapper for @ref random_rangeNbit for the correct int size
/// @return int16_t
static inline int16_t random_range15bit(int n) { return (int16_t)random_rangeNbit(n); }

/// @brief Generates a pseudo-random vec3f with axes within [min, max)
/// @return Vec3f_t
static inline Vec3f_t random_vec3f(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax)
{
    return (Vec3f_t){
        .x = random_rangeF32(xMin, xMax),
        .y = random_rangeF32(yMin, yMax),
        .z = random_rangeF32(zMin, zMax),
    };
}

/// @brief Generates a random rotation quaternion
static inline Quaternionf_t random_quat(void)
{
    float u1 = random_rangeF32(0.0F, 1.0F);
    float u2 = random_rangeF32(0.0F, 1.0F);
    float u3 = random_rangeF32(0.0F, 1.0F);

    float r1 = sqrtf(1.0F - u1);
    float r2 = sqrtf(u1);
    float t1 = PI_F * 2.0F * u2;
    float t2 = PI_F * 2.0F * u3;

    Quaternionf_t q = {
        .qx = sinf(t1) * r1,
        .qy = cosf(t1) * r1,
        .qz = sinf(t2) * r2,
        .qw = cosf(t2) * r2};

    return cmath_quat_normalize(q);
}

/// @brief Generates a random column-major rotation matrix
static inline Mat4c_t random_mat_rot(void) { return cmath_quat2mat(random_quat()); }

/// @brief Initializes deterministic random using the provided seed or current time if seed = 0
/// @param seed
void random_init(uint32_t seed);