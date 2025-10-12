#pragma once

#include <math.h>
#include <stdint.h>
#include "core/logs.h"
#include "c_math/c_math.h"

// COLUMN MAJOR!!!!
static const Mat4c_t MAT4_IDENTITY = {
    .m = {
        {1.0F, 0.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F, 0.0F},
        {0.0F, 0.0F, 1.0F, 0.0F},
        {0.0F, 0.0F, 0.0F, 1.0F},
    },
};

// Column-major Quaternion -> 4x4 Matrix
static inline Mat4c_t cm_quat2mat(Quaternion_t q)
{
    return (Mat4c_t){
        {{1.0f - 2.0f * (q.qy * q.qy + q.qz * q.qz), 2.0f * (q.qx * q.qy + q.qz * q.qw), 2.0f * (q.qx * q.qz - q.qy * q.qw), 0.0f},
         {2.0f * (q.qx * q.qy - q.qz * q.qw), 1.0f - 2.0f * (q.qx * q.qx + q.qz * q.qz), 2.0f * (q.qy * q.qz + q.qx * q.qw), 0.0f},
         {2.0f * (q.qx * q.qz + q.qy * q.qw), 2.0f * (q.qy * q.qz - q.qx * q.qw), 1.0f - 2.0f * (q.qx * q.qx + q.qy * q.qy), 0.0f},
         {0.0f, 0.0f, 0.0f, 1.0f}},
    };
}

// Column-major matrix × vector
static inline Vec4f_t cm_matrixTransform(Mat4c_t matrix, Vec4f_t v)
{
    return (Vec4f_t){
        .x = matrix.m[0].x * v.x + matrix.m[1].x * v.y + matrix.m[2].x * v.z + matrix.m[3].x * v.w,
        .y = matrix.m[0].y * v.x + matrix.m[1].y * v.y + matrix.m[2].y * v.z + matrix.m[3].y * v.w,
        .z = matrix.m[0].z * v.x + matrix.m[1].z * v.y + matrix.m[2].z * v.z + matrix.m[3].z * v.w,
        .w = matrix.m[0].w * v.x + matrix.m[1].w * v.y + matrix.m[2].w * v.z + matrix.m[3].w * v.w,
    };
}

// Column-major matrix translation. Warns if not a position matrix.
static inline Vec4f_t cm_matrixTranslate(Mat4c_t matrix, Vec3f_t vector)
{
    if (matrix.m[3].w != 1.0F)
    {
        logs_log(LOG_WARN, "Performed a matrix translate or scale on a non-position matrix! (p =/= 1)");
    }

    return (Vec4f_t){
        .x = matrix.m[0].x * vector.x + matrix.m[1].x * vector.y + matrix.m[2].x * vector.z + matrix.m[3].x,
        .y = matrix.m[0].y * vector.x + matrix.m[1].y * vector.y + matrix.m[2].y * vector.z + matrix.m[3].y,
        .z = matrix.m[0].z * vector.x + matrix.m[1].z * vector.y + matrix.m[2].z * vector.z + matrix.m[3].z,
        .w = 1.0F,
    };
}

// Column-major matrix scaling. Warns if not a position matrix.
static inline Vec4f_t cm_matrixScale(Mat4c_t matrix, Vec3f_t vector)
{
    if (matrix.m[3].w != 1.0F)
    {
        logs_log(LOG_WARN, "Performed a matrix scale on a non-position matrix! (p =/= 1)");
    }

    return (Vec4f_t){
        .x = matrix.m[0].x * vector.x + matrix.m[1].x * vector.y + matrix.m[2].x * vector.z,
        .y = matrix.m[0].y * vector.x + matrix.m[1].y * vector.y + matrix.m[2].y * vector.z,
        .z = matrix.m[0].z * vector.x + matrix.m[1].z * vector.y + matrix.m[2].z * vector.z,
        .w = 1.0F,
    };
}

