#pragma region Includes
#include "../../unit_tests.h"
#include "cmath/cmath.h"
#pragma endregion

static int fails = 0;

static inline int almost_eq(float a, float b, float eps) { return fabsf(a - b) <= eps; }
static const float EPS = 1E-4F;
static const float EPS_BIG = 2E-3F;

static void test_fundamentals(void)
{
    fails += ut_assert(cmath_clampF(2.0F, 0.0F, 1.0F) == 1.0F, "ClampF > max");
    fails += ut_assert(cmath_clampF(1.0F, 0.0F, 1.0F) == 1.0F, "ClampF = max");
    fails += ut_assert(cmath_clampF(-2.0F, 0.0F, 1.0F) == 0.0F, "ClampF < min");
    fails += ut_assert(cmath_clampF(0.0F, 0.0F, 1.0F) == 0.0F, "ClampF = min");

    fails += ut_assert(cmath_clampD(2.0, 0.0, 1.0) == 1.0, "ClampD > max");
    fails += ut_assert(cmath_clampD(1.0, 0.0, 1.0) == 1.0, "ClampD = max");
    fails += ut_assert(cmath_clampD(-2.0, 0.0, 1.0) == 0.0, "ClampD < min");
    fails += ut_assert(cmath_clampD(0.0, 0.0, 1.0) == 0.0, "ClampD = min");

    fails += ut_assert(cmath_clampI(2, 0, 1) == 1, "ClampI > max");
    fails += ut_assert(cmath_clampI(1, 0, 1) == 1, "ClampI = max");
    fails += ut_assert(cmath_clampI(-2, 0, 1) == 0, "ClampI < min");
    fails += ut_assert(cmath_clampI(0, 0, 1) == 0, "ClampI = min");

    fails += ut_assert(cmath_clampU32t(3U, 1U, 2U) == 2U, "ClampU32t > max");
    fails += ut_assert(cmath_clampU32t(2U, 1U, 2U) == 2U, "ClampU32t = max");
    fails += ut_assert(cmath_clampU32t(0U, 1U, 2U) == 1U, "ClampU32t < min");
    fails += ut_assert(cmath_clampU32t(1U, 1U, 2U) == 1U, "ClampU32t = min");
}

static void test_aabb(void)
{
    const int L = CMATH_CHUNK_AXIS_LENGTH;

    Vec3i_t A = VEC3I_ZERO;
    Vec3i_t B = cmath_vec3i_mult_scalar(VEC3I_ONE, L);
    Boundsi_t bounds = (Boundsi_t){A, B};

    Vec3i_t P;

    P = VEC3I_ONE;
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == true, "AABB boundsI: point strictly inside");

    P = VEC3I_ZERO;
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == true, "AABB boundsI: min corner inside");

    P = (Vec3i_t){L, 0, 0};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: point on max X border outside");

    P = (Vec3i_t){0, L, 0};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: point on max Y border outside");

    P = (Vec3i_t){0, 0, L};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: point on max Z border outside");

    P = (Vec3i_t){L - 1, L - 1, L - 1};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == true, "AABB boundsI: point at max-1 inside");

    P = (Vec3i_t){-1, 0, 0};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: X < min outside");

    P = (Vec3i_t){0, -1, 0};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: Y < min outside");

    P = (Vec3i_t){0, 0, -1};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: Z < min outside");

    P = (Vec3i_t){L + 10, L + 10, L + 10};
    fails += ut_assert(cmath_AABB_boundsI(P, bounds) == false, "AABB boundsI: point far outside");

    Vec3i_t A_rev = (Vec3i_t){L, L, L};
    Vec3i_t B_rev = VEC3I_ZERO;

    P = (Vec3i_t){1, 1, 1};
    fails += ut_assert(cmath_AABB_halfOpenI(P, A_rev, B_rev) == true,
                       "AABB_halfOpenI: inside with reversed bounds");

    P = (Vec3i_t){0, 0, 0};
    fails += ut_assert(cmath_AABB_halfOpenI(P, A_rev, B_rev) == true,
                       "AABB_halfOpenI: min corner with reversed bounds");

    P = (Vec3i_t){L, L, L};
    fails += ut_assert(cmath_AABB_halfOpenI(P, A_rev, B_rev) == false,
                       "AABB_halfOpenI: max corner excluded with reversed bounds");

    Vec3i_t Ai = VEC3I_ZERO;
    Vec3i_t Bi = (Vec3i_t){L - 1, L - 1, L - 1};

    P = (Vec3i_t){0, 0, 0};
    fails += ut_assert(cmath_AABB_inclusiveI(P, Ai, Bi) == true,
                       "AABB_inclusiveI: min corner inside");

    P = (Vec3i_t){L - 1, L - 1, L - 1};
    fails += ut_assert(cmath_AABB_inclusiveI(P, Ai, Bi) == true,
                       "AABB_inclusiveI: max corner inside");

    P = (Vec3i_t){L, L - 1, L - 1};
    fails += ut_assert(cmath_AABB_inclusiveI(P, Ai, Bi) == false,
                       "AABB_inclusiveI: X > max outside");

    fails += ut_assert(cmath_AABB_inclusiveI(P, Bi, Ai) == false,
                       "AABB_inclusiveI: X > max outside with reversed bounds");

    P = (Vec3i_t){1, 1, 1};
    fails += ut_assert(cmath_AABB_inclusiveI(P, Bi, Ai) == true,
                       "AABB_inclusiveI: inside with reversed bounds");

    Vec3f_t Af = (Vec3f_t){0.0F, 0.0F, 0.0F};
    Vec3f_t Bf = (Vec3f_t){(float)L, (float)L, (float)L};

    Vec3f_t Pf;

    Pf = (Vec3f_t){1.0F, 1.0F, 1.0F};
    fails += ut_assert(cmath_AABB_halfOpenF(Pf, Af, Bf) == true,
                       "AABB_halfOpenF: point strictly inside");

    Pf = (Vec3f_t){0.0F, 0.0F, 0.0F};
    fails += ut_assert(cmath_AABB_halfOpenF(Pf, Af, Bf) == true,
                       "AABB_halfOpenF: min corner inside");

    Pf = (Vec3f_t){(float)L, 0.0F, 0.0F};
    fails += ut_assert(cmath_AABB_halfOpenF(Pf, Af, Bf) == false,
                       "AABB_halfOpenF: max X border excluded");

    Vec3f_t Bfi = (Vec3f_t){(float)L - 1.0F, (float)L - 1.0F, (float)L - 1.0F};

    Pf = (Vec3f_t){0.0F, 0.0F, 0.0F};
    fails += ut_assert(cmath_AABB_inclusiveF(Pf, Af, Bfi) == true,
                       "AABB_inclusiveF: min corner inside");

    Pf = (Vec3f_t){(float)L - 1.0F, (float)L - 1.0F, (float)L - 1.0F};
    fails += ut_assert(cmath_AABB_inclusiveF(Pf, Af, Bfi) == true,
                       "AABB_inclusiveF: max corner inside");

    Pf = (Vec3f_t){(float)L, (float)L - 1.0F, (float)L - 1.0F};
    fails += ut_assert(cmath_AABB_inclusiveF(Pf, Af, Bfi) == false,
                       "AABB_inclusiveF: past max outside");
}

