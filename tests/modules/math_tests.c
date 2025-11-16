#include "../src/cmath/cmath.h"
#include "../unit_tests.h"

static int fails = 0;

static inline int almost_eq(float a, float b, float eps) { return fabsf(a - b) <= eps; }
static const float EPS = 1e-4f;
static const float EPS_BIG = 2e-3f;

void test_arithmetic(void)
{
    fails += ut_assert(cmath_clampF(2.0F, 0.0F, 1.0F) == 1.0F, "ClampF > max");
    fails += ut_assert(cmath_clampF(1.0F, 0.0F, 1.0F) == 1.0F, "ClampF = max");
    fails += ut_assert(cmath_clampF(-2.0F, 0.0F, 1.0F) == 0.0F, "ClampF < min");
    fails += ut_assert(cmath_clampF(0.0F, 0.0F, 1.0F) == 0.0F, "ClampF = min");

    fails += ut_assert(cmath_clampD(2.0, 0.0, 1.0) == 1.0, "ClampD > max");
    fails += ut_assert(cmath_clampD(1.0, 0.0, 1.0) == 1.0, "ClampD = max");
    fails += ut_assert(cmath_clampD(-2.0, 0.0, 1.0) == 0.0, "ClampD < min");
    fails += ut_assert(cmath_clampD(0.0, 0.0, 1.0) == 0.0, "ClampD = min");

    fails += ut_assert(cmath_clampU32t(3U, 1U, 2U) == 2U, "ClampU32t > max");
    fails += ut_assert(cmath_clampU32t(2U, 1U, 2U) == 2U, "ClampU32t = max");
    fails += ut_assert(cmath_clampU32t(0U, 1U, 2U) == 1U, "ClampU32t < min");
    fails += ut_assert(cmath_clampU32t(1U, 1U, 2U) == 1U, "ClampU32t = min");
}

