#include "c_math/c_math.h"
#include "core/logs.h"
#include <math.h>
#include <float.h>

// A "good enough" delta
#define QUAT_TEST_EPS 1e-5F

/// @brief Check if two floats are nearly equal
static inline bool near_equal(float a, float b)
{
    return fabsf(a - b) < QUAT_TEST_EPS;
}

/// @brief Check if two Vec3f_t are nearly equal
static inline bool near_vec3(Vec3f_t a, Vec3f_t b)
{
    return near_equal(a.x, b.x) && near_equal(a.y, b.y) && near_equal(a.z, b.z);
}

/// @brief Check if two Quaternion_t are nearly equal (ignoring sign)
static inline bool near_quat(Quaternion_t a, Quaternion_t b)
{
    if (a.qx * b.qx + a.qy * b.qy + a.qz * b.qz + a.qw * b.qw < 0.0F)
    {
        b.qx = -b.qx;
        b.qy = -b.qy;
        b.qz = -b.qz;
        b.qw = -b.qw;
    }

    return near_equal(a.qx, b.qx) &&
           near_equal(a.qy, b.qy) &&
           near_equal(a.qz, b.qz) &&
           near_equal(a.qw, b.qw);
}

/// @brief Verify that converting Euler→Quat→Euler yields approximately the same rotation
static int test_quat_euler_roundtrip(void)
{
    int fails = 0;

    // Input Euler (pitch, yaw, roll)
    Vec3f_t input = {0.25F * PI_F, 0.5F * PI_F, -0.25F * PI_F};

    // Euler -> Quaternion -> Euler
    Quaternion_t q1 = cm_quatFromEuler(input);
    Vec3f_t euler_out = cm_quatToEulerAngles(q1);

    // Convert back to quaternion to compare rotation equivalence, not raw angles
    Quaternion_t q2 = cm_quatFromEuler(euler_out);

    if (!near_quat(q1, q2))
    {
        logs_log(LOG_UNIT_TEST,
                 "Euler roundtrip mismatch: in (%.4f, %.4f, %.4f) out (%.4f, %.4f, %.4f)",
                 input.x, input.y, input.z,
                 euler_out.x, euler_out.y, euler_out.z);
        fails++;
    }

    return fails;
}

/// @brief Verify that AxisAngle->Quat->AxisAngle reproduces same rotation
static int test_quat_axis_roundtrip(void)
{
    int fails = 0;

    Vec3f_t axis = cm_vec3fNormalize((Vec3f_t){1.0F, 2.0F, 3.0F});
    float angle = 1.2345F; // radians

    Quaternion_t q = cm_quatFromAxisAngle(angle, axis);

    Vec3f_t axis2;
    float angle2;
    cm_quatToAxisAngle(q, &axis2, &angle2);

    if (!near_equal(angle, angle2) || !near_vec3(axis, axis2))
    {
        logs_log(LOG_UNIT_TEST,
                 "AxisAngle roundtrip mismatch: in (axis %.3f,%.3f,%.3f angle %.3f) "
                 "out (axis %.3f,%.3f,%.3f angle %.3f)",
                 axis.x, axis.y, axis.z, angle,
                 axis2.x, axis2.y, axis2.z, angle2);
        fails++;
    }

    return fails;
}

/// @brief Verify that Euler->Quat->Matrix->Quat->Euler is consistent
static int test_quat_matrix_roundtrip(void)
{
    int fails = 0;

    Vec3f_t euler = {PI_F * 0.3F, PI_F * 0.4F, PI_F * -0.2F};
    Quaternion_t q1 = cm_quatFromEuler(euler);

    Mat4c_t m = cm_quat2mat(q1);
    Quaternion_t q2 = cm_mat2quat(m);
    q2 = cm_quatNormalize(q2); // ensure consistent scale

    if (!near_quat(q1, q2))
    {
        logs_log(LOG_UNIT_TEST,
                 "Matrix roundtrip mismatch: q1=(%.5f,%.5f,%.5f,%.5f) q2=(%.5f,%.5f,%.5f,%.5f)",
                 q1.qx, q1.qy, q1.qz, q1.qw,
                 q2.qx, q2.qy, q2.qz, q2.qw);
        fails++;
    }

    return fails;
}

/// @brief Run all quaternion unit tests
int quatTests_run(void)
{
    int failures = 0;

    failures += test_quat_euler_roundtrip();
    failures += test_quat_axis_roundtrip();
    failures += test_quat_matrix_roundtrip();

    return failures;
}

void mathTests_run(void)
{
    logs_logIfError(quatTests_run(), "Math test(s) failed!");
}