static void test_quaternion(void)
{
    // Identity -> matrices
    {
        Quaternionf_t q = QUATERNION_IDENTITY;
        Mat4c_t m_full = cmath_quat2mat(q);
        fails += ut_assert(fabsf(m_full.m[0].x - 1.0F) < EPS && fabsf(m_full.m[1].y - 1.0F) < EPS && fabsf(m_full.m[2].z - 1.0F) < EPS,
                           "quat identity to full matrix");
    }

    // Pure yaw 90 deg about +Y. Forward (0,0,1) should go to +X.
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0F), VEC3F_UP);
        Mat4c_t m_full = cmath_quat2mat(q);
        Vec3f_t fwd = (Vec3f_t){0, 0, 1};
        Vec3f_t r_full = cmath_mat_transformByVec3(m_full, fwd);
        fails += ut_assert(fabsf(r_full.x - 1.0F) < EPS_BIG && fabsf(r_full.z) < EPS_BIG,
                           "yaw 90 full rotates forward to +X");
    }

    // Pure pitch 90 deg about +X. Up (0,1,0) should go to +Z.
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0F), VEC3F_RIGHT);
        Mat4c_t m_full = cmath_quat2mat(q);
        Vec3f_t up = (Vec3f_t){0, 1, 0};
        Vec3f_t r_full = cmath_mat_transformByVec3(m_full, up);
        fails += ut_assert(fabsf(r_full.z - 1.0F) < EPS_BIG && fabsf(r_full.y) < EPS_BIG,
                           "pitch 90 full rotates up to +Z");
    }

    // Pure roll 90 deg about +Z
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0F), VEC3F_BACK);
        Mat4c_t m_full = cmath_quat2mat(q);
        Vec3f_t right = (Vec3f_t){1, 0, 0};
        Vec3f_t r_full = cmath_mat_transformByVec3(m_full, right);
        fails += ut_assert(fabsf(r_full.y - 1.0F) < EPS_BIG && fabsf(r_full.x) < EPS_BIG,
                           "roll 90 full rotates +X to +Y");
    }

    // Combined yaw then pitch. Compare quatRotateVec3 vs matrix transform.
    {
        Quaternionf_t qYaw = cmath_quat_fromAxisAngle(cmath_deg2radF(30.0F), VEC3F_UP);
        Quaternionf_t qPitch = cmath_quat_fromAxisAngle(cmath_deg2radF(20.0F), VEC3F_RIGHT);
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
        fails += ut_assert(fabsf(n.qx) < 1e-6f && fabsf(n.qy) < 1e-6f && fabsf(n.qz) < 1e-6f && fabsf(n.qw - 1.0F) < 1e-6f,
                           "quat normalize identity (full)");
    }

    // Inverse: q * q^-1 = identity
    {
        Quaternionf_t q = cmath_quat_fromAxisAngle(cmath_deg2radF(45.0F), VEC3F_RIGHT);
        Quaternionf_t qi = cmath_quat_inverse(q);
        Quaternionf_t id = cmath_quat_mult_quat(q, qi);
        fails += ut_assert(fabsf(id.qx) < EPS && fabsf(id.qy) < EPS && fabsf(id.qz) < EPS && fabsf(id.qw - 1.0F) < EPS,
                           "quat inverse full produces identity");
    }

    // Multiplication non-commutativity
    {
        Quaternionf_t qYaw = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0F), VEC3F_UP);
        Quaternionf_t qPitch = cmath_quat_fromAxisAngle(cmath_deg2radF(90.0F), VEC3F_RIGHT);
        Quaternionf_t a = cmath_quat_mult_quat(qYaw, qPitch);
        Quaternionf_t b = cmath_quat_mult_quat(qPitch, qYaw);
        int diff = (fabsf(a.qx - b.qx) > 1e-4f) || (fabsf(a.qy - b.qy) > 1e-4f) || (fabsf(a.qz - b.qz) > 1e-4f) || (fabsf(a.qw - b.qw) > 1e-4f);
        fails += ut_assert(diff, "quat multiply is non-commutative");
    }

    // SLERP endpoints and mid for full
    {
        Quaternionf_t a = cmath_quat_fromEuler((Vec3f_t){0, 0, 0});
        Quaternionf_t b = cmath_quat_fromEuler((Vec3f_t){cmath_deg2radF(30), cmath_deg2radF(45), cmath_deg2radF(10)});

        Quaternionf_t r0 = cmath_quat_slerp(a, b, 0.0F);
        Quaternionf_t r1 = cmath_quat_slerp(a, b, 1.0F);
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

static void test_matrix(void)
{
    // cmath_mat_rotate yaw 90 about +Y, both full and noRoll
    {
        Mat4c_t r_full = cmath_mat_rotate(MAT4_IDENTITY, cmath_deg2radF(90.0F), VEC3F_UP);
        Vec3f_t fwd = (Vec3f_t){0, 0, 1};
        Vec3f_t a = cmath_mat_transformByVec3(r_full, fwd);
        fails += ut_assert(fabsf(a.x - 1.0F) < EPS_BIG && fabsf(a.z) < EPS_BIG,
                           "matrixRotate full yaw 90");
    }

    // cmath_mat_rotate pitch 90 about +X, both paths
    {
        Mat4c_t r_full = cmath_mat_rotate(MAT4_IDENTITY, cmath_deg2radF(90.0F), VEC3F_RIGHT);
        Vec3f_t up = (Vec3f_t){0, 1, 0};
        Vec3f_t a = cmath_mat_transformByVec3(r_full, up);
        fails += ut_assert(fabsf(a.z - 1.0F) < EPS_BIG && fabsf(a.y) < EPS_BIG,
                           "matrixRotate full pitch 90");
    }

    // cmath_mat_rotate roll 90 about +Z. Full rotates +X to +Y; noRoll ignores.
    {
        Mat4c_t r_full = cmath_mat_rotate(MAT4_IDENTITY, cmath_deg2radF(90.0F), VEC3F_BACK);
        Vec3f_t right = (Vec3f_t){1, 0, 0};
        Vec3f_t a = cmath_mat_transformByVec3(r_full, right);
        fails += ut_assert(fabsf(a.y - 1.0F) < EPS_BIG && fabsf(a.x) < EPS_BIG,
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
        fails += ut_assert(fabsf(s_norm - 1.0F) < 1e-3f && fabsf(u_norm - 1.0F) < 1e-3f && fabsf(f_norm - 1.0F) < 1e-3f,
                           "lookAt full basis normalized");
    }
}

static void test_vector(void)
{
    const int TOLERANCE_I = 0;
    const float TOLERANCE_F = 0.0F;
    fails += ut_assert(cmath_vec3i_equals(VEC3I_ONE, VEC3I_ONE, TOLERANCE_I) == true,
                       "Vec3I Equals (equal)");
    fails += ut_assert(cmath_vec3i_equals(VEC3I_ZERO, VEC3I_ONE, TOLERANCE_I) == false,
                       "Vec3I Equals (not equal)");
    fails += ut_assert(cmath_vec3i_equals(cmath_vec3i_add_scalar(VEC3I_ONE, 4), (Vec3i_t){5, 5, 5}, TOLERANCE_I) == true,
                       "Vec3I Add scalar (5)");
    fails += ut_assert(cmath_vec3i_equals(cmath_vec3i_add_scalar((Vec3i_t){5, 5, 5}, -5), VEC3I_ZERO, TOLERANCE_I) == true,
                       "Vec3I Add scalar (-5)");
    fails += ut_assert(cmath_vec3i_equals(cmath_vec3i_mult_scalar(VEC3I_ONE, 5), (Vec3i_t){5, 5, 5}, TOLERANCE_I) == true,
                       "Vec3I Mult scalar (5)");
    fails += ut_assert(cmath_vec3i_equals(cmath_vec3i_sub_scalar((Vec3i_t){5, 5, 5}, 5), VEC3I_ZERO, TOLERANCE_I) == true,
                       "Vec3I Sub scalar (5)");
    fails += ut_assert(cmath_vec3i_equals(cmath_vec3i_add_vec3i((Vec3i_t){5, 5, 5}, (Vec3i_t){5, 5, 5}), (Vec3i_t){10, 10, 10}, TOLERANCE_I) == true,
                       "Vec3I Add Vec3I");
    fails += ut_assert(cmath_vec3i_equals(cmath_vec3i_sub_vec3i((Vec3i_t){5, 5, 5}, (Vec3i_t){5, 5, 5}), VEC3I_ZERO, TOLERANCE_I) == true,
                       "Vec3I Sub Vec3I");
    fails += ut_assert(cmath_vec3f_isZero(VEC3F_ZERO) == true,
                       "Vec3F isZero");
    fails += ut_assert(cmath_vec3f_equals(VEC3F_ONE, VEC3F_ONE, TOLERANCE_F) == true,
                       "Vec3F equals (equal)");
    fails += ut_assert(cmath_vec3f_equals((cmath_vec3i_to_vec3f(VEC3I_ONE)), VEC3F_ONE, TOLERANCE_F) == true,
                       "Vec3I to Vec3F");
    fails += ut_assert(cmath_vec3f_equals(VEC3F_ZERO, VEC3F_ONE, TOLERANCE_F) == false,
                       "Vec3F equals (not equal)");
    Vec3f_t v_fi = {1.2F, -3.8F, 4.9F};
    Vec3i_t v_i_from_f = cmath_vec3f_to_vec3i(v_fi);
    fails += ut_assert(v_i_from_f.x == (int)v_fi.x && v_i_from_f.y == (int)v_fi.y && v_i_from_f.z == (int)v_fi.z,
                       "Vec3F to Vec3I");
    Vec3u8_t v_u = {255, 0, 128};
    Vec3i_t v_i_from_u = cmath_vec3u8_to_vec3i(v_u);
    fails += ut_assert(v_i_from_u.x == 255 && v_i_from_u.y == 0 && v_i_from_u.z == 128,
                       "Vec3U8 to Vec3I");
    fails += ut_assert(cmath_vec3f_isZero(VEC3F_ONE) == false,
                       "Vec3F isZero (false)");
    fails += ut_assert(cmath_vec3f_equals(VEC3F_ONE, VEC3F_ONE, 0.001F) == true,
                       "Vec3F equals (equal, tol)");
    Vec3f_t vf_a = {1.0F, 2.0F, 3.0F};
    Vec3f_t vf_b = {1.0005f, 1.9995f, 3.0F};
    fails += ut_assert(cmath_vec3f_equals(vf_a, vf_b, 0.001F) == true,
                       "Vec3F equals (within tol)");
    Vec3f_t vf_c = {1.0F, 2.0F, 3.0F};
    Vec3f_t vf_d = {1.1f, 2.0F, 3.0F};
    fails += ut_assert(cmath_vec3f_equals(vf_c, vf_d, 0.05F) == false,
                       "Vec3F equals (outside tol)");
    Vec3f_t vf_e = {1.0F, -2.0F, 0.5F};
    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_mult_scalar(vf_e, 2.0F), (Vec3f_t){2.0F, -4.0F, 1.0F}, 0.0001F) == true,
                       "Vec3F Mult scalar (2)");
    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_add_vec3f((Vec3f_t){1.0F, 2.0F, 3.0F}, (Vec3f_t){-1.0F, 0.5F, 1.0F}), (Vec3f_t){0.0F, 2.5F, 4.0F}, 0.0001F) == true,
                       "Vec3F Add Vec3F");
    fails += ut_assert(fabsf(cmath_vec3f_magnitudeF((Vec3f_t){3.0F, 4.0F, 0.0F}) - 5.0F) < CMATH_EPSILON_F,
                       "Vec3F magnitude (3,4,0)");
    fails += ut_assert(cmath_vec3f_isZero(cmath_vec3f_normalize(VEC3F_ZERO)) == true,
                       "Vec3F normalize (zero)");
    Vec3f_t vf_n = {3.0F, 0.0F, 4.0F};
    Vec3f_t vf_n_norm = cmath_vec3f_normalize(vf_n);
    fails += ut_assert(fabsf(cmath_vec3f_magnitudeF(vf_n_norm) - 1.0F) < 1E-4F,
                       "Vec3F normalize (magnitude 1)");
    fails += ut_assert(vf_n_norm.x > 0.0F && vf_n_norm.y == 0.0F && vf_n_norm.z > 0.0F,
                       "Vec3F normalize (direction)");
    Vec3f_t vl_a = {0.0F, 0.0F, 0.0F};
    Vec3f_t vl_b = {10.0F, 10.0F, 10.0F};
    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_lerpF(vl_a, vl_b, 0.0F),
                                          vl_a, 0.0001f) == true,
                       "Vec3F lerp t=0");
    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_lerpF(vl_a, vl_b, 1.0F),
                                          vl_b, 0.0001f) == true,
                       "Vec3F lerp t=1");
    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_lerpF(vl_a, vl_b, 0.5f),
                                          (Vec3f_t){5.0F, 5.0F, 5.0F},
                                          0.0001f) == true,
                       "Vec3F lerp t=0.5");

    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_lerpF(vl_a, vl_b, -1.0F),
                                          vl_a, 0.0001f) == true,
                       "Vec3F lerp clamp low");
    fails += ut_assert(cmath_vec3f_equals(cmath_vec3f_lerpF(vl_a, vl_b, 2.0F),
                                          vl_b, 0.0001f) == true,
                       "Vec3F lerp clamp high");
}

