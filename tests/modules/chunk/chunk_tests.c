#include "../../unit_tests.h"
#include <stdbool.h>
#include "cmath/cmath.h"
#include "chunk/chunk.h"

static int fails = 0;

static bool test_chunk_world_create_basic(void)
{
    const Vec3i_t POS = {1, 2, 3};
    Chunk_t *pChunk = chunk_world_create(POS);
    if (!pChunk)
        return false;

    // Check position
    if (!cmath_vec3i_equals(pChunk->chunkPos, POS, 0))
        return false;

    // Check initial state
    if (pChunk->chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;

    // Block storage allocated
    if (!pChunk->pBlockVoxels)
        return false;

    if (pChunk->pRenderChunk != NULL)
        return false;
    if (pChunk->pTransparencyGrid != NULL)
        return false;
    if (pChunk->pEntitiesLoadingChunkLL != NULL)
        return false;

    // Cleanup via chunk_destroy (normal path)
    int dummyCtx = 0;
    chunk_destroy(&dummyCtx, pChunk);

    return true;
}

static bool test_chunk_world_destroy_basic(void)
{
    const Vec3i_t POS = {0, 0, 0};
    Chunk_t *pChunk = chunk_world_create(POS);
    if (!pChunk)
        return false;

    if (!pChunk->pBlockVoxels)
        return false;

    chunk_world_destroy(pChunk);

    if (pChunk->chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;

    if (pChunk->pEntitiesLoadingChunkLL != NULL)
        return false;

    free(pChunk);

    return true;
}

static bool test_chunk_destroy_cpu_empty(void)
{
    const Vec3i_t POS = {5, -2, 7};
    Chunk_t *pChunk = chunk_world_create(POS);
    if (!pChunk)
        return false;

    if (pChunk->chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;

    int dummyCtx = 0;

    chunk_destroy(&dummyCtx, pChunk);

    return true;
}

static bool test_chunk_destroy_cpu_only(void)
{
    const Vec3i_t POS = {10, 20, 30};
    Chunk_t *pChunk = chunk_world_create(POS);
    if (!pChunk)
        return false;

    pChunk->chunkState = CHUNK_STATE_CPU_ONLY;

    pChunk->pEntitiesLoadingChunkLL = NULL;

    int dummyCtx = 0;
    chunk_destroy(&dummyCtx, pChunk);

    return true;
}

static bool test_chunk_destroy_cpu_loading(void)
{
    const Vec3i_t POS = {0, 0, 0};
    Chunk_t *pChunk = chunk_world_create(POS);
    if (!pChunk)
        return false;

    // Force the loading state
    pChunk->chunkState = CHUNK_STATE_CPU_LOADING;

    int dummyCtx = 0;
    chunk_destroy(&dummyCtx, pChunk);

    return true;
}

int chunk_tests_run(void)
{
    fails += ut_assert(test_chunk_world_create_basic() == true,
                       "Chunk world_create basic");
    fails += ut_assert(test_chunk_world_destroy_basic() == true,
                       "Chunk world_destroy basic (no GPU)");
    fails += ut_assert(test_chunk_destroy_cpu_empty() == true,
                       "Chunk destroy CPU_EMPTY");
    fails += ut_assert(test_chunk_destroy_cpu_only() == true,
                       "Chunk destroy CPU_ONLY");
    fails += ut_assert(test_chunk_destroy_cpu_loading() == true,
                       "Chunk destroy CPU_LOADING warning + fallthrough");

    return fails;
}
