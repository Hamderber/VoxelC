#pragma once

#pragma region Includes
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#pragma endregion

#pragma region Defines
/// @brief PI definition (double)
#define PI_D 3.1415926535897931
/// @brief PI definition (float)
#define PI_F 3.1415927F
/// @brief Runtime constant for converting radians to degrees
#define CMATH_RAD2DEG_F (180.0F / PI_F)
/// @brief Runtime constant for converting degrees to radians
#define CMATH_DEG2RAD_F (PI_F / 180.0F)
/// @brief Runtime constant for converting radians to degrees
#define CMATH_RAD2DEG_D (180.0 / PI_D)
/// @brief Runtime constant for converting degrees to radians
#define CMATH_DEG2RAD_D (PI_D / 180.0)
/// @brief The smallest float difference to consider values equal
#define CMATH_EPSILON_F 1e-5F
/// @brief The smallest double difference to consider values equal
#define CMATH_EPSILON_D 1e-10
/// @brief The threshold to consider a dot product result equal to 1 (being the same)
#define CMATH_DOT_THRESHOLD_F 0.9995F
/// @brief Maximum float value
#define CMATH_MAX_F 3.402823466e+38F
/// @brief Maximum double value
#define CMATH_MAX_D 1.7976931348623158e+308
#pragma endregion

#pragma region Types
/// @brief 4-dimensional representation of rotation used in computer graphics etc. (float)
typedef struct
{
    float qx, qy, qz, qw;
} Quaternionf_t;

/// @brief 2-dimensional value representation (float)
typedef struct
{
    float x, y;
} Vec2f_t;

/// @brief 3-dimensional value representation (float)
typedef struct
{
    float x, y, z;
} Vec3f_t;

/// @brief 4-dimensional value representation (float). A w of 1 means position and non-1 means rotation.
typedef struct
{
    float x, y, z, w;
} Vec4f_t;

/// @brief 4x4 Matrix column-major (array index is for each column). Struct members are the same as the shader codes'.
typedef struct
{
    Vec4f_t m[4];
} Mat4c_t;
#pragma endregion

#pragma region Type Constants
/// @brief The Identity Quaternion representing no rotation and no shear
static const Quaternionf_t QUATERNION_IDENTITY = {0.0F, 0.0F, 0.0F, 1.0F};

/// @brief Column-major
static const Mat4c_t MAT4_IDENTITY = {
    .m = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
        {0.0F, 0.0F, 0.0F, 1.0F},
    },
};

static const Vec3f_t VEC3_ONE = {1.0F, 1.0F, 1.0F};
static const Vec3f_t VEC3_NEG_ONE = {-1.0F, -1.0F, -1.0F};
static const Vec3f_t VEC3_ZERO = {0.0F, 0.0F, 0.0F};

#pragma region Directions
static const Vec3f_t VEC3_RIGHT = {1.0F, 0.0F, 0.0F};
static const Vec3f_t VEC3_LEFT = {-1.0F, 0.0F, 0.0F};
static const Vec3f_t VEC3_UP = {0.0F, 1.0F, 0.0F};
static const Vec3f_t VEC3_DOWN = {0.0F, -1.0F, 0.0F};
/// @brief Vulkan/GLM/OpenGL convention is -z is "looking forward"
static const Vec3f_t VEC3_FORWARD = {0.0F, 0.0F, -1.0F};
static const Vec3f_t VEC3_BACK = {0.0F, 0.0F, 1.0F};
#pragma endregion

#pragma region Colors
static const Vec3f_t COLOR_RED = {1.0F, 0.0F, 0.0F};
static const Vec3f_t COLOR_GREEN = {0.0F, 1.0F, 0.0F};
static const Vec3f_t COLOR_BLUE = {0.0F, 0.0F, 1.0F};
static const Vec3f_t COLOR_BLACK = {0.0F, 0.0F, 0.0F};
static const Vec3f_t COLOR_WHITE = {1.0F, 1.0F, 1.0F};
static const Vec3f_t COLOR_YELLOW = {1.0F, 1.0F, 0.0F};
static const Vec3f_t COLOR_CYAN = {0.0F, 1.0F, 1.0F};
static const Vec3f_t COLOR_MAGENTA = {1.0F, 0.0F, 1.0F};
static const Vec3f_t COLOR_GRAY = {0.5F, 0.5F, 0.5F};
#pragma endregion

