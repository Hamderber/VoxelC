#include <time.h>
#include <stdlib.h>
#include "core/core.h"
#include "c_math/c_math.h"
#include "world/chunk.h"

const int NUM_TESTS = 100000;

/// @brief Performs NUM_TESTS coordinate packing and unpacking. Randomizes a coordinate, packs it, and makes sure the unpacked version
/// matches the original
/// @param seed
/// @return int - number of failures
static int ut_chunkCoordPacking()
{
    logs_log(LOG_UNIT_TEST, "Chunk 3D coordinate packing/unpacking test...");

    int failures = 0;

    for (int i = 0; i < NUM_TESTS; i++)
    {
        // Random coordinates within signed 21-bit range (safe for our packer)
        int32_t x = rand_range32bit(21);
        int32_t y = rand_range32bit(21);
        int32_t z = rand_range32bit(21);

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

/// @brief
/// @param
/// @return
static int ut_chunkFlagBehavior(void)
{
    logs_log(LOG_UNIT_TEST, "Chunk isLoaded flag manipulation test...");

    // Random starting coordinates within safe 21-bit range
    int32_t x = rand_range32bit(21);
    int32_t y = rand_range32bit(21);
    int32_t z = rand_range32bit(21);

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

/// @brief Runs all unit tests and accumulates all failures
/// @param  void
/// @return int
static int ut_tests(void)
{
    logs_log(LOG_UNIT_TEST, "Running unit tests (seed=%u)...", rand_seedGet());

    int failures = 0;
    failures += ut_chunkCoordPacking();
    failures += ut_chunkFlagBehavior();

    return failures;
}

/// @brief Run all unit tests and log if any failed
/// @param void
void ut_run(void)
{
    logs_logIfError(ut_tests(), "Unit test(s) failed!");
}