static bool test_localPointsInChunk(const Vec3u8_t *pPOINTS, size_t size, bool verbose, const char *pName)
{
    if (!pPOINTS || !pName)
        return false;

    Vec3u8_t *pWrongPoints = malloc(sizeof(Vec3u8_t) * size);
    if (!pWrongPoints)
        return false;

    size_t wrongCount = 0;
    for (size_t i = 0; i < size; i++)
    {
        const Vec3u8_t POINT = pPOINTS[i];
        const uint8_t BOUND = (uint8_t)(CMATH_CHUNK_AXIS_LENGTH - 1);
        if (POINT.x > BOUND || POINT.y > BOUND || POINT.z > BOUND)
            pWrongPoints[wrongCount++] = POINT;
    }

    if (wrongCount > 0)
    {
        logs_log(LOG_ERROR, "%s: %zu local points are baked incorrectly! (Outside of chunk)", pName, wrongCount);
        if (verbose)
        {
            for (size_t i = 0; i < wrongCount; i++)
            {
                const Vec3u8_t POINT = pWrongPoints[i];
                logs_log(LOG_ERROR, "Point (%u, %u, %u) is out of chunk bounds!", POINT.x, POINT.y, POINT.z);
            }
        }
    }

    free(pWrongPoints);
    return wrongCount == 0;
}