#pragma region Axes
static const Vec3f_t VEC3_X_AXIS = {1.0F, 0.0F, 0.0F};
static const Vec3f_t VEC3_Y_AXIS = {0.0F, 1.0F, 0.0F};
static const Vec3f_t VEC3_Z_AXIS = {0.0F, 0.0F, 1.0F};
#pragma endregion
#pragma endregion

#pragma region Fundamentals
/// @brief Clamps f between [min, max]
static inline float cmath_clampF(float f, float min, float max)
{
    if (f < min)
        return min;
    else if (f > max)
        return max;
    else
        return f;
}

/// @brief Clamps d between [min, max]
static inline double cmath_clampD(double d, double min, double max)
{
    if (d < min)
        return min;
    else if (d > max)
        return max;
    else
        return d;
}

/// @brief Clamps u between [min, max]
static inline uint32_t cmath_clampU32t(uint32_t u, uint32_t min, uint32_t max)
{
    if (u < min)
        return min;
    else if (u > max)
        return max;
    else
        return u;
}

#pragma endregion

#pragma region Unit Conversion
/// @brief Converts radians to degrees
static inline float cmath_rad2degF(float rad)
{
    return rad * CMATH_RAD2DEG_F;
}

/// @brief Converts radians to degrees
static inline double cmath_rad2degD(double rad)
{
    return rad * CMATH_RAD2DEG_D;
}

/// @brief Converts degrees to radians
static inline float cmath_deg2radF(float deg)
{
    return deg * CMATH_DEG2RAD_F;
}

/// @brief Converts degrees to radians
static inline double cmath_deg2radD(double deg)
{
    return deg * CMATH_DEG2RAD_D;
}
#pragma endregion

#pragma region Vector Math
/// @brief True if all axes of the passed vector are 0.0F
static inline bool cmath_vec3f_isZero(Vec3f_t vec3)
{
    return fabsf(vec3.x) < CMATH_EPSILON_F && fabsf(vec3.y) < CMATH_EPSILON_F && fabsf(vec3.z) < CMATH_EPSILON_F;
}

/// @brief Multiplies each axis of the vector by the given scalar (float)
static inline Vec3f_t cmath_vec3f_mult_scalarF(Vec3f_t vec3, float scalar)
{
    return (Vec3f_t){
        .x = vec3.x * scalar,
        .y = vec3.y * scalar,
        .z = vec3.z * scalar,
    };
}

/// @brief Adds the axes of the left and right vector into one new vector
static inline Vec3f_t cmath_vec3f_add_vec3f(Vec3f_t left, Vec3f_t right)
{
    return (Vec3f_t){
        .x = left.x + right.x,
        .y = left.y + right.y,
        .z = left.z + right.z,
    };
}

/// @brief Calculates the magnitude (length) of the given vector (float)
static inline float cmath_vec3f_magnitudeF(Vec3f_t vec3)
{
    return sqrtf(vec3.x * vec3.x + vec3.y * vec3.y + vec3.z * vec3.z);
}

/// @brief Proportionally scales the axes of the vector to have a magnitude of 1 while preserving the vector direction
static inline Vec3f_t cmath_vec3f_normalize(Vec3f_t vec3)
{
    float length = cmath_vec3f_magnitudeF(vec3);

    // Avoid divide by zero
    if (length < CMATH_EPSILON_F)
        return VEC3_ZERO;

    return cmath_vec3f_mult_scalarF(vec3, 1.0F / length);
}

/// @brief Linearly interpolates between two vectors by t->E[0, 1] (float)
static inline Vec3f_t cmath_vec3f_lerpF(Vec3f_t a, Vec3f_t b, float t)
{
    t = cmath_clampF(t, 0.0F, 1.0F);

    return (Vec3f_t){
        .x = a.x + (b.x - a.x) * t,
        .y = a.y + (b.y - a.y) * t,
        .z = a.z + (b.z - a.z) * t,
    };
}
#pragma endregion

