#include "unit_tests.h"
#include "../src/c_math/c_math.h"

int unitTests_run(void)
{
    int fails = 0;

    ut_section("Math Tests");
    fails += ut_assert(cm_vec3fMagnitude((Vec3f_t){1, 0, 0}) == 1.0f, "Vec3 length (1,0,0)");
    fails += ut_assert(cm_vec3fMagnitude((Vec3f_t){0, 3, 4}) == 5.0f, "Vec3 length (0,3,4)");

    ut_section("Quaternion Tests");
    Quaternion_t q = cm_quatFromAxisAngle(0, (Vec3f_t){1, 0, 0});
    fails += ut_assert(q.qw == 1.0f, "Identity quaternion from 0 deg");

    if (fails == 0)
        printf("\nAll tests passed!\n");
    else
        printf("\n%d test(s) failed!\n", fails);

    return fails;
}
