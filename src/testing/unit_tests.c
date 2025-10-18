#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/core.h"
#include "c_math/c_math.h"
#include "world/chunk.h"
#include <float.h>

const int NUM_TESTS = 1000;
static uint32_t originalRngSeed;

/// @brief Performs NUM_TESTS coordinate packing and unpacking. Randomizes a coordinate, packs it, and makes sure the unpacked version
/// matches the original
/// @param seed
/// @return int - number of failures
static int ut_chunkCoordPacking(void)
{
    logs_log(LOG_UNIT_TEST, "Chunk 3D coordinate packing/unpacking test...");

    int failures = 0;

    for (int i = 0; i < NUM_TESTS; i++)
    {
        // Random coordinates within signed 21-bit range (safe for our packer)
        int32_t x = rand_range31bit(21);
        int32_t y = rand_range31bit(21);
        int32_t z = rand_range31bit(21);

        bool loaded = rand_5050();

        uint64_t packed = packChunkPos3D(x, y, z, loaded);
        ChunkPosUnpacked_t unpacked = unpackChunkPos3D(packed);

        if (unpacked.xPos != x || unpacked.yPos != y || unpacked.zPos != z || unpacked.isLoaded != loaded)
        {
            failures++;
            logs_log(LOG_UNIT_TEST, "Test %d FAILED:", i);
            logs_log(LOG_UNIT_TEST, "  Original:   x=%d y=%d z=%d loaded=%d", x, y, z, loaded);
            logs_log(LOG_UNIT_TEST, "  Unpacked:   x=%d y=%d z=%d loaded=%d", unpacked.xPos, unpacked.yPos, unpacked.zPos, unpacked.isLoaded);
            logs_log(LOG_UNIT_TEST, "  Packed:     0x%016llX", packed);
        }
    }

    if (failures == 0)
        logs_log(LOG_UNIT_TEST, "All %d chunk packing tests passed!", NUM_TESTS);
    else
        logs_log(LOG_UNIT_TEST, "%d chunk packing tests failed.", failures);

    return failures;
}

/// @brief Tests the chunk loaded flag bit placement
/// @param void
/// @return int - number of failures
static int ut_chunkFlagBehavior(void)
{
    logs_log(LOG_UNIT_TEST, "Chunk isLoaded flag manipulation test...");

    // Random starting coordinates within safe 21-bit range
    int32_t x = rand_range31bit(21);
    int32_t y = rand_range31bit(21);
    int32_t z = rand_range31bit(21);

    // Randomly pick a starting state (0 or 1)
    bool startLoaded = rand_5050();

    uint64_t packed = packChunkPos3D(x, y, z, startLoaded);

    // Verify initial state
    if (chunkIsLoaded(packed) != startLoaded)
    {
        logs_log(LOG_UNIT_TEST, "Initial load state mismatch! Expected %d got %d",
                 startLoaded, chunkIsLoaded(packed));
        return 1;
    }

    // Flip the flag
    packed = chunkSetLoaded(packed, !startLoaded);
    if (chunkIsLoaded(packed) != !startLoaded)
    {
        logs_log(LOG_UNIT_TEST, "Failed to toggle isLoaded flag! Expected %d got %d",
                 !startLoaded, chunkIsLoaded(packed));
        return 1;
    }

    // Flip back to the original state
    packed = chunkSetLoaded(packed, startLoaded);
    if (chunkIsLoaded(packed) != startLoaded)
    {
        logs_log(LOG_UNIT_TEST, "Failed to restore isLoaded flag! Expected %d got %d",
                 startLoaded, chunkIsLoaded(packed));
        return 1;
    }

    logs_log(LOG_UNIT_TEST, "isLoaded flag tests passed (start=%d).", startLoaded);
    return 0;
}

// Adapt the specific values output from the rand_ranges to all be the same type (for *func use)
static int64_t rand_range7bit_adapt(int n) { return (int64_t)rand_range7bit(n); }
static int64_t rand_range15bit_adapt(int n) { return (int64_t)rand_range15bit(n); }
static int64_t rand_range31bit_adapt(int n) { return (int64_t)rand_range31bit(n); }
static int64_t rand_rangeI32_adapt(int n) { return (int64_t)rand_rangeI32(-n, n); }
static int64_t rand_rangeU32_adapt(int n)
{
    // recenter mean to 0
    int32_t r = (int32_t)rand_rangeU32(0, (uint32_t)n);
    return (int64_t)(r - (n / 2));
}