static bool test_blockPackingAndIndexing(void)
{
    bool seen[CMATH_CHUNK_BLOCK_CAPACITY] = {false};

    for (uint8_t x = 0; x < CMATH_CHUNK_AXIS_LENGTH; ++x)
        for (uint8_t y = 0; y < CMATH_CHUNK_AXIS_LENGTH; ++y)
            for (uint8_t z = 0; z < CMATH_CHUNK_AXIS_LENGTH; ++z)
            {
                const uint16_t packed = blockPos_pack_localXYZ(x, y, z);

                // Unpack by axis
                if (cmath_chunk_blockPosPacked_getLocal_x(packed) != x ||
                    cmath_chunk_blockPosPacked_getLocal_y(packed) != y ||
                    cmath_chunk_blockPosPacked_getLocal_z(packed) != z)
                {
                    logs_log(LOG_ERROR,
                             "blockPacking: axis unpack mismatch for (%u,%u,%u)",
                             x, y, z);
                    return false;
                }

                // Unpack via Vec3u8_t
                const Vec3u8_t local = cmath_chunk_blockPosPacked_2_localPos(packed);
                if (local.x != x || local.y != y || local.z != z)
                {
                    logs_log(LOG_ERROR,
                             "blockPacking: vec unpack mismatch for (%u,%u,%u) -> (%u,%u,%u)",
                             x, y, z, local.x, local.y, local.z);
                    return false;
                }

                // Index mapping
                const uint16_t idx_xyz = xyz_to_chunkBlockIndex(x, y, z);
                const uint16_t idx_packed = cmath_chunk_blockPosPacked_2_chunkBlockIndex(packed);

                if (idx_xyz != idx_packed)
                {
                    logs_log(LOG_ERROR,
                             "blockPacking: index mismatch for (%u,%u,%u): xyz=%u packed=%u",
                             x, y, z, idx_xyz, idx_packed);
                    return false;
                }

                if (idx_xyz >= CMATH_CHUNK_BLOCK_CAPACITY)
                {
                    logs_log(LOG_ERROR,
                             "blockPacking: index out of range for (%u,%u,%u): %u",
                             x, y, z, idx_xyz);
                    return false;
                }

                if (seen[idx_xyz])
                {
                    logs_log(LOG_ERROR,
                             "blockPacking: duplicate index for (%u,%u,%u): %u",
                             x, y, z, idx_xyz);
                    return false;
                }

                seen[idx_xyz] = true;
            }

    // Ensure every index 0..capacity-1 is hit exactly once
    for (size_t i = 0; i < CMATH_CHUNK_BLOCK_CAPACITY; ++i)
    {
        if (!seen[i])
        {
            logs_log(LOG_ERROR,
                     "blockPacking: index %zu was never produced", i);
            return false;
        }
    }

    return true;
}

static bool test_chunkNeighborPos_single(void)
{
    const Vec3i_t CHUNK_POS = {0, 0, 0};
    Vec3i_t *pNeighbors = cmath_chunk_chunkNeighborPos_get(CHUNK_POS);
    if (!pNeighbors)
        return false;

    bool ok = true;
    for (int face = 0; face < CMATH_GEOM_CUBE_FACES; ++face)
    {
        const Vec3i_t expected = {
            CHUNK_POS.x + pCMATH_CUBE_NEIGHBOR_OFFSETS[face].x,
            CHUNK_POS.y + pCMATH_CUBE_NEIGHBOR_OFFSETS[face].y,
            CHUNK_POS.z + pCMATH_CUBE_NEIGHBOR_OFFSETS[face].z};

        if (!cmath_vec3i_equals(pNeighbors[face], expected, 0))
        {
            logs_log(LOG_ERROR,
                     "chunkNeighborPos: face %d mismatch. Got (%d,%d,%d) expected (%d,%d,%d)",
                     face,
                     pNeighbors[face].x, pNeighbors[face].y, pNeighbors[face].z,
                     expected.x, expected.y, expected.z);
            ok = false;
            break;
        }
    }

    free(pNeighbors);
    return ok;
}