#pragma region Quaternion Math
/// @brief Checks if the quaternion is (0, 0, 0, +/-1) (identity)
static inline bool cmath_quat_isIdentity(Quaternionf_t q)
{
    // qw = -1 also is identity so check abs of w
    return fabsf(q.qx) < CMATH_EPSILON_F &&
           fabsf(q.qy) < CMATH_EPSILON_F &&
           fabsf(q.qz) < CMATH_EPSILON_F &&
           fabsf(1.0F - fabsf(q.qw)) < CMATH_EPSILON_F;
}

/// @brief Calculates the magnitude (length) of the given quaternion (float)
static inline float cmath_quat_magnitudeF(Quaternionf_t q)
{
    return sqrtf(q.qx * q.qx + q.qy * q.qy + q.qz * q.qz + q.qw * q.qw);
}

/// @brief Multiplies the axes of the quaternion by the given scalar (float)
static inline Quaternionf_t cmath_quat_mult_scalarF(Quaternionf_t q, float scalar)
{
    return (Quaternionf_t){
        .qx = q.qx * scalar,
        .qy = q.qy * scalar,
        .qz = q.qz * scalar,
        .qw = q.qw * scalar,
    };
}

/// @brief Scales the quaternion to have a normalized magnitude of 1 (necessary for rotation representations)
static inline Quaternionf_t cmath_quat_normalize(Quaternionf_t q)
{
    float mag = cmath_quat_magnitudeF(q);

    // Avoid divide by zero
    if (mag != 0.0F)
    {
        // Inverse of square root of sum of squaring each quaternion component. (Inverse of Vector4 magnitude).
        return cmath_quat_mult_scalarF(q, 1.0F / mag);
    }
    else
    {
        return QUATERNION_IDENTITY;
    }
}

/// @brief Multiply two quaternions (rotation composition). Order matters: q = q1 * q2 means "apply q2, then q1"
static inline Quaternionf_t cmath_quat_mult_quat(Quaternionf_t a, Quaternionf_t b)
{
    return (Quaternionf_t){
        .qx = a.qw * b.qx + a.qx * b.qw + a.qy * b.qz - a.qz * b.qy,
        .qy = a.qw * b.qy - a.qx * b.qz + a.qy * b.qw + a.qz * b.qx,
        .qz = a.qw * b.qz + a.qx * b.qy - a.qy * b.qx + a.qz * b.qw,
        .qw = a.qw * b.qw - a.qx * b.qx - a.qy * b.qy - a.qz * b.qz,
    };
}

/// @brief Rotate one quaternion toward another (spherical linear interpolation) by t->E[0, 1] (float)
static inline Quaternionf_t cmath_quat_slerp(Quaternionf_t a, Quaternionf_t b, float t)
{
    t = cmath_clampF(t, 0.0F, 1.0F);

    // Cosine of the angle between the two quaternions
    float dot = a.qx * b.qx + a.qy * b.qy + a.qz * b.qz + a.qw * b.qw;

    // If dot < 0, the quaternions are opposite so negate one to take the shortest path
    if (dot < 0.0F)
    {
        b.qx = -b.qx;
        b.qy = -b.qy;
        b.qz = -b.qz;
        b.qw = -b.qw;
        dot = -dot;
    }

    if (dot > CMATH_DOT_THRESHOLD_F)
    {
        // Quaternions are the same so use linear interpolation
        Quaternionf_t result = {
            .qx = a.qx + t * (b.qx - a.qx),
            .qy = a.qy + t * (b.qy - a.qy),
            .qz = a.qz + t * (b.qz - a.qz),
            .qw = a.qw + t * (b.qw - a.qw),
        };
        return cmath_quat_normalize(result);
    }

    // Compute the angle between them
    float theta_0 = acosf(dot);
    float theta = theta_0 * t;

    float sin_theta_0 = sinf(theta_0);
    float sin_theta = sinf(theta);

    float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    Quaternionf_t result = {
        .qx = (a.qx * s0) + (b.qx * s1),
        .qy = (a.qy * s0) + (b.qy * s1),
        .qz = (a.qz * s0) + (b.qz * s1),
        .qw = (a.qw * s0) + (b.qw * s1),
    };

    return cmath_quat_normalize(result);
}