static double rand_rangeF32_adapt(double n)
{
    if (n == 0)
        return 0.0;
    return (double)rand_rangeF32(-1.0F * (float)n, (float)n);
}

static double rand_rangeF64_adapt(double n)
{
    if (n == 0)
        return 0.0;
    return rand_rangeF64(-1.0 * n, n);
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

static int ut_randSpecific64(int64_t (*func)(int), int range)
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

    logs_log(LOG_UNIT_TEST,
             "Rand bits (%d):\tMean (5 std %.3le): %.3le\tVar: %.3le\tStd: %.3le\tMin: %12lld\tMax: %12lld",
             range, tolerance, mean, var, std, (long long)min, (long long)max);

    if (fabsl(mean) > tolerance)
        logs_log(LOG_WARN, "Mean %.4lf drifted beyond expected window %.4lf", mean, tolerance);

    return 0;
}

static int ut_randSpecific_int(int64_t (*func)(int), int range)
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

    logs_log(LOG_UNIT_TEST,
             "Rand (u)int %d:\tMean (5 std %.3le): %.3le\tVar: %.3le\tStd: %.3le\tMin: %12lld\tMax: %12lld",
             range, tolerance, mean, var, std, (long long)min, (long long)max);

    // Numebers may not be centered around zero so the mean has to be shifted
    double expectedMean = (min + max) / 2.0;
    double meanDeviation = (double)mean - expectedMean;

    if (fabs(meanDeviation) > tolerance)
        logs_log(LOG_WARN, "Mean %.4Lf drifted beyond expected window %.4lf", mean, tolerance);

    return 0;
}

static int ut_randSpecific_float(double (*func)(double), float range)
{
    const int N = NUM_TESTS;
    long double sum = 0.0L, sumsq = 0.0L;
    double min = DBL_MAX, max = -DBL_MAX;

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
    if (tolerance < 1e-6)
        tolerance = 1e-6;

    logs_log(LOG_UNIT_TEST,
             "Rand float %.4f:\tMean (5 std %.3e): %.3Le\tVar: %.3Le\tStd: %.3Le\tMin: %12.6f\tMax: %12.6f",
             range, tolerance, mean, var, std, min, max);

    double expectedMean = (min + max) / 2.0;
    double meanDeviation = (double)mean - expectedMean;

    if (fabs(meanDeviation) > tolerance)
        logs_log(LOG_WARN, "Mean %.6Lf drifted beyond expected window %.4le", mean, tolerance);

    return 0;
}

static int ut_deterministicRandom(void)
{
    rand_init(rand_nextU32t());

    int numFailures = 0;

    for (int i = 0; i < 32; i++)
    {
        if (i < 8)
        {
            numFailures += ut_randSpecific64(rand_range7bit_adapt, i);
        }
        else if (i < 16)
        {
            numFailures += ut_randSpecific64(rand_range15bit_adapt, i);
        }
        else
        {
            numFailures += ut_randSpecific64(rand_range31bit_adapt, i);
        }
    }

    for (int range = 1; range <= 8; ++range)
        numFailures += ut_randSpecific_int(rand_rangeI32_adapt, range * (int)pow(10, range));

    for (int range = 1; range <= 8; ++range)
        numFailures += ut_randSpecific_int(rand_rangeU32_adapt, range * (int)pow(10, range));

    // Test decent ranges of floats
    const float floatRanges[] = {0.0001F, 0.001F, 0.01F, 0.1F, 1.0F, 10.0F, 100.0F, 1000.0F, 10000.0F};
    for (int i = 0; i < 9; ++i)
    {
        numFailures += ut_randSpecific_float(rand_rangeF32_adapt, floatRanges[i]);
        numFailures += ut_randSpecific_float(rand_rangeF64_adapt, floatRanges[i]);
    }
    return numFailures;
}

/// @brief Runs all unit tests and accumulates all failures
/// @param  void
/// @return int
static int ut_tests(void)
{
    logs_log(LOG_UNIT_TEST, "Running unit tests (seed=%u)...", rand_seedGet());

    int failures = 0;
    failures += ut_deterministicRandom();
    failures += ut_chunkCoordPacking();
    failures += ut_chunkFlagBehavior();

    return failures;
}

/// @brief Run all unit tests and log if any failed
/// @param void
void ut_run(void)
{
    // Preserve original seed before testing
    originalRngSeed = rand_seedGet();

    logs_logIfError(ut_tests(), "Unit test(s) failed!");

    // Restore original seed after testing
    rand_init(originalRngSeed);
}
