#pragma once

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include "c_math/c_math.h"

// Directions
static const Vec3f_t RIGHT = {1.0f, 0.0f, 0.0f};
static const Vec3f_t LEFT = {-1.0f, 0.0f, 0.0f};
static const Vec3f_t UP = {0.0f, 1.0f, 0.0f};
static const Vec3f_t DOWN = {0.0f, -1.0f, 0.0f};
// Vulkan/GLM/OpenGL convention is -z is "looking forward"
static const Vec3f_t FORWARD = {0.0f, 0.0f, -1.0f};
static const Vec3f_t BACK = {0.0f, 0.0f, 1.0f};

// Colors
static const Vec3f_t RED = {1.0f, 0.0f, 0.0f};
static const Vec3f_t GREEN = {0.0f, 1.0f, 0.0f};
static const Vec3f_t BLUE = {0.0f, 0.0f, 1.0f};
static const Vec3f_t BLACK = {0.0f, 0.0f, 0.0f};
static const Vec3f_t WHITE = {1.0f, 1.0f, 1.0f};
static const Vec3f_t YELLOW = {1.0f, 1.0f, 0.0f};
static const Vec3f_t CYAN = {0.0f, 1.0f, 1.0f};
static const Vec3f_t MAGENTA = {1.0f, 0.0f, 1.0f};
static const Vec3f_t GRAY = {0.5f, 0.5f, 0.5f};

// Axes
static const Vec3f_t X_AXIS = {1.0f, 0.0f, 0.0f};
static const Vec3f_t Y_AXIS = {0.0f, 1.0f, 0.0f};
static const Vec3f_t Z_AXIS = {0.0f, 0.0f, 1.0f};

// Diagonals
static const Vec3f_t VEC3_ONE = {1.0f, 1.0f, 1.0f};
static const Vec3f_t VEC3_NEG_ONE = {-1.0f, -1.0f, -1.0f};
static const Vec3f_t VEC3_ZERO = {0.0f, 0.0f, 0.0f};

static inline Vec3f_t cm_vec3fMultScalar(Vec3f_t vec3, float scalar)
{
    return (Vec3f_t){
        .x = vec3.x * scalar,
        .y = vec3.y * scalar,
        .z = vec3.z * scalar,
    };
}

static inline Vec3f_t cm_vec3fSum(Vec3f_t left, Vec3f_t right)
{
    return (Vec3f_t){
        .x = left.x + right.x,
        .y = left.y + right.y,
        .z = left.z + right.z,
    };
}

static inline float cm_vec3fMagnitude(Vec3f_t vec3)
{
    return sqrtf(vec3.x * vec3.x + vec3.y * vec3.y + vec3.z * vec3.z);
}

static inline Vec3f_t cm_vec3fNormalize(Vec3f_t vec3)
{
    float length = cm_vec3fMagnitude(vec3);

    if (length < FLT_EPSILON)
        return VEC3_ZERO;

    return (Vec3f_t){
        .x = vec3.x / length,
        .y = vec3.y / length,
        .z = vec3.z / length,
    };
}

static inline bool cm_vec3fIsZero(Vec3f_t vec3)
{
    return fabs(vec3.x) < FLT_EPSILON && fabs(vec3.y) < FLT_EPSILON && fabs(vec3.z) < FLT_EPSILON;
}