/// @brief Create a new quaternion representing euler rotation about the given axis (radians)
static inline Quaternionf_t cmath_quat_fromAxisAngle(float radians, Vec3f_t axis)
{
    float len = cmath_vec3f_magnitudeF(axis);

    // Avoid divide by zero, fallback to identity
    if (len < CMATH_EPSILON_F)
    {
        return QUATERNION_IDENTITY;
    }

    float s = sinf(radians * 0.5F);
    float c = cosf(radians * 0.5F);

    Vec3f_t n = cmath_vec3f_mult_scalarF(axis, s / len);

    return (Quaternionf_t){
        .qx = n.x,
        .qy = n.y,
        .qz = n.z,
        .qw = c,
    };
}

/// @brief Convert euler (axis-angle) to quaternion in yaw y/pitch x/roll z order (radians)
static inline Quaternionf_t cmath_quat_fromEuler(Vec3f_t euler)
{
    // half-angles
    Vec3f_t half = cmath_vec3f_mult_scalarF(euler, 0.5F);

    // sin/cos
    float sx = sinf(half.x), cx = cosf(half.x);
    float sy = sinf(half.y), cy = cosf(half.y);
    float sz = sinf(half.z), cz = cosf(half.z);

    // Order: Yaw (Y), Pitch (X), Roll (Z)
    Quaternionf_t q = {
        .qx = sx * cy * cz + cx * sy * sz,
        .qy = cx * sy * cz - sx * cy * sz,
        .qz = cx * cy * sz + sx * sy * cz,
        .qw = cx * cy * cz - sx * sy * sz};

    return cmath_quat_normalize(q);
}

/// @brief Convert quaternion to euler in yaw y/pitch x/roll z order (radians)
static inline Vec3f_t cmath_quat_toEulerAngles(Quaternionf_t q)
{
    q = cmath_quat_normalize(q);
    Vec3f_t angles;

    // Pitch (X)
    float sinr_cosp = 2.0F * (q.qw * q.qx + q.qy * q.qz);
    float cosr_cosp = 1.0F - 2.0F * (q.qx * q.qx + q.qy * q.qy);
    angles.x = atan2f(sinr_cosp, cosr_cosp);

    // Yaw (Y)
    float sinp = 2.0F * (q.qw * q.qy - q.qz * q.qx);
    if (fabsf(sinp) >= 1.0F)
        angles.y = copysignf(PI_F / 2.0F, sinp);
    else
        angles.y = asinf(sinp);

    // Roll (Z)
    float siny_cosp = 2.0F * (q.qw * q.qz + q.qx * q.qy);
    float cosy_cosp = 1.0F - 2.0F * (q.qy * q.qy + q.qz * q.qz);
    angles.z = atan2f(siny_cosp, cosy_cosp);

    return angles;
}

/// @brief Convert quaternion to euler (axis-angle radians) representation
static inline void cmath_quat_toAxisAngle(Quaternionf_t q, Vec3f_t *axis, float *angle)
{
    q = cmath_quat_normalize(q);
    float sinHalfAngle = sqrtf(1.0F - q.qw * q.qw);

    if (sinHalfAngle < CMATH_EPSILON_F)
    {
        // Angle is zero so arbitrary axis
        *axis = VEC3_RIGHT;
        *angle = 0.0F;
    }
    else
    {
        *axis = (Vec3f_t){
            .x = q.qx / sinHalfAngle,
            .y = q.qy / sinHalfAngle,
            .z = q.qz / sinHalfAngle,
        };
        *angle = 2.0F * acosf(q.qw);
    }
}

/// @brief Apply quaternion rotation to a vector (v' = q * v * q^-1)
static inline Vec3f_t cmath_quat_rotateVec3(Quaternionf_t q, Vec3f_t v)
{
    Quaternionf_t qv = {.qx = v.x, .qy = v.y, .qz = v.z, .qw = 0.0F};

    Quaternionf_t qvRot = cmath_quat_mult_quat(q, qv);
    Quaternionf_t qConj = {.qx = -q.qx, .qy = -q.qy, .qz = -q.qz, .qw = q.qw};
    Quaternionf_t qResult = cmath_quat_mult_quat(qvRot, qConj);

    return (Vec3f_t){qResult.qx, qResult.qy, qResult.qz};
}

