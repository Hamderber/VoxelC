// #include <time.h>
// #include <stdlib.h>
// #include <stdint.h>
// #include "core/logs.h"
// #include "core/random.h"
// #include "cmath/cmath.h"
// #include "../unit_tests.h"

// static const int NUM_TESTS = 1000;
// static int fails = 0;
// static uint32_t originalRngSeed;

// static void ut_chunkCoordPacking(void)
// {
//     for (int i = 0; i < NUM_TESTS; i++)
//     {
//         // Random coordinates within signed 21-bit range (safe for our packer)
//         int32_t x = random_rangeNbit(21);
//         int32_t y = random_rangeNbit(21);
//         int32_t z = random_rangeNbit(21);

//         bool loaded = random_5050();

//         uint64_t packed = packChunkPos3D(x, y, z, loaded);
//         ChunkPosUnpacked_t unpacked = unpackChunkPos3D(packed);
//         fails += ut_assert(unpacked.xPos == x && unpacked.yPos == y && unpacked.zPos == z && unpacked.isLoaded == loaded, "Chunk packing/unpacking");
//     }
// }

// static void ut_chunkFlagBehavior(void)
// {
//     // Random starting coordinates within safe 21-bit range
//     int32_t x = random_rangeNbit(21);
//     int32_t y = random_rangeNbit(21);
//     int32_t z = random_rangeNbit(21);

//     // Randomly pick a starting state (0 or 1)
//     bool startLoaded = random_5050();

//     uint64_t packed = packChunkPos3D(x, y, z, startLoaded);

//     // Verify initial state
//     fails += ut_assert(chunkIsLoaded(packed) == startLoaded, "Chunk isLoaded flag packing");

//     // Flip the flag
//     packed = chunkSetLoaded(packed, !startLoaded);
//     fails += ut_assert(chunkIsLoaded(packed) == !startLoaded, "Chunk isLoaded flag toggle");

//     // Flip back to the original state
//     packed = chunkSetLoaded(packed, startLoaded);
//     fails += ut_assert(chunkIsLoaded(packed) == startLoaded, "Chunk isLoaded flag restoration");
// }

// int chunk_tests_run(void)
// {
//     // Preserve original seed before testing
//     originalRngSeed = random_seedGet();

//     ut_chunkCoordPacking();
//     ut_chunkFlagBehavior();

//     // Restore original seed after testing
//     random_init(originalRngSeed);

//     return fails;
// }