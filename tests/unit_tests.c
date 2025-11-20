#include "unit_tests.h"
#include "../src/cmath/cmath.h"
#include "modules/math/math_tests.h"
#include "modules/random_tests.h"
#include "modules/event_tests.h"
// #include "modules/chunk_tests.h"

int unitTests_run(void)
{
    int fails = 0;

    ut_section("CMath Tests");
    fails += math_tests_run();

    ut_section("Deterministic Random Tests");
    fails += random_tests_run();

    ut_section("Event Tests");
    fails += event_tests_run();

    // ut_section("Chunk Tests");
    // fails += chunk_tests_run();

    if (fails == 0)
        printf("\nAll tests passed!\n");
    else
        printf("\n%d test(s) failed!\n", fails);

    return fails;
}