/// @brief Apply euler rotation (radians) to existing quaternion
static inline Quaternionf_t cmath_quat_applyEuler(Quaternionf_t q, Vec3f_t euler)
{
    Quaternionf_t delta = cmath_quat_fromEuler(euler);

    Quaternionf_t result = cmath_quat_mult_quat(delta, q);

    return cmath_quat_normalize(result);
}

/// @brief Inverse the direction of the quaternion
static inline Quaternionf_t cmath_quat_inverse(Quaternionf_t q)
{
    float magSq = q.qx * q.qx + q.qy * q.qy + q.qz * q.qz + q.qw * q.qw;
    if (magSq < CMATH_EPSILON_F)
    {
        // Degenerate quaternion return identity to avoid NaNs
        return QUATERNION_IDENTITY;
    }

    // Conjugate divided by magnitude squared
    float invMagSq = 1.0F / magSq;
    return (Quaternionf_t){
        .qx = -q.qx * invMagSq,
        .qy = -q.qy * invMagSq,
        .qz = -q.qz * invMagSq,
        .qw = q.qw * invMagSq,
    };
}
#pragma endregion

#pragma region Matrix Math
/// @brief Converts a 4x4 column-major matrix into a quaternion
static inline Quaternionf_t cmath_mat2quat(Mat4c_t m)
{
    Quaternionf_t q;
    float trace = m.m[0].x + m.m[1].y + m.m[2].z;

    if (trace > 0.0F)
    {
        float s = sqrtf(trace + 1.0F) * 2.0F;
        q.qw = 0.25F * s;
        q.qx = (m.m[2].y - m.m[1].z) / s;
        q.qy = (m.m[0].z - m.m[2].x) / s;
        q.qz = (m.m[1].x - m.m[0].y) / s;
    }
    else if ((m.m[0].x > m.m[1].y) && (m.m[0].x > m.m[2].z))
    {
        float s = sqrtf(1.0F + m.m[0].x - m.m[1].y - m.m[2].z) * 2.0F;
        q.qx = 0.25F * s;
        q.qw = (m.m[2].y - m.m[1].z) / s;
        q.qy = (m.m[1].x + m.m[0].y) / s; // swapped order
        q.qz = (m.m[2].x + m.m[0].z) / s; // swapped order
    }
    else if (m.m[1].y > m.m[2].z)
    {
        float s = sqrtf(1.0F + m.m[1].y - m.m[0].x - m.m[2].z) * 2.0F;
        q.qy = 0.25F * s;
        q.qw = (m.m[0].z - m.m[2].x) / s;
        q.qx = (m.m[1].x + m.m[0].y) / s;
        q.qz = (m.m[2].y + m.m[1].z) / s;
    }
    else
    {
        float s = sqrtf(1.0F + m.m[2].z - m.m[0].x - m.m[1].y) * 2.0F;
        q.qz = 0.25f * s;
        q.qw = (m.m[1].x - m.m[0].y) / s;
        q.qx = (m.m[2].x + m.m[0].z) / s;
        q.qy = (m.m[2].y + m.m[1].z) / s;
    }

    // Adjust for column-major basis (invert rotation direction)
    q.qx = -q.qx;
    q.qy = -q.qy;
    q.qz = -q.qz;

    return cmath_quat_normalize(q);
}

/// @brief Column-major Quaternion -> 4x4 Matrix
static inline Mat4c_t cmath_quat2mat(Quaternionf_t q)
{
    return (Mat4c_t){
        {{1.0F - 2.0F * (q.qy * q.qy + q.qz * q.qz), 2.0F * (q.qx * q.qy + q.qz * q.qw), 2.0F * (q.qx * q.qz - q.qy * q.qw), 0.0F},
         {2.0F * (q.qx * q.qy - q.qz * q.qw), 1.0F - 2.0F * (q.qx * q.qx + q.qz * q.qz), 2.0F * (q.qy * q.qz + q.qx * q.qw), 0.0F},
         {2.0F * (q.qx * q.qz + q.qy * q.qw), 2.0F * (q.qy * q.qz - q.qx * q.qw), 1.0F - 2.0F * (q.qx * q.qx + q.qy * q.qy), 0.0F},
         {0.0F, 0.0F, 0.0F, 1.0F}},
    };
}

