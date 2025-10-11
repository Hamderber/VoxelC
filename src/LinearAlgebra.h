#pragma once
/*  Notes:
    1. Vectors must be floats to support direct use in shaders
    2. https://open.gl/transformations
    3. https://www.opengl-tutorial.org/beginners-tutorials/tutorial-3-matrices/#rotation-matrices
*/

float la_rad2degf(float radians)
{
    const float rad2deg = 180.0F / PI_F;
    return radians * rad2deg;
}

double la_rad2degd(double radians)
{
    const double rad2deg = 180.0 / PI_D;
    return radians * rad2deg;
}

float la_deg2radf(float degrees)
{
    const float deg2rad = PI_F / 180.0F;
    return degrees * deg2rad;
}

double la_deg2radd(double degrees)
{
    const double deg2rad = PI_D / 180.0;
    return degrees * deg2rad;
}

// Column-major Quaternion -> 4x4 Matrix
Mat4c_t la_quat2mat(Quaternion_t q)
{
    return (Mat4c_t){
        {{1.0f - 2.0f * (q.qy * q.qy + q.qz * q.qz), 2.0f * (q.qx * q.qy + q.qz * q.qw), 2.0f * (q.qx * q.qz - q.qy * q.qw), 0.0f},
         {2.0f * (q.qx * q.qy - q.qz * q.qw), 1.0f - 2.0f * (q.qx * q.qx + q.qz * q.qz), 2.0f * (q.qy * q.qz + q.qx * q.qw), 0.0f},
         {2.0f * (q.qx * q.qz + q.qy * q.qw), 2.0f * (q.qy * q.qz - q.qx * q.qw), 1.0f - 2.0f * (q.qx * q.qx + q.qy * q.qy), 0.0f},
         {0.0f, 0.0f, 0.0f, 1.0f}},
    };
}

Quaternion_t la_mat2quat(Mat4c_t m)
{
    Quaternion_t q;
    float trace = m.m[0].x + m.m[1].y + m.m[2].z;

    if (trace > 0.0f)
    {
        float s = sqrtf(trace + 1.0f) * 2.0f; // s = 4 * qw
        q.qw = 0.25f * s;
        q.qx = (m.m[2].y - m.m[1].z) / s;
        q.qy = (m.m[0].z - m.m[2].x) / s;
        q.qz = (m.m[1].x - m.m[0].y) / s;
    }
    else if ((m.m[0].x > m.m[1].y) && (m.m[0].x > m.m[2].z))
    {
        float s = sqrtf(1.0f + m.m[0].x - m.m[1].y - m.m[2].z) * 2.0f; // s = 4 * qx
        q.qw = (m.m[2].y - m.m[1].z) / s;
        q.qx = 0.25f * s;
        q.qy = (m.m[0].y + m.m[1].x) / s;
        q.qz = (m.m[0].z + m.m[2].x) / s;
    }
    else if (m.m[1].y > m.m[2].z)
    {
        float s = sqrtf(1.0f + m.m[1].y - m.m[0].x - m.m[2].z) * 2.0f; // s = 4 * qy
        q.qw = (m.m[0].z - m.m[2].x) / s;
        q.qx = (m.m[0].y + m.m[1].x) / s;
        q.qy = 0.25f * s;
        q.qz = (m.m[1].z + m.m[2].y) / s;
    }
    else
    {
        float s = sqrtf(1.0f + m.m[2].z - m.m[0].x - m.m[1].y) * 2.0f; // s = 4 * qz
        q.qw = (m.m[1].x - m.m[0].y) / s;
        q.qx = (m.m[0].z + m.m[2].x) / s;
        q.qy = (m.m[1].z + m.m[2].y) / s;
        q.qz = 0.25f * s;
    }

    return q;
}

Quaternion_t la_quatNormalize(Quaternion_t quat)
{
    /*
        Uq = q/|q| = (qx + qy + qz + qw) / sqrt(qx^2 + qy^2 + qz^2 + qw^2)
    */
    // Inverse of square root of sum of squaring each quaternion component. (Inverse of Vector4 magnitude).
    float mag = sqrtf(quat.qx * quat.qx + quat.qy * quat.qy + quat.qz * quat.qz + quat.qw * quat.qw);

    // Avoid divide by zero
    if (mag != 0.0F)
    {
        float denom = 1.0f / mag;

        return (Quaternion_t){
            .qx = quat.qx * denom,
            .qy = quat.qy * denom,
            .qz = quat.qz * denom,
            .qw = quat.qw * denom,
        };
    }
    else
    {
        return QUAT_IDENTITY;
    }
}

// Multiply two quaternions (rotation composition)
// Order matters: q = q1 * q2 means "apply q2, then q1"
Quaternion_t la_quatMultiply(Quaternion_t a, Quaternion_t b)
{
    return (Quaternion_t){
        .qx = a.qw * b.qx + a.qx * b.qw + a.qy * b.qz - a.qz * b.qy,
        .qy = a.qw * b.qy - a.qx * b.qz + a.qy * b.qw + a.qz * b.qx,
        .qz = a.qw * b.qz + a.qx * b.qy - a.qy * b.qx + a.qz * b.qw,
        .qw = a.qw * b.qw - a.qx * b.qx - a.qy * b.qy - a.qz * b.qz,
    };
}