static inline Mat4c_t cm_matrixMultiply(Mat4c_t a, Mat4c_t b)
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

static inline Mat4c_t cm_matrixRotate(Mat4c_t m, float rad, Vec3f_t axis)
{
    float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len < 1e-6f)
        return m;

    float x = axis.x / len;
    float y = axis.y / len;
    float z = axis.z / len;

    float c = cosf(rad);
    float s = sinf(rad);
    float t = 1.0f - c;

    Mat4c_t r = MAT4_IDENTITY;

    r.m[0].x = t * x * x + c;
    r.m[0].y = t * x * y + z * s;
    r.m[0].z = t * x * z - y * s;
    r.m[1].x = t * x * y - z * s;
    r.m[1].y = t * y * y + c;
    r.m[1].z = t * y * z + x * s;
    r.m[2].x = t * x * z + y * s;
    r.m[2].y = t * y * z - x * s;
    r.m[2].z = t * z * z + c;

    return cm_matrixMultiply(m, r);
}

static inline Mat4c_t cm_lookAt(Vec3f_t eye, Vec3f_t center, Vec3f_t up)
{
    // Compute forward vector (center - eye)
    Vec3f_t f = {
        .x = center.x - eye.x,
        .y = center.y - eye.y,
        .z = center.z - eye.z};

    float flen = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
    if (flen > 1e-6f)
    {
        f.x /= flen;
        f.y /= flen;
        f.z /= flen;
    }
    else
    {
        // Fallback forward
        f = (Vec3f_t){0.0f, 0.0f, -1.0f};
    }

    // Normalize up
    float ulen = sqrtf(up.x * up.x + up.y * up.y + up.z * up.z);
    Vec3f_t upN = up;
    if (ulen > 1e-6f)
    {
        upN.x /= ulen;
        upN.y /= ulen;
        upN.z /= ulen;
    }
    else
    {
        // Fallback up
        upN = (Vec3f_t){0.0f, 0.0f, 1.0f};
    }

    // Compute right vector (s = f × up)
    Vec3f_t s = {
        .x = f.y * upN.z - f.z * upN.y,
        .y = f.z * upN.x - f.x * upN.z,
        .z = f.x * upN.y - f.y * upN.x};
    float slen = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
    if (slen > 1e-6f)
    {
        s.x /= slen;
        s.y /= slen;
        s.z /= slen;
    }
    else
    {
        // Fallback right
        s = (Vec3f_t){1.0f, 0.0f, 0.0f};
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
    result.m[0].w = 0.0f;
    result.m[1].x = s.y;
    result.m[1].y = u.y;
    result.m[1].z = -f.y;
    result.m[1].w = 0.0f;
    result.m[2].x = s.z;
    result.m[2].y = u.z;
    result.m[2].z = -f.z;
    result.m[2].w = 0.0f;

    result.m[3].x = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
    result.m[3].y = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
    // OpenGL negative forward dot
    result.m[3].z = f.x * eye.x + f.y * eye.y + f.z * eye.z;
    result.m[3].w = 1.0f;

    return result;
}

// Vulkan-specific perspective (z clip space [0, 1])
// Aspect = width / height
static inline Mat4c_t cm_perspective(float fovYRad, float aspect, float nearClipPlane, float farClipPlane)
{
    float tanHalfFovy = tanf(fovYRad / 2.0f);

    Mat4c_t result = {0};

    result.m[0].x = 1.0f / (aspect * tanHalfFovy);
    result.m[1].y = 1.0f / (tanHalfFovy);
    result.m[2].z = farClipPlane / (nearClipPlane - farClipPlane);
    result.m[2].w = -1.0f;
    result.m[3].z = (farClipPlane * nearClipPlane) / (nearClipPlane - farClipPlane);
    result.m[3].w = 0.0f;

    // Flip Y for Vulkan because this is based off of opengl
    result.m[1].y *= -1.0f;

    return result;
}