/// @brief Column-major matrix × vector
static inline Vec4f_t cmath_mat_transformByVec4(Mat4c_t matrix, Vec4f_t v)
{
    return (Vec4f_t){
        .x = matrix.m[0].x * v.x + matrix.m[1].x * v.y + matrix.m[2].x * v.z + matrix.m[3].x * v.w,
        .y = matrix.m[0].y * v.x + matrix.m[1].y * v.y + matrix.m[2].y * v.z + matrix.m[3].y * v.w,
        .z = matrix.m[0].z * v.x + matrix.m[1].z * v.y + matrix.m[2].z * v.z + matrix.m[3].z * v.w,
        .w = matrix.m[0].w * v.x + matrix.m[1].w * v.y + matrix.m[2].w * v.z + matrix.m[3].w * v.w,
    };
}

/// @brief Column-major matrix translation (position matrix, so w = 1)
static inline Vec4f_t cmath_mat_translateByVec3(Mat4c_t matrix, Vec3f_t vector)
{
    return (Vec4f_t){
        .x = matrix.m[0].x * vector.x + matrix.m[1].x * vector.y + matrix.m[2].x * vector.z + matrix.m[3].x,
        .y = matrix.m[0].y * vector.x + matrix.m[1].y * vector.y + matrix.m[2].y * vector.z + matrix.m[3].y,
        .z = matrix.m[0].z * vector.x + matrix.m[1].z * vector.y + matrix.m[2].z * vector.z + matrix.m[3].z,
        .w = 1.0F,
    };
}

/// @brief Column-major matrix transformation (not position matrix specific)
static inline Vec3f_t cmath_mat_transformByVec3(Mat4c_t m, Vec3f_t v)
{
    Vec4f_t result4 = cmath_mat_translateByVec3(m, v);
    return (Vec3f_t){result4.x, result4.y, result4.z};
}

/// @brief Column-major matrix scaling (position matrix)
static inline Vec4f_t cmath_mat_scale(Mat4c_t matrix, Vec3f_t vector)
{
    return (Vec4f_t){
        .x = matrix.m[0].x * vector.x + matrix.m[1].x * vector.y + matrix.m[2].x * vector.z,
        .y = matrix.m[0].y * vector.x + matrix.m[1].y * vector.y + matrix.m[2].y * vector.z,
        .z = matrix.m[0].z * vector.x + matrix.m[1].z * vector.y + matrix.m[2].z * vector.z,
        .w = 1.0F,
    };
}

/// @brief Column-major matrix a mult by b
static inline Mat4c_t cmath_mat_mult_mat(Mat4c_t a, Mat4c_t b)
{
    Mat4c_t result;

    for (uint8_t col = 0; col < 4; ++col)
    {
        result.m[col].x = a.m[0].x * b.m[col].x + a.m[1].x * b.m[col].y +
                          a.m[2].x * b.m[col].z + a.m[3].x * b.m[col].w;

        result.m[col].y = a.m[0].y * b.m[col].x + a.m[1].y * b.m[col].y +
                          a.m[2].y * b.m[col].z + a.m[3].y * b.m[col].w;

        result.m[col].z = a.m[0].z * b.m[col].x + a.m[1].z * b.m[col].y +
                          a.m[2].z * b.m[col].z + a.m[3].z * b.m[col].w;

        result.m[col].w = a.m[0].w * b.m[col].x + a.m[1].w * b.m[col].y +
                          a.m[2].w * b.m[col].z + a.m[3].w * b.m[col].w;
    }

    return result;
}