// Create a quaternion representing rotation about an axis
Quaternion_t la_quatAngleAxis(float radians, Vec3f_t axis)
{
    float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
    if (len < 1e-6f)
    {
        // Avoid divide by zero, fallback to identity
        return QUAT_IDENTITY;
    }

    float s = sinf(radians * 0.5f);
    float c = cosf(radians * 0.5f);

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

// Rotate one quaternion toward another (spherical linear interpolation)
Quaternion_t la_quatSlerp(Quaternion_t a, Quaternion_t b, float t)
{
    // Clamp t [0, 1]
    if (t < 0.0f)
        t = 0.0f;
    else if (t > 1.0f)
        t = 1.0f;

    // Compute cosine of the angle between the two quaternions
    float dot = a.qx * b.qx + a.qy * b.qy + a.qz * b.qz + a.qw * b.qw;

    // If dot < 0, the quaternions are opposite so negate one to take the shortest path
    if (dot < 0.0f)
    {
        b.qx = -b.qx;
        b.qy = -b.qy;
        b.qz = -b.qz;
        b.qw = -b.qw;
        dot = -dot;
    }

    const float DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD)
    {
        // Quaternions are the same so use linear interpolation
        Quaternion_t result = {
            .qx = a.qx + t * (b.qx - a.qx),
            .qy = a.qy + t * (b.qy - a.qy),
            .qz = a.qz + t * (b.qz - a.qz),
            .qw = a.qw + t * (b.qw - a.qw),
        };
        return la_quatNormalize(result);
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

    return la_quatNormalize(result);
}

// Convert quaternion to axis-angle representation
void la_quatToAxisAngle(Quaternion_t q, Vec3f_t *axis, float *angle)
{
    q = la_quatNormalize(q);
    float sinHalfAngle = sqrtf(1.0f - q.qw * q.qw);

    if (sinHalfAngle < 1e-6f)
    {
        // Angle is zero so arbitrary axis
        *axis = (Vec3f_t){1.0f, 0.0f, 0.0f};
        *angle = 0.0f;
    }
    else
    {
        *axis = (Vec3f_t){
            .x = q.qx / sinHalfAngle,
            .y = q.qy / sinHalfAngle,
            .z = q.qz / sinHalfAngle,
        };
        *angle = 2.0f * acosf(q.qw);
    }
}

// Apply quaternion rotation to a vector (v' = q * v * q^-1)
Vec3f_t la_quatRotateVec3(Quaternion_t q, Vec3f_t v)
{
    Quaternion_t qv = {.qx = v.x, .qy = v.y, .qz = v.z, .qw = 0.0f};

    Quaternion_t qvRot = la_quatMultiply(q, qv);
    Quaternion_t qConj = {.qx = -q.qx, .qy = -q.qy, .qz = -q.qz, .qw = q.qw};
    Quaternion_t qResult = la_quatMultiply(qvRot, qConj);

    return (Vec3f_t){qResult.qx, qResult.qy, qResult.qz};
}

// Column-major matrix × vector
Vec4f_t la_matrixTransform(Mat4c_t matrix, Vec4f_t v)
{
    return (Vec4f_t){
        .x = matrix.m[0].x * v.x + matrix.m[1].x * v.y + matrix.m[2].x * v.z + matrix.m[3].x * v.w,
        .y = matrix.m[0].y * v.x + matrix.m[1].y * v.y + matrix.m[2].y * v.z + matrix.m[3].y * v.w,
        .z = matrix.m[0].z * v.x + matrix.m[1].z * v.y + matrix.m[2].z * v.z + matrix.m[3].z * v.w,
        .w = matrix.m[0].w * v.x + matrix.m[1].w * v.y + matrix.m[2].w * v.z + matrix.m[3].w * v.w,
    };
}

// Column-major matrix translation. Warns if not a position matrix.
Vec4f_t la_matrixTranslate(Mat4c_t matrix, Vec3f_t vector)
{
    if (matrix.m[3].w != 1.0F)
    {
        logger(LOG_WARN, "Performed a matrix translate or scale on a non-position matrix! (p =/= 1)");
    }

    return (Vec4f_t){
        .x = matrix.m[0].x * vector.x + matrix.m[1].x * vector.y + matrix.m[2].x * vector.z + matrix.m[3].x,
        .y = matrix.m[0].y * vector.x + matrix.m[1].y * vector.y + matrix.m[2].y * vector.z + matrix.m[3].y,
        .z = matrix.m[0].z * vector.x + matrix.m[1].z * vector.y + matrix.m[2].z * vector.z + matrix.m[3].z,
        .w = 1.0F,
    };
}

// Column-major matrix scaling. Warns if not a position matrix.
Vec4f_t la_matrixScale(Mat4c_t matrix, Vec3f_t vector)
{
    if (matrix.m[3].w != 1.0F)
    {
        logger(LOG_WARN, "Performed a matrix scale on a non-position matrix! (p =/= 1)");
    }

    return (Vec4f_t){
        .x = matrix.m[0].x * vector.x + matrix.m[1].x * vector.y + matrix.m[2].x * vector.z,
        .y = matrix.m[0].y * vector.x + matrix.m[1].y * vector.y + matrix.m[2].y * vector.z,
        .z = matrix.m[0].z * vector.x + matrix.m[1].z * vector.y + matrix.m[2].z * vector.z,
        .w = 1.0F,
    };
}

static inline Mat4c_t la_matrixMultiply(Mat4c_t a, Mat4c_t b)
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

Mat4c_t la_matrixRotate(Mat4c_t m, float rad, Vec3f_t axis)
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

    return la_matrixMultiply(m, r);
}

Mat4c_t la_lookAt(Vec3f_t eye, Vec3f_t center, Vec3f_t up)
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
Mat4c_t la_perspective(float fovYRad, float aspect, float nearClipPlane, float farClipPlane)
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