void test_quaternion(void)
{
    // Identity -> matrices
    {
        Quaternionf_t q = QUATERNION_IDENTITY;
        Mat4c_t m_full = cmath_quat2mat(q);
        fails += ut_assert(fabsf(m_full.m[0].x - 1.0f) < EPS && fabsf(m_full.m[1].y - 1.0f) < EPS && fabsf(m_full.m[2].z - 1.0f) < EPS,
                           "quat identity to full matrix");
    }

    // Pure yaw 90 deg about +Y. Forward (0,0,1) should go to +X.
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0f), VEC3F_UP);
        Mat4c_t m_full = cmath_quat2mat(q);
        Vec3f_t fwd = (Vec3f_t){0, 0, 1};
        Vec3f_t r_full = cmath_mat_transformByVec3(m_full, fwd);
        fails += ut_assert(fabsf(r_full.x - 1.0f) < EPS_BIG && fabsf(r_full.z) < EPS_BIG,
                           "yaw 90 full rotates forward to +X");
    }

    // Pure pitch 90 deg about +X. Up (0,1,0) should go to +Z.
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0f), VEC3F_RIGHT);
        Mat4c_t m_full = cmath_quat2mat(q);
        Vec3f_t up = (Vec3f_t){0, 1, 0};
        Vec3f_t r_full = cmath_mat_transformByVec3(m_full, up);
        fails += ut_assert(fabsf(r_full.z - 1.0f) < EPS_BIG && fabsf(r_full.y) < EPS_BIG,
                           "pitch 90 full rotates up to +Z");
    }

    // Pure roll 90 deg about +Z
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0f), VEC3F_BACK);
        Mat4c_t m_full = cmath_quat2mat(q);
        Vec3f_t right = (Vec3f_t){1, 0, 0};
        Vec3f_t r_full = cmath_mat_transformByVec3(m_full, right);
        fails += ut_assert(fabsf(r_full.y - 1.0f) < EPS_BIG && fabsf(r_full.x) < EPS_BIG,
                           "roll 90 full rotates +X to +Y");
    }

    // Combined yaw then pitch. Compare quatRotateVec3 vs matrix transform.
    {
        Quaternionf_t qYaw = cmath_quat_fromAxisAngle(cmath_deg2radF(30.0f), VEC3F_UP);
        Quaternionf_t qPitch = cmath_quat_fromAxisAngle(cmath_deg2radF(20.0f), VEC3F_RIGHT);
        Quaternionf_t q_full = cmath_quat_mult_quat(qYaw, qPitch);
        Vec3f_t v = (Vec3f_t){0.3f, -0.7f, 1.2f};
        Mat4c_t m_full = cmath_quat2mat(q_full);
        Vec3f_t a_full = cmath_quat_rotateVec3(q_full, v);
        Vec3f_t b_full = cmath_mat_transformByVec3(m_full, v);
        fails += ut_assert(fabsf(a_full.x - b_full.x) < EPS && fabsf(a_full.y - b_full.y) < EPS && fabsf(a_full.z - b_full.z) < EPS,
                           "combined yaw+pitch full: quatRotate matches matrix");
    }

    // Normalization checks
    {
        Quaternionf_t q = QUATERNION_IDENTITY;
        Quaternionf_t n = cmath_quat_normalize(q);
        fails += ut_assert(fabsf(n.qx) < 1e-6f && fabsf(n.qy) < 1e-6f && fabsf(n.qz) < 1e-6f && fabsf(n.qw - 1.0f) < 1e-6f,
                           "quat normalize identity (full)");
    }

    // Inverse: q * q^-1 = identity
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(45.0f), VEC3F_RIGHT);
        Quaternionf_t qi = cmath_quat_inverse(q);
        Quaternionf_t id = cmath_quat_mult_quat(q, qi);
        fails += ut_assert(fabsf(id.qx) < EPS && fabsf(id.qy) < EPS && fabsf(id.qz) < EPS && fabsf(id.qw - 1.0f) < EPS,
                           "quat inverse full produces identity");
    }

    // Multiplication non-commutativity
    {
        Quaternionf_t qYaw = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0f), VEC3F_UP);
        Quaternionf_t qPitch = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0f), VEC3F_RIGHT);
        Quaternionf_t a = cmath_quat_mult_quat(qYaw, qPitch);
        Quaternionf_t b = cmath_quat_mult_quat(qPitch, qYaw);
        int diff = (fabsf(a.qx - b.qx) > 1e-4f) || (fabsf(a.qy - b.qy) > 1e-4f) || (fabsf(a.qz - b.qz) > 1e-4f) || (fabsf(a.qw - b.qw) > 1e-4f);
        fails += ut_assert(diff, "quat multiply is non-commutative");
    }

    // SLERP endpoints and mid for full
    {
        Quaternionf_t a = cmath_quat_fromEuler((Vec3f_t){0, 0, 0});
        Quaternionf_t b = cmath_quat_fromEuler((Vec3f_t){cmath_deg2radF(30), cmath_deg2radF(45), cmath_deg2radF(10)});

        Quaternionf_t r0 = cmath_quat_slerp(a, b, 0.0f);
        Quaternionf_t r1 = cmath_quat_slerp(a, b, 1.0f);
        fails += ut_assert(fabsf(r0.qx - a.qx) < EPS && fabsf(r0.qy - a.qy) < EPS && fabsf(r0.qz - a.qz) < EPS && fabsf(r0.qw - a.qw) < EPS,
                           "slerp full t=0 equals a");
        fails += ut_assert(fabsf(r1.qx - b.qx) < EPS && fabsf(r1.qy - b.qy) < EPS && fabsf(r1.qz - b.qz) < EPS && fabsf(r1.qw - b.qw) < EPS,
                           "slerp full t=1 equals b");
    }

    // Mat -> Quat -> Mat roundtrip (full and noRoll)
    {
        // full
        Quaternionf_t qf0 = cmath_quat_fromEuler((Vec3f_t){cmath_deg2radF(15), cmath_deg2radF(-25), cmath_deg2radF(10)});
        Mat4c_t Mf0 = cmath_quat2mat(qf0);
        Quaternionf_t qf1 = cmath_mat2quat(Mf0);
        Mat4c_t Mf1 = cmath_quat2mat(qf1);

        float err_full =
            fabsf(Mf0.m[0].x - Mf1.m[0].x) + fabsf(Mf0.m[0].y - Mf1.m[0].y) + fabsf(Mf0.m[0].z - Mf1.m[0].z) +
            fabsf(Mf0.m[1].x - Mf1.m[1].x) + fabsf(Mf0.m[1].y - Mf1.m[1].y) + fabsf(Mf0.m[1].z - Mf1.m[1].z) +
            fabsf(Mf0.m[2].x - Mf1.m[2].x) + fabsf(Mf0.m[2].y - Mf1.m[2].y) + fabsf(Mf0.m[2].z - Mf1.m[2].z);

        fails += ut_assert(err_full < 5e-3f, "mat->quat->mat roundtrip (full)");
    }
}

