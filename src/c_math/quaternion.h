#pragma once

#include "c_math/vector.h"
#include "c_math/matrix.h"

/// @brief The Identity Quaternion representing no rotation and no shear
static const Quaternion_t QUATERNION_IDENTITY = {0.0F, 0.0F, 0.0F, 1.0F};

static inline bool cm_quatIsIdentity(Quaternion_t q)
{
    // qw = -1 also is identity
    return fabs(q.qx) < FLT_EPSILON && fabs(q.qy) < FLT_EPSILON && fabs(q.qz) < FLT_EPSILON && fabs(1.0F - fabs(q.qw)) < FLT_EPSILON;
}

/// @brief Scales the quaternion to have a normalized magnitude of 1 (necessary for rotation representations)
/// @param quat
/// @return
static inline Quaternion_t cm_quatNormalize(Quaternion_t quat)
{
    /*
        Uq = q/|q| = (qx + qy + qz + qw) / sqrt(qx^2 + qy^2 + qz^2 + qw^2)
    */
    // Inverse of square root of sum of squaring each quaternion component. (Inverse of Vector4 magnitude).
    float mag = sqrtf(quat.qx * quat.qx + quat.qy * quat.qy + quat.qz * quat.qz + quat.qw * quat.qw);

    // Avoid divide by zero
    if (mag != 0.0F)
    {
        float denom = 1.0F / mag;

        return (Quaternion_t){
            .qx = quat.qx * denom,
            .qy = quat.qy * denom,
            .qz = quat.qz * denom,
            .qw = quat.qw * denom,
        };
    }
    else
    {
        return QUATERNION_IDENTITY;
    }
}

/// @brief Converts a 4x4 column-major matrix into a quaternion
/// @param m
/// @return Quaternion_t
static inline Quaternion_t cm_mat2quat(Mat4c_t m)
{
    Quaternion_t q;
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

    return cm_quatNormalize(q);
}

/// @brief Multiply two quaternions (rotation composition). Order matters: q = q1 * q2 means "apply q2, then q1"
/// @param a
/// @param b
/// @return Quaternion_t
static inline Quaternion_t cm_quatMultiply(Quaternion_t a, Quaternion_t b)
{
    return (Quaternion_t){
        .qx = a.qw * b.qx + a.qx * b.qw + a.qy * b.qz - a.qz * b.qy,
        .qy = a.qw * b.qy - a.qx * b.qz + a.qy * b.qw + a.qz * b.qx,
        .qz = a.qw * b.qz + a.qx * b.qy - a.qy * b.qx + a.qz * b.qw,
        .qw = a.qw * b.qw - a.qx * b.qx - a.qy * b.qy - a.qz * b.qz,
    };
}

/// @brief Create a new quaternion representing euler rotation about an arbitrary axis radians
/// @param radians
/// @param axis
/// @return Quaternion_t
static inline Quaternion_t cm_quatFromAxisAngle(float radians, Vec3f_t axis)
{
    float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len < 1E-6F)
    {
        // Avoid divide by zero, fallback to identity
        return QUATERNION_IDENTITY;
    }

    float s = sinf(radians * 0.5F);
    float c = cosf(radians * 0.5F);

    float nx = axis.x / len;
    float ny = axis.y / len;
    float nz = axis.z / len;

    return (Quaternion_t){
        .qx = nx * s,
        .qy = ny * s,
        .qz = nz * s,
        .qw = c,
    };
}

/// @brief Rotate one quaternion toward another (spherical linear interpolation)
/// @param a
/// @param b
/// @param t
/// @return Quaternion_t
static inline Quaternion_t cm_quatSlerp(Quaternion_t a, Quaternion_t b, float t)
{
    t = cm_clampf(t, 0.0F, 1.0F);

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

    const float DOT_THRESHOLD = 0.9995F;
    if (dot > DOT_THRESHOLD)
    {
        // Quaternions are the same so use linear interpolation
        Quaternion_t result = {
            .qx = a.qx + t * (b.qx - a.qx),
            .qy = a.qy + t * (b.qy - a.qy),
            .qz = a.qz + t * (b.qz - a.qz),
            .qw = a.qw + t * (b.qw - a.qw),
        };
        return cm_quatNormalize(result);
    }

    // Compute the angle between them
    float theta_0 = acosf(dot);
    float theta = theta_0 * t;

    float sin_theta_0 = sinf(theta_0);
    float sin_theta = sinf(theta);

    float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    Quaternion_t result = {
        .qx = (a.qx * s0) + (b.qx * s1),
        .qy = (a.qy * s0) + (b.qy * s1),
        .qz = (a.qz * s0) + (b.qz * s1),
        .qw = (a.qw * s0) + (b.qw * s1),
    };

    return cm_quatNormalize(result);
}

/// @brief Convert euler (axis-angle) to quaternion in yaw/pitch/roll order
/// @param pitch (X)
/// @param yaw (Y)
/// @param roll (Z)
/// @return Quaternion_t
static inline Quaternion_t cm_quatFromEuler(Vec3f_t euler)
{
    // half-angles
    float halfX = euler.x * 0.5F;
    float halfY = euler.y * 0.5F;
    float halfZ = euler.z * 0.5F;

    // sin/cos
    float sx = sinf(halfX), cx = cosf(halfX);
    float sy = sinf(halfY), cy = cosf(halfY);
    float sz = sinf(halfZ), cz = cosf(halfZ);

    // Order: Yaw (Y), Pitch (X), Roll (Z)
    Quaternion_t q = {
        .qx = sx * cy * cz + cx * sy * sz,
        .qy = cx * sy * cz - sx * cy * sz,
        .qz = cx * cy * sz + sx * sy * cz,
        .qw = cx * cy * cz - sx * sy * sz};

    return cm_quatNormalize(q);
}

/// @brief Convert quaternion to euler (radians) in pitch/yaw/roll order
/// @param q
/// @return
static inline Vec3f_t cm_quatToEulerAngles(Quaternion_t q)
{
    q = cm_quatNormalize(q);
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
/// @param q
/// @param axis
/// @param angle
static inline void cm_quatToAxisAngle(Quaternion_t q, Vec3f_t *axis, float *angle)
{
    q = cm_quatNormalize(q);
    float sinHalfAngle = sqrtf(1.0F - q.qw * q.qw);

    if (sinHalfAngle < FLT_EPSILON)
    {
        // Angle is zero so arbitrary axis
        *axis = RIGHT;
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
/// @param q
/// @param v
/// @return Vec3f_t
static inline Vec3f_t cm_quatRotateVec3(Quaternion_t q, Vec3f_t v)
{
    Quaternion_t qv = {.qx = v.x, .qy = v.y, .qz = v.z, .qw = 0.0F};

    Quaternion_t qvRot = cm_quatMultiply(q, qv);
    Quaternion_t qConj = {.qx = -q.qx, .qy = -q.qy, .qz = -q.qz, .qw = q.qw};
    Quaternion_t qResult = cm_quatMultiply(qvRot, qConj);

    return (Vec3f_t){qResult.qx, qResult.qy, qResult.qz};
}

/// @brief Apply euler rotation (radians) to existing quaternion
/// @param q
/// @param euler euler radians
/// @return Quaternion_t
static inline Quaternion_t cm_quatApplyEuler(Quaternion_t q, Vec3f_t euler)
{
    Quaternion_t delta = cm_quatFromEuler(euler);

    Quaternion_t result = cm_quatMultiply(delta, q);

    return cm_quatNormalize(result);
}
