#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include "./Chunk/Chunk.h"
#include "./Debug/Logger.h"
const int NUM_TESTS = 100000;

int chunkCoordPackingTest(unsigned int seed)
{
    logger(LOG_INFO, "Chunk coordinate packing/unpacking test...");

    int failures = 0;

    // Set the seed for the rand
    srand(seed);

    for (int i = 0; i < NUM_TESTS; i++)
    {
        // Random coordinates in full int32 range
        // Need to do the shift because rand outputs a 16 bit int. So shift 16 then or the bits of another rand
        int32_t x = (int32_t)((rand() << 16) | rand());
        int32_t z = (int32_t)((rand() << 16) | rand());

        uint64_t packed = packChunkPos(x, z);
        ChunkPosUnpacked_t unpacked = unpackChunkPos(packed);

        if (unpacked.xPos != x || unpacked.zPos != z)
        {
            failures++;
            logger(LOG_INFO, "Test %d FAILED:\n", i);
            logger(LOG_INFO, "  Original:   x=%d, z=%d\n", x, z);
            logger(LOG_INFO, "  Unpacked:   x=%d, z=%d\n", unpacked.xPos, unpacked.zPos);
            logger(LOG_INFO, "  Packed:     0x%016llX\n", packed);
        }
    }

    if (failures == 0)
    {
        logger(LOG_INFO, "All %d tests passed!\n", NUM_TESTS);
    }
    else
    {
        logger(LOG_WARN, "%d tests failed.\n", failures);
    }

    return failures;
}

int main(void)
{
    // Random seed using unsigned time at .exe start
    unsigned int seed = (unsigned int)time(NULL);

    int failures = 0;
    failures += chunkCoordPackingTest(seed);

    // If failures != 0, then there is an exit code that is interpreted as some type of bad exit status
    return failures;
}
