#include "unit_tests.h"
#include "../src/cmath/cmath.h"

int main(void)
{
    cmath_instantiate();

    int fails = unitTests_run();

    cmath_destroy();

    return fails;
}