/// @brief Rotate the given matrix by radians around the axis
static inline Mat4c_t cmath_mat_rotate(Mat4c_t m, float rad, Vec3f_t axis)
{
    axis = cmath_vec3f_normalize(axis);

    float c = cosf(rad);
    float s = sinf(rad);
    float t = 1.0f - c;

    Mat4c_t r = MAT4_IDENTITY;

    r.m[0].x = t * axis.x * axis.x + c;
    r.m[0].y = t * axis.x * axis.y + axis.z * s;
    r.m[0].z = t * axis.x * axis.z - axis.y * s;
    r.m[1].x = t * axis.x * axis.y - axis.z * s;
    r.m[1].y = t * axis.y * axis.y + c;
    r.m[1].z = t * axis.y * axis.z + axis.x * s;
    r.m[2].x = t * axis.x * axis.z + axis.y * s;
    r.m[2].y = t * axis.y * axis.z - axis.x * s;
    r.m[2].z = t * axis.z * axis.z + c;

    return cmath_mat_mult_mat(m, r);
}

/// @brief Compute the matrix representation of the vec3 eye looking at vec3 center with vec3 up orientation
static inline Mat4c_t cmath_lookAt(Vec3f_t eye, Vec3f_t center, Vec3f_t up)
{
    // Compute forward vector (center - eye)
    Vec3f_t f = {
        .x = center.x - eye.x,
        .y = center.y - eye.y,
        .z = center.z - eye.z};

    float flen = cmath_vec3f_magnitudeF(f);

    if (flen > CMATH_EPSILON_F)
    {
        f.x /= flen;
        f.y /= flen;
        f.z /= flen;
    }
    else
    {
        // Fallback forward
        f = VEC3_FORWARD;
    }

    // Normalize up
    float ulen = cmath_vec3f_magnitudeF(up);
    Vec3f_t upN = up;
    if (ulen > CMATH_EPSILON_F)
    {
        upN.x /= ulen;
        upN.y /= ulen;
        upN.z /= ulen;
    }
    else
    {
        // Fallback (use back)
        upN = VEC3_BACK;
    }

    // Compute right vector (s = f × up)
    Vec3f_t s = {
        .x = f.y * upN.z - f.z * upN.y,
        .y = f.z * upN.x - f.x * upN.z,
        .z = f.x * upN.y - f.y * upN.x};
    float slen = cmath_vec3f_magnitudeF(s);
    if (slen > CMATH_EPSILON_D)
    {
        s.x /= slen;
        s.y /= slen;
        s.z /= slen;
    }
    else
    {
        // Fallback right
        s = VEC3_RIGHT;
    }

    // Recompute orthogonal up vector
    Vec3f_t u = {
        .x = s.y * f.z - s.z * f.y,
        .y = s.z * f.x - s.x * f.z,
        .z = s.x * f.y - s.y * f.x};

    // Build column-major view matrix
    Mat4c_t result = MAT4_IDENTITY;

    result.m[0].x = s.x;
    result.m[0].y = u.x;
    result.m[0].z = -f.x;
    result.m[0].w = 0.0F;
    result.m[1].x = s.y;
    result.m[1].y = u.y;
    result.m[1].z = -f.y;
    result.m[1].w = 0.0F;
    result.m[2].x = s.z;
    result.m[2].y = u.z;
    result.m[2].z = -f.z;
    result.m[2].w = 0.0F;

    result.m[3].x = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    result.m[3].y = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    // OpenGL negative forward dot
    result.m[3].z = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    result.m[3].w = 1.0F;

    return result;
}

/// @brief Vulkan-specific perspective (z clip space [0, 1]) with aspect = width / height
static inline Mat4c_t cmath_perspective(float fovYRad, float aspect, float nearClipPlane, float farClipPlane)
{
    float tanHalfFovy = tanf(fovYRad / 2.0F);

    Mat4c_t result = {0};

    result.m[0].x = 1.0F / (aspect * tanHalfFovy);
    result.m[1].y = 1.0F / (tanHalfFovy);
    result.m[2].z = farClipPlane / (nearClipPlane - farClipPlane);
    result.m[2].w = -1.0F;
    result.m[3].z = (farClipPlane * nearClipPlane) / (nearClipPlane - farClipPlane);
    result.m[3].w = 0.0F;

    // Flip Y for Vulkan because this is based off of opengl
    result.m[1].y *= -1.0F;

    return result;
}
#pragma endregion