static bool test_chunkNeighborsUnique_single(void)
{
    const Vec3i_t CHUNK_POS = {0, 0, 0};

    size_t neighborsCount = 0;
    Vec3i_t *pNeighbors = cmath_chunk_chunkNeighborPos_get(CHUNK_POS);
    if (!pNeighbors)
        return false;

    size_t uniqueCount = 0;
    Vec3i_t *pUnique = cmath_chunk_GetNeighborsPosUnique_get(&CHUNK_POS, 1, &uniqueCount);
    bool ok = true;

    if (!pUnique)
    {
        free(pNeighbors);
        return false;
    }

    // Should not include original chunk
    for (size_t i = 0; i < uniqueCount; ++i)
    {
        if (cmath_vec3i_equals(pUnique[i], CHUNK_POS, 0))
        {
            logs_log(LOG_ERROR, "chunkNeighborsUnique: contains original chunk position");
            ok = false;
            break;
        }
    }

    // Should have one entry per face (6 neighbors) for a single chunk
    if (ok && uniqueCount != CMATH_GEOM_CUBE_FACES)
    {
        logs_log(LOG_ERROR,
                 "chunkNeighborsUnique: expected %d neighbors, got %zu",
                 CMATH_GEOM_CUBE_FACES, uniqueCount);
        ok = false;
    }

    // Unique set should match the neighbor set (order may differ)
    if (ok)
    {
        neighborsCount = CMATH_GEOM_CUBE_FACES;
        for (size_t i = 0; i < neighborsCount; ++i)
        {
            bool found = false;
            for (size_t j = 0; j < uniqueCount; ++j)
            {
                if (cmath_vec3i_equals(pNeighbors[i], pUnique[j], 0))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                logs_log(LOG_ERROR,
                         "chunkNeighborsUnique: neighbor (%d,%d,%d) missing from unique set",
                         pNeighbors[i].x, pNeighbors[i].y, pNeighbors[i].z);
                ok = false;
                break;
            }
        }
    }

    free(pNeighbors);
    free(pUnique);
    return ok;
}

static bool test_worldIndexConsistency(void)
{
    // For integer coordinates, the float and int versions should agree
    for (int x = -64; x <= 64; ++x)
    {
        const int idx_int = cmath_chunk_floorDivChunkAxis(x);
        const int idx_float = cmath_chunk_indexFromWorldF((float)x);

        if (idx_int != idx_float)
        {
            logs_log(LOG_ERROR,
                     "worldIndexConsistency: mismatch for %d -> int=%d float=%d",
                     x, idx_int, idx_float);
            return false;
        }
    }

    return true;
}

