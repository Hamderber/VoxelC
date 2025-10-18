#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    float qx, qy, qz, qw;
} Quaternion_t;

typedef struct
{
    float x, y;
} Vec2f_t;

typedef struct
{
    float x, y, z;
} Vec3f_t;

// A w of 0 means rotation and 1 means position and non-1 means rotation.
typedef struct
{
    float x, y, z, w;
} Vec4f_t;

// Matricies
// https://www.c-jump.com/bcc/common/Talk3/Math/GLM/GLM.html
// 4x4 Matrix column-major (array index is for each column)
// Struct members are the same as the shader codes'
typedef struct
{
    Vec4f_t m[4];
} Mat4c_t;

/// @brief Clamps f between min and max
/// @param f
/// @param min
/// @param max
/// @return min <= f <= max
static inline float cm_clampf(float f, float min, float max)
{
    if (f < min)
        return min;
    else if (f > max)
        return max;
    else
        return f;
}

/// @brief Clamps d between min and max
/// @param d
/// @param min
/// @param max
/// @return min <= d <= max
static inline double cm_clampd(double d, double min, double max)
{
    if (d < min)
        return min;
    else if (d > max)
        return max;
    else
        return d;
}

/// @brief Clamps u between min and max
/// @param u
/// @param min
/// @param max
/// @return min <= u <= max
static inline uint32_t cm_clampu32t(uint32_t u, uint32_t min, uint32_t max)
{
    if (u < min)
    {
        return min;
    }
    else if (u > max)
    {
        return max;
    }
    else
    {
        return u;
    }
}

// Must include before the actual headers so that they can reference eachother properly (avoid circular includes)

#include "c_math/units.h"
#include "c_math/vector.h"
#include "c_math/quaternion.h"
#include "c_math/matrix.h"