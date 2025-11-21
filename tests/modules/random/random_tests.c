#include "../../unit_tests.h"
#include <stdint.h>
#include "cmath/cmath.h"
#include "core/random.h"

// Good midpoint for monte carlo simulation count
static const int NUM_TESTS = 5000;
static int fails = 0;

// Adapt the specific values output from the rand_ranges to all be the same type (for *func use)
static int64_t rand_range7bit_adapt(int n) { return (int64_t)random_range7bit(n); }
static int64_t rand_range15bit_adapt(int n) { return (int64_t)random_range15bit(n); }
static int64_t rand_rangeI32_adapt(int n) { return (int64_t)random_rangeI32(-n, n); }
static int64_t rand_rangeU32_adapt(int n)
{
    // recenter mean to 0
    int32_t r = (int32_t)random_rangeU32(0, (uint32_t)n);
    return (int64_t)(r - (n / 2));
}

static double rand_rangeF32_adapt(double n)
{
    if (n == 0)
        return 0.0;
    return (double)random_rangeF32(-1.0F * (float)n, (float)n);
}

static double rand_rangeF64_adapt(double n)
{
    if (n == 0)
        return 0.0;
    return random_rangeD64(-1.0 * n, n);
}

static double mean_window_ksigma(int numBits, int N, double k)
{
    // M = 2^(n-1) - 1
    double M = (double)((1ULL << (numBits - 1)) - 1ULL);
    double sigma_mean = M / sqrt(3.0 * (double)N);
    return k * sigma_mean;
}

static double mean_window_ksigma_range(double min, double max, int N, double k)
{
    double a = (max - min) / 2.0;
    double sigma_mean = a / sqrt(3.0 * (double)N);
    return k * sigma_mean;
}

static void ut_randSpecific64(int64_t (*func)(int), int range)
{
    const int N = NUM_TESTS;
    long double sum = 0.0L;
    long double sumsq = 0.0L;
    int64_t min = INT64_MAX;
    int64_t max = INT64_MIN;

    for (int i = 0; i < N; ++i)
    {
        int64_t r = func(range);
        sum += (long double)r;
        sumsq += (long double)r * (long double)r;
        if (r < min)
            min = r;
        if (r > max)
            max = r;
    }

    long double mean = sum / N;
    long double var = (sumsq / N) - (mean * mean);
    if (var < 0)
        var = 0; // clamp small negatives
    long double std = sqrtl(var);

    double tolerance = mean_window_ksigma(range, NUM_TESTS, 5.0);

    tolerance = tolerance < 0.25 ? 0.25 : tolerance;

    fails += ut_assert(fabsl(mean) <= tolerance, "Mean expected value tolerance");
}

static void ut_randSpecific_int(int64_t (*func)(int), int range)
{
    const int N = NUM_TESTS;
    long double sum = 0.0L, sumsq = 0.0L;
    int64_t min = INT64_MAX, max = INT64_MIN;

    for (int i = 0; i < N; ++i)
    {
        int64_t r = func(range);
        sum += (long double)r;
        sumsq += (long double)r * (long double)r;
        if (r < min)
            min = r;
        if (r > max)
            max = r;
    }

    long double mean = sum / N;
    long double var = (sumsq / N) - (mean * mean);
    if (var < 0)
        var = 0;
    long double std = sqrtl(var);

    double tolerance = mean_window_ksigma_range((double)min, (double)max, N, 5.0);

    double expectedMean = (min + max) / 2.0;
    double meanDeviation = (double)mean - expectedMean;

    fails += ut_assert(fabs(meanDeviation) <= tolerance, "Mean expected value tolerance");
}

static void ut_randSpecific_float(double (*func)(double), float range)
{
    const int N = NUM_TESTS;
    long double sum = 0.0L, sumsq = 0.0L;
    double min = CMATH_MAX_D, max = -CMATH_MAX_D;

    for (int i = 0; i < N; ++i)
    {
        double r = func(range);
        sum += (long double)r;
        sumsq += (long double)r * (long double)r;
        if (r < min)
            min = r;
        if (r > max)
            max = r;
    }

    long double mean = sum / N;
    long double var = (sumsq / N) - (mean * mean);
    if (var < 0)
        var = 0;
    long double std = sqrtl(var);

    double tolerance = mean_window_ksigma_range((double)min, (double)max, N, 5.0);
    if (tolerance < CMATH_EPSILON_F)
        tolerance = CMATH_EPSILON_F;

    double expectedMean = (min + max) / 2.0;
    double meanDeviation = (double)mean - expectedMean;

    fails += ut_assert(fabs(meanDeviation) <= tolerance, "Mean expected value tolerance");
}

static void ut_deterministicRandom(void)
{
    random_init(random_nextU32t());

    int numFailures = 0;

    for (int i = 0; i < 32; i++)
    {
        if (i < 8)
        {
            ut_randSpecific64(rand_range7bit_adapt, i);
        }
        else
        {
            ut_randSpecific64(rand_range15bit_adapt, i);
        }
    }

    for (int range = 1; range <= 8; ++range)
        ut_randSpecific_int(rand_rangeI32_adapt, range * (int)pow(10, range));

    for (int range = 1; range <= 8; ++range)
        ut_randSpecific_int(rand_rangeU32_adapt, range * (int)pow(10, range));

    // Test decent ranges of floats
    const float floatRanges[] = {0.0001F, 0.001F, 0.01F, 0.1F, 1.0F, 10.0F, 100.0F, 1000.0F, 10000.0F};
    for (int i = 0; i < 9; ++i)
    {
        ut_randSpecific_float(rand_rangeF32_adapt, floatRanges[i]);
        ut_randSpecific_float(rand_rangeF64_adapt, floatRanges[i]);
    }
}

int random_tests_run(void)
{
    return fails;
}