static void test_chunk(void)
{
    const bool VERBOSE = true;
    // Local pos checks
    fails += ut_assert(test_localPointsInChunk(cmath_chunk_blockNeighborPoints_Get(), CMATH_CHUNK_BLOCK_NEIGHBOR_POINTS_COUNT, VERBOSE, "pCMATH_CHUNK_BLOCK_NEIGHBOR_POINTS") == true,
                       "CMATH_CHUNK_BLOCK_NEIGHBOR_POINTS");
    fails += ut_assert(test_localPointsInChunk(cmath_chunkCornerPoints_Get(), CMATH_CHUNK_CORNER_POINTs_COUNT, VERBOSE, "pCMATH_CHUNK_CORNER_POINTS") == true,
                       "CMATH_CHUNK_CORNER_POINTS");
    fails += ut_assert(test_localPointsInChunk(cmath_chunkInnerPoints_Get(), CMATH_CHUNK_INNER_POINTS_COUNT, VERBOSE, "pCMATH_CHUNK_INNER_POINTS") == true,
                       "CMATH_CHUNK_INNER_POINTS");
    fails += ut_assert(test_localPointsInChunk(cmath_chunkPoints_Get(), CMATH_CHUNK_POINTS_COUNT, VERBOSE, "pCMATH_CHUNK_POINTS") == true,
                       "CMATH_CHUNK_POINTS");
    fails += ut_assert(test_localPointsInChunk(cmath_chunkShellBorderlessPoints_Get(), CMATH_CHUNK_SHELL_BORDERLESS_POINTS_COUNT, VERBOSE, "pCMATH_CHUNK_SHELL_BORDERLESS_POINTS") == true,
                       "CMATH_CHUNK_SHELL_BORDERLESS_POINTS");
    fails += ut_assert(test_localPointsInChunk(cmath_chunkShellEdgePoints_Get(), CMATH_CHUNK_SHELL_EDGE_POINTS_COUNT, VERBOSE, "pCMATH_CHUNK_SHELL_EDGE_POINTS") == true,
                       "CMATH_CHUNK_SHELL_EDGE_POINTS");

    fails += ut_assert(CMATH_CHUNK_BLOCK_CAPACITY == CMATH_CHUNK_AXIS_LENGTH * CMATH_CHUNK_AXIS_LENGTH * CMATH_CHUNK_AXIS_LENGTH,
                       "Chunk block capacity matches axis^3");

    // Block packing and indexing
    fails += ut_assert(test_blockPackingAndIndexing() == true,
                       "Chunk block packing and indexing");

    // Simple spot-checks for xyz_to_chunkBlockIndex
    fails += ut_assert(xyz_to_chunkBlockIndex(0, 0, 0) == 0,
                       "xyz_to_chunkBlockIndex (0,0,0)");
    fails += ut_assert(xyz_to_chunkBlockIndex(0, 0, 1) == 1,
                       "xyz_to_chunkBlockIndex (0,0,1)");
    fails += ut_assert(xyz_to_chunkBlockIndex(0, 1, 0) == CMATH_CHUNK_AXIS_LENGTH,
                       "xyz_to_chunkBlockIndex (0,1,0)");
    fails += ut_assert(xyz_to_chunkBlockIndex(1, 0, 0) == (CMATH_CHUNK_AXIS_LENGTH * CMATH_CHUNK_AXIS_LENGTH),
                       "xyz_to_chunkBlockIndex (1,0,0)");

    // Neighbor positions
    fails += ut_assert(test_chunkNeighborPos_single() == true,
                       "Chunk neighbor positions (single chunk)");
    fails += ut_assert(test_chunkNeighborsUnique_single() == true,
                       "Chunk neighbors unique (single chunk)");

    // Wrap local positions
    {
        const uint8_t BOUND = (uint8_t)(CMATH_CHUNK_AXIS_LENGTH - 1);

        Vec3u8_t w = cmath_chunk_wrapLocalPos(CMATH_CHUNK_AXIS_LENGTH, 5, 6, CUBE_FACE_RIGHT);
        fails += ut_assert(w.x == 0 && w.y == 5 && w.z == 6,
                           "Chunk wrapLocalPos RIGHT");

        w = cmath_chunk_wrapLocalPos(0, 5, 6, CUBE_FACE_LEFT);
        fails += ut_assert(w.x == BOUND && w.y == 5 && w.z == 6,
                           "Chunk wrapLocalPos LEFT");

        w = cmath_chunk_wrapLocalPos(3, CMATH_CHUNK_AXIS_LENGTH, 7, CUBE_FACE_TOP);
        fails += ut_assert(w.x == 3 && w.y == 0 && w.z == 7,
                           "Chunk wrapLocalPos TOP");

        w = cmath_chunk_wrapLocalPos(3, 0, 7, CUBE_FACE_BOTTOM);
        fails += ut_assert(w.x == 3 && w.y == BOUND && w.z == 7,
                           "Chunk wrapLocalPos BOTTOM");

        w = cmath_chunk_wrapLocalPos(8, 9, CMATH_CHUNK_AXIS_LENGTH, CUBE_FACE_FRONT);
        fails += ut_assert(w.x == 8 && w.y == 9 && w.z == 0,
                           "Chunk wrapLocalPos FRONT");

        w = cmath_chunk_wrapLocalPos(8, 9, 0, CUBE_FACE_BACK);
        fails += ut_assert(w.x == 8 && w.y == 9 && w.z == BOUND,
                           "Chunk wrapLocalPos BACK");
    }

    // floorDiv behavior
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(0) == 0, "floorDiv 0");
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(15) == 0, "floorDiv 15");
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(16) == 1, "floorDiv 16");
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(17) == 1, "floorDiv 17");
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(-1) == -1, "floorDiv -1");
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(-16) == -1, "floorDiv -16");
    fails += ut_assert(cmath_chunk_floorDivChunkAxis(-17) == -2, "floorDiv -17");

    // Int world/chunk conversions
    {
        Vec3i_t cp = {0, 0, 0};
        Vec3i_t wp = cmath_chunk_chunkPos_2_worldPosI(cp);
        fails += ut_assert(cmath_vec3i_equals(wp, (Vec3i_t){0, 0, 0}, 0) == true,
                           "chunkPos_2_worldPosI (0,0,0)");

        Vec3i_t cp2 = {1, 2, 3};
        Vec3i_t wp2 = cmath_chunk_chunkPos_2_worldPosI(cp2);
        fails += ut_assert(cmath_vec3i_equals(wp2, (Vec3i_t){16, 32, 48}, 0) == true,
                           "chunkPos_2_worldPosI (1,2,3)");

        Vec3i_t cp3 = {-1, 0, 2};
        Vec3i_t wp3 = cmath_chunk_chunkPos_2_worldPosI(cp3);
        fails += ut_assert(cmath_vec3i_equals(wp3, (Vec3i_t){-16, 0, 32}, 0) == true,
                           "chunkPos_2_worldPosI (-1,0,2)");

        // world -> chunk for various boundaries
        fails += ut_assert(cmath_worldPosI_2_chunkPos((Vec3i_t){0, 0, 0}).x == 0,
                           "worldPosI_2_chunkPos 0");
        fails += ut_assert(cmath_worldPosI_2_chunkPos((Vec3i_t){15, 0, 0}).x == 0,
                           "worldPosI_2_chunkPos 15");
        fails += ut_assert(cmath_worldPosI_2_chunkPos((Vec3i_t){16, 0, 0}).x == 1,
                           "worldPosI_2_chunkPos 16");
        fails += ut_assert(cmath_worldPosI_2_chunkPos((Vec3i_t){-1, 0, 0}).x == -1,
                           "worldPosI_2_chunkPos -1");
        fails += ut_assert(cmath_worldPosI_2_chunkPos((Vec3i_t){-16, 0, 0}).x == -1,
                           "worldPosI_2_chunkPos -16");
        fails += ut_assert(cmath_worldPosI_2_chunkPos((Vec3i_t){-17, 0, 0}).x == -2,
                           "worldPosI_2_chunkPos -17");
    }

    // Float index consistency with int floorDiv
    fails += ut_assert(test_worldIndexConsistency() == true,
                       "world index int/float consistency");

    // Float world -> chunk mapping edge cases
    {
        Vec3i_t c;

        c = cmath_chunk_worldPosF_2_chunkPos((Vec3f_t){0.0f, 0.0f, 0.0f});
        fails += ut_assert(c.x == 0 && c.y == 0 && c.z == 0,
                           "worldPosF_2_chunkPos 0.0");

        c = cmath_chunk_worldPosF_2_chunkPos((Vec3f_t){15.9f, 0.0f, 0.0f});
        fails += ut_assert(c.x == 0,
                           "worldPosF_2_chunkPos 15.9");

        c = cmath_chunk_worldPosF_2_chunkPos((Vec3f_t){16.0f, 0.0f, 0.0f});
        fails += ut_assert(c.x == 1,
                           "worldPosF_2_chunkPos 16.0");

        c = cmath_chunk_worldPosF_2_chunkPos((Vec3f_t){-0.1f, 0.0f, 0.0f});
        fails += ut_assert(c.x == -1,
                           "worldPosF_2_chunkPos -0.1");

        c = cmath_chunk_worldPosF_2_chunkPos((Vec3f_t){-16.0f, 0.0f, 0.0f});
        fails += ut_assert(c.x == -1,
                           "worldPosF_2_chunkPos -16.0");

        c = cmath_chunk_worldPosF_2_chunkPos((Vec3f_t){-16.1f, 0.0f, 0.0f});
        fails += ut_assert(c.x == -2,
                           "worldPosF_2_chunkPos -16.1");
    }

    // blockPosPacked -> worldPos and worldSamplePos
    {
        const Vec3i_t CHUNK_POS = {1, -1, 2};
        const uint8_t lx = 3, ly = 4, lz = 5;
        const uint16_t packed = blockPos_pack_localXYZ(lx, ly, lz);

        const Vec3i_t worldBlockPos = blockPosPacked_get_worldPos(CHUNK_POS, packed);
        // Origin = (16, -16, 32), so world = (19, -12, 37)
        fails += ut_assert(cmath_vec3i_equals(worldBlockPos, (Vec3i_t){19, -12, 37}, 0) == true,
                           "blockPosPacked_get_worldPos");

        const Vec3f_t worldSamplePos = cmath_chunk_blockPosPacked_2_worldSamplePos(CHUNK_POS, packed);
        fails += ut_assert(cmath_vec3f_equals(worldSamplePos,
                                              (Vec3f_t){19.5f, -11.5f, 37.5f},
                                              0.0001f) == true,
                           "blockPosPacked_2_worldSamplePos");
    }

    // Chunk bounds
    {
        Boundsi_t b0 = cmath_chunk_getBoundsi((Vec3i_t){0, 0, 0});
        fails += ut_assert(cmath_vec3i_equals(b0.A, VEC3I_ZERO, 0) == true,
                           "chunk_getBoundsi origin A");
        fails += ut_assert(cmath_vec3i_equals(b0.B, (Vec3i_t){16, 16, 16}, 0) == true,
                           "chunk_getBoundsi origin B");

        Vec3i_t cp = {1, 2, 3};
        Boundsi_t b1 = cmath_chunk_getBoundsi(cp);
        Vec3i_t expectedA = {cp.x * CMATH_CHUNK_AXIS_LENGTH,
                             cp.y * CMATH_CHUNK_AXIS_LENGTH,
                             cp.z * CMATH_CHUNK_AXIS_LENGTH};
        Vec3i_t expectedB = {expectedA.x + CMATH_CHUNK_AXIS_LENGTH,
                             expectedA.y + CMATH_CHUNK_AXIS_LENGTH,
                             expectedA.z + CMATH_CHUNK_AXIS_LENGTH};

        fails += ut_assert(cmath_vec3i_equals(b1.A, expectedA, 0) == true,
                           "chunk_getBoundsi A");
        fails += ut_assert(cmath_vec3i_equals(b1.B, expectedB, 0) == true,
                           "chunk_getBoundsi B");
    }
}

