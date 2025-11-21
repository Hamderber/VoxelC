#include "unit_tests.h"
#include "../src/cmath/cmath.h"
#include "modules/math/math_tests.h"
#include "modules/random/random_tests.h"
#include "modules/collections/linkedList_tests.h"
#include "modules/collections/dynamicStack_tests.h"
#include "modules/collections/flags64_tests.h"
#include "modules/chunk/chunk_tests.h"
#include "modules/chunk/chunkState_tests.h"
#include "modules/chunk/chunkAPI_tests.h"
#include "modules/events/event_tests.h"
#include "modules/voxel/voxel_tests.h"

int unitTests_run(void)
{
    // Make logs.c skip the timestamping stuff
    logs_toggleSimple(true);

    int fails = 0;

    ut_section("CMath Tests");
    fails += math_tests_run();

    ut_section("Deterministic Random Tests");
    fails += random_tests_run();

    ut_section("Linked List Tests");
    fails += linkedLists_tests_run();

    ut_section("Dynamic Stack Tests");
    fails += dynamicStack_tests_run();

    ut_section("Flags64 Tests");
    fails += flags64_tests_run();

    ut_section("Event Tests");
    fails += event_tests_run();

    ut_section("Chunk Tests");
    fails += chunk_tests_run();
    fails += chunkState_tests_run();
    fails += chunkAPI_tests_run();

    ut_section("Voxel Tests");
    fails += voxel_tests_run();

    if (fails == 0)
        printf("\nAll tests passed!\n");
    else
        printf("\n%d test(s) failed!\n", fails);

    return fails;
}