void test_matrix(void)
{
    // cmath_mat_rotate yaw 90 about +Y, both full and noRoll
    {
        Mat4c_t r_full = cmath_mat_rotate(MAT4_IDENTITY, cmath_deg2radF(90.0f), VEC3F_UP);
        Vec3f_t fwd = (Vec3f_t){0, 0, 1};
        Vec3f_t a = cmath_mat_transformByVec3(r_full, fwd);
        fails += ut_assert(fabsf(a.x - 1.0f) < EPS_BIG && fabsf(a.z) < EPS_BIG,
                           "matrixRotate full yaw 90");
    }

    // cmath_mat_rotate pitch 90 about +X, both paths
    {
        Mat4c_t r_full = cmath_mat_rotate(MAT4_IDENTITY, cmath_deg2radF(90.0f), VEC3F_RIGHT);
        Vec3f_t up = (Vec3f_t){0, 1, 0};
        Vec3f_t a = cmath_mat_transformByVec3(r_full, up);
        fails += ut_assert(fabsf(a.z - 1.0f) < EPS_BIG && fabsf(a.y) < EPS_BIG,
                           "matrixRotate full pitch 90");
    }

    // cmath_mat_rotate roll 90 about +Z. Full rotates +X to +Y; noRoll ignores.
    {
        Mat4c_t r_full = cmath_mat_rotate(MAT4_IDENTITY, cmath_deg2radF(90.0f), VEC3F_BACK);
        Vec3f_t right = (Vec3f_t){1, 0, 0};
        Vec3f_t a = cmath_mat_transformByVec3(r_full, right);
        fails += ut_assert(fabsf(a.y - 1.0f) < EPS_BIG && fabsf(a.x) < EPS_BIG,
                           "matrixRotate full roll 90");
    }

    // cmath_lookAt standard vs cm_lookAt_noRoll horizon-locked up
    {
        Vec3f_t eye = (Vec3f_t){0, 10, 0};
        Vec3f_t center = (Vec3f_t){10, 10, -10};

        Mat4c_t V_full = cmath_lookAt(eye, center, (Vec3f_t){0, 1, 0});
        // Full should also have a valid orthonormal basis; check column norms near 1
        float s_norm = sqrtf(V_full.m[0].x * V_full.m[0].x + V_full.m[0].y * V_full.m[0].y + V_full.m[0].z * V_full.m[0].z);
        float u_norm = sqrtf(V_full.m[1].x * V_full.m[1].x + V_full.m[1].y * V_full.m[1].y + V_full.m[1].z * V_full.m[1].z);
        float f_norm = sqrtf(V_full.m[2].x * V_full.m[2].x + V_full.m[2].y * V_full.m[2].y + V_full.m[2].z * V_full.m[2].z);
        fails += ut_assert(fabsf(s_norm - 1.0f) < 1e-3f && fabsf(u_norm - 1.0f) < 1e-3f && fabsf(f_norm - 1.0f) < 1e-3f,
                           "lookAt full basis normalized");
    }
}

// Test
int math_tests_run(void)
{
    test_arithmetic();
    test_quaternion();
    test_matrix();
    return fails;
}