static bool test_cubicShellNoBorder_basic(void)
{
    const Vec3i_t ORIGIN = {0, 0, 0};
    size_t size = 0;

    // radius 0 -> currently expected: empty (COUNT == 0 -> NULL, size 0)
    Vec3i_t *pShell0 = cmath_algo_cubicShellNoBorder(ORIGIN, 0, &size);
    if (pShell0 != NULL || size != 0)
    {
        logs_log(LOG_ERROR,
                 "cubicShellNoBorder: radius 0 expected NULL and size 0, got ptr=%p size=%zu",
                 (void *)pShell0, size);
        free(pShell0);
        return false;
    }

    // radius 1 -> expect 12 edge points (no corners)
    size = 0;
    Vec3i_t *pShell1 = cmath_algo_cubicShellNoBorder(ORIGIN, 1, &size);
    if (!pShell1)
    {
        logs_log(LOG_ERROR, "cubicShellNoBorder: radius 1 returned NULL");
        return false;
    }

    const size_t EXPECT_COUNT = 12;
    if (size != EXPECT_COUNT)
    {
        logs_log(LOG_ERROR,
                 "cubicShellNoBorder: radius 1 size mismatch. Got %zu, expected %zu",
                 size, EXPECT_COUNT);
        free(pShell1);
        return false;
    }

    const int MIN = -1;
    const int MAX = 1;

    // Validate each point is in [-1, 1]^3 and lies on an edge but not a corner
    for (size_t i = 0; i < size; ++i)
    {
        Vec3i_t p = pShell1[i];
        if (p.x < MIN || p.x > MAX ||
            p.y < MIN || p.y > MAX ||
            p.z < MIN || p.z > MAX)
        {
            logs_log(LOG_ERROR,
                     "cubicShellNoBorder: point %zu (%d,%d,%d) out of bounds [-1,1]",
                     i, p.x, p.y, p.z);
            free(pShell1);
            return false;
        }

        int atExtreme = 0;
        if (p.x == MIN || p.x == MAX)
            atExtreme++;
        if (p.y == MIN || p.y == MAX)
            atExtreme++;
        if (p.z == MIN || p.z == MAX)
            atExtreme++;

        // Edge point -> exactly two coords at extremes, no corners (3) or interior (1/0)
        if (atExtreme != 2)
        {
            logs_log(LOG_ERROR,
                     "cubicShellNoBorder: point %zu (%d,%d,%d) is not an edge point (extremes=%d)",
                     i, p.x, p.y, p.z, atExtreme);
            free(pShell1);
            return false;
        }
    }

    // Check for duplicates
    for (size_t i = 0; i < size; ++i)
        for (size_t j = i + 1; j < size; ++j)
            if (cmath_vec3i_equals(pShell1[i], pShell1[j], 0))
            {
                logs_log(LOG_ERROR,
                         "cubicShellNoBorder: duplicate points at %zu and %zu (%d,%d,%d)",
                         i, j,
                         pShell1[i].x, pShell1[i].y, pShell1[i].z);
                free(pShell1);
                return false;
            }

    free(pShell1);
    return true;
}

static bool test_cubicShell_basic(void)
{
    const Vec3i_t ORIGIN = {0, 0, 0};

    // radius 0 -> single point at origin
    size_t size0 = 0;
    Vec3i_t *pShell0 = cmath_algo_cubicShell(ORIGIN, 0, &size0);
    if (!pShell0)
    {
        logs_log(LOG_ERROR, "cubicShell: radius 0 returned NULL");
        return false;
    }
    if (size0 != 1)
    {
        logs_log(LOG_ERROR,
                 "cubicShell: radius 0 size mismatch. Got %zu, expected 1",
                 size0);
        free(pShell0);
        return false;
    }
    if (!cmath_vec3i_equals(pShell0[0], ORIGIN, 0))
    {
        logs_log(LOG_ERROR,
                 "cubicShell: radius 0 point mismatch. Got (%d,%d,%d), expected origin",
                 pShell0[0].x, pShell0[0].y, pShell0[0].z);
        free(pShell0);
        return false;
    }
    free(pShell0);

    // radius 1 -> shell of [-1,1]^3 minus (0,0,0). Expected count 24R^2+2 = 26
    size_t size1 = 0;
    Vec3i_t *pShell1 = cmath_algo_cubicShell(ORIGIN, 1, &size1);
    if (!pShell1)
    {
        logs_log(LOG_ERROR, "cubicShell: radius 1 returned NULL");
        return false;
    }

    const size_t R1 = 1;
    // 26
    const size_t EXPECT_COUNT_1 = 24ULL * R1 * R1 + 2ULL;
    if (size1 != EXPECT_COUNT_1)
    {
        logs_log(LOG_ERROR,
                 "cubicShell: radius 1 size mismatch. Got %zu, expected %zu",
                 size1, EXPECT_COUNT_1);
        free(pShell1);
        return false;
    }

    const int MIN = -1;
    const int MAX = 1;

    // Validate shell properties
    for (size_t i = 0; i < size1; ++i)
    {
        Vec3i_t p = pShell1[i];

        // In cube bounds
        if (p.x < MIN || p.x > MAX ||
            p.y < MIN || p.y > MAX ||
            p.z < MIN || p.z > MAX)
        {
            logs_log(LOG_ERROR,
                     "cubicShell: radius 1 point %zu (%d,%d,%d) out of bounds [-1,1]",
                     i, p.x, p.y, p.z);
            free(pShell1);
            return false;
        }

        // On surface: at least one coord at ±1
        if (!(p.x == MIN || p.x == MAX ||
              p.y == MIN || p.y == MAX ||
              p.z == MIN || p.z == MAX))
        {
            logs_log(LOG_ERROR,
                     "cubicShell: radius 1 point %zu (%d,%d,%d) not on surface",
                     i, p.x, p.y, p.z);
            free(pShell1);
            return false;
        }

        // Origin must not be included
        if (cmath_vec3i_equals(p, ORIGIN, 0))
        {
            logs_log(LOG_ERROR,
                     "cubicShell: radius 1 includes origin at index %zu",
                     i);
            free(pShell1);
            return false;
        }
    }

    // Check for duplicates
    for (size_t i = 0; i < size1; ++i)
        for (size_t j = i + 1; j < size1; ++j)
            if (cmath_vec3i_equals(pShell1[i], pShell1[j], 0))
            {
                logs_log(LOG_ERROR,
                         "cubicShell: radius 1 duplicate at %zu and %zu (%d,%d,%d)",
                         i, j,
                         pShell1[i].x, pShell1[i].y, pShell1[i].z);
                free(pShell1);
                return false;
            }

    free(pShell1);

    // radius 2 -> check count only (24R^2+2)
    size_t size2 = 0;
    Vec3i_t *pShell2 = cmath_algo_cubicShell(ORIGIN, 2, &size2);
    if (!pShell2)
    {
        logs_log(LOG_ERROR, "cubicShell: radius 2 returned NULL");
        return false;
    }

    const size_t R2 = 2;
    const size_t EXPECT_COUNT_2 = 24ULL * R2 * R2 + 2ULL;
    if (size2 != EXPECT_COUNT_2)
    {
        logs_log(LOG_ERROR,
                 "cubicShell: radius 2 size mismatch. Got %zu, expected %zu",
                 size2, EXPECT_COUNT_2);
        free(pShell2);
        return false;
    }

    free(pShell2);
    return true;
}

