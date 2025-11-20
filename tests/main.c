#include "../src/cmath/cmath.h"
#include "unit_tests.h"

int main(void)
{
    cmath_instantiate();
    return unitTests_run();
    cmath_destroy();
}