static bool test_expandingCubicShell_radius0(void)
{
    const Vec3i_t ORIGIN = {0, 0, 0};
    size_t size = 0;

    Vec3i_t *p = cmath_algo_expandingCubicShell(ORIGIN, 0, &size);
    if (!p)
    {
        logs_log(LOG_ERROR, "expandingCubicShell: radius 0 returned NULL");
        return false;
    }

    if (size != 1)
    {
        logs_log(LOG_ERROR,
                 "expandingCubicShell: radius 0 size mismatch. Got %zu, expected 1",
                 size);
        free(p);
        return false;
    }

    if (!cmath_vec3i_equals(p[0], ORIGIN, 0))
    {
        logs_log(LOG_ERROR,
                 "expandingCubicShell: radius 0 point mismatch. Got (%d,%d,%d), expected origin",
                 p[0].x, p[0].y, p[0].z);
        free(p);
        return false;
    }

    free(p);
    return true;
}

static bool test_expandingCubicShell_radius1_fullCube(void)
{
    const Vec3i_t ORIGIN = {0, 0, 0};
    size_t size = 0;

    Vec3i_t *p = cmath_algo_expandingCubicShell(ORIGIN, 1, &size);
    if (!p)
    {
        logs_log(LOG_ERROR, "expandingCubicShell: radius 1 returned NULL");
        return false;
    }

    const size_t R = 1;
    // 27
    const size_t EXPECT_COUNT = (R * 2ULL + 1ULL) * (R * 2ULL + 1ULL) * (R * 2ULL + 1ULL);
    if (size != EXPECT_COUNT)
    {
        logs_log(LOG_ERROR,
                 "expandingCubicShell: radius 1 size mismatch. Got %zu, expected %zu",
                 size, EXPECT_COUNT);
        free(p);
        return false;
    }

    const int MIN = -1;
    const int MAX = 1;

    // Coverage + uniqueness using a 3x3x3 occupancy grid
    bool seen[3][3][3] = {{{false}}};

    for (size_t i = 0; i < size; ++i)
    {
        Vec3i_t v = p[i];

        if (v.x < MIN || v.x > MAX ||
            v.y < MIN || v.y > MAX ||
            v.z < MIN || v.z > MAX)
        {
            logs_log(LOG_ERROR,
                     "expandingCubicShell: radius 1 point %zu (%d,%d,%d) out of bounds [-1,1]",
                     i, v.x, v.y, v.z);
            free(p);
            return false;
        }

        int ix = v.x - MIN; // 0..2
        int iy = v.y - MIN;
        int iz = v.z - MIN;

        if (seen[ix][iy][iz])
        {
            logs_log(LOG_ERROR,
                     "expandingCubicShell: radius 1 duplicate at (%d,%d,%d)",
                     v.x, v.y, v.z);
            free(p);
            return false;
        }

        seen[ix][iy][iz] = true;
    }

    // Ensure every coordinate in [-1,1]^3 is present exactly once
    for (int x = MIN; x <= MAX; ++x)
        for (int y = MIN; y <= MAX; ++y)
            for (int z = MIN; z <= MAX; ++z)
            {
                int ix = x - MIN;
                int iy = y - MIN;
                int iz = z - MIN;
                if (!seen[ix][iy][iz])
                {
                    logs_log(LOG_ERROR,
                             "expandingCubicShell: radius 1 missing point (%d,%d,%d)",
                             x, y, z);
                    free(p);
                    return false;
                }
            }

    free(p);
    return true;
}

static bool test_expandingCubicShell_radius2_countOnly(void)
{
    const Vec3i_t ORIGIN = {0, 0, 0};
    size_t size = 0;

    Vec3i_t *p = cmath_algo_expandingCubicShell(ORIGIN, 2, &size);
    if (!p)
    {
        logs_log(LOG_ERROR, "expandingCubicShell: radius 2 returned NULL");
        return false;
    }

    const size_t R = 2;
    // 5^3 = 125
    const size_t EXPECT_COUNT = (R * 2ULL + 1ULL) * (R * 2ULL + 1ULL) * (R * 2ULL + 1ULL);
    if (size != EXPECT_COUNT)
    {
        logs_log(LOG_ERROR,
                 "expandingCubicShell: radius 2 size mismatch. Got %zu, expected %zu",
                 size, EXPECT_COUNT);
        free(p);
        return false;
    }

    free(p);
    return true;
}

static void test_algorithms(void)
{
    fails += ut_assert(test_cubicShellNoBorder_basic() == true,
                       "cmath_algo_cubicShellNoBorder basic");
    fails += ut_assert(test_cubicShell_basic() == true,
                       "cmath_algo_cubicShell basic");
    fails += ut_assert(test_expandingCubicShell_radius0() == true,
                       "cmath_algo_expandingCubicShell radius 0");
    fails += ut_assert(test_expandingCubicShell_radius1_fullCube() == true,
                       "cmath_algo_expandingCubicShell radius 1 full cube");
    fails += ut_assert(test_expandingCubicShell_radius2_countOnly() == true,
                       "cmath_algo_expandingCubicShell radius 2 count only");
}

int math_tests_run(void)
{
    test_fundamentals();
    test_aabb();
    test_quaternion();
    test_matrix();
    test_vector();
    test_chunk();
    test_algorithms();
    return fails;
}