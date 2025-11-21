#include "../../unit_tests.h"
#include <stdbool.h>
#include "chunk/chunk.h"
#include "api/chunk/chunkState_e.h"

static int fails = 0;

static bool test_chunkState_set_basic_and_invalid(void)
{
    Chunk_t chunk = {0};
    chunk.chunkState = CHUNK_STATE_UNLOADED;

    // Valid transition
    if (!chunkState_set(&chunk, CHUNK_STATE_CPU_EMPTY))
        return false;
    if (chunk.chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;

    // Another valid transition
    if (!chunkState_set(&chunk, CHUNK_STATE_CPU_ONLY))
        return false;
    if (chunk.chunkState != CHUNK_STATE_CPU_ONLY)
        return false;

    // Invalid state: CHUNK_STATE_COUNT (one past last valid)
    ChunkState_e invalid = (ChunkState_e)CHUNK_STATE_COUNT;
    if (chunkState_set(&chunk, invalid) != false)
        return false;
    // State must be unchanged
    if (chunk.chunkState != CHUNK_STATE_CPU_ONLY)
        return false;

    // NULL chunk pointer must be rejected
    if (chunkState_set(NULL, CHUNK_STATE_CPU_GPU) != false)
        return false;

    return true;
}

static bool test_chunkState_setBatch_basic(void)
{
    Chunk_t a = {0};
    Chunk_t b = {0};
    Chunk_t c = {0};

    a.chunkState = CHUNK_STATE_UNLOADED;
    b.chunkState = CHUNK_STATE_CPU_ONLY;
    c.chunkState = CHUNK_STATE_CPU_GPU;

    Chunk_t *chunks[3] = {&a, &b, &c};

    // Valid batch set
    if (!chunkState_setBatch(chunks, 3, CHUNK_STATE_CPU_EMPTY))
        return false;

    if (a.chunkState != CHUNK_STATE_CPU_EMPTY ||
        b.chunkState != CHUNK_STATE_CPU_EMPTY ||
        c.chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;

    // count == 0: must be a no-op success
    a.chunkState = CHUNK_STATE_UNLOADED;
    if (!chunkState_setBatch(chunks, 0, CHUNK_STATE_CPU_GPU))
        return false;
    // nothing should change
    if (a.chunkState != CHUNK_STATE_UNLOADED)
        return false;

    // NULL array pointer: must fail
    if (chunkState_setBatch(NULL, 1, CHUNK_STATE_CPU_ONLY) != false)
        return false;

    return true;
}

static bool test_chunkState_setBatch_null_element_and_invalid_state(void)
{
    Chunk_t a = {0};
    Chunk_t b = {0};
    Chunk_t c = {0};

    a.chunkState = CHUNK_STATE_UNLOADED;
    b.chunkState = CHUNK_STATE_CPU_ONLY;
    c.chunkState = CHUNK_STATE_CPU_GPU;

    // Array with a NULL in the middle
    Chunk_t *chunksWithNull[3] = {&a, NULL, &c};

    // Valid state, but one NULL element: some entries updated, but return false
    bool ok = chunkState_setBatch(chunksWithNull, 3, CHUNK_STATE_CPU_EMPTY);
    if (ok != false)
        return false;

    // a and c must be updated
    if (a.chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;
    if (c.chunkState != CHUNK_STATE_CPU_EMPTY)
        return false;

    // b is untouched (we never passed its address)
    if (b.chunkState != CHUNK_STATE_CPU_ONLY)
        return false;

    // Now test invalid state via batch: none should change, and return false
    a.chunkState = CHUNK_STATE_UNLOADED;
    b.chunkState = CHUNK_STATE_CPU_ONLY;
    c.chunkState = CHUNK_STATE_CPU_GPU;

    Chunk_t *chunks[3] = {&a, &b, &c};
    ChunkState_e invalid = (ChunkState_e)CHUNK_STATE_COUNT;

    ok = chunkState_setBatch(chunks, 3, invalid);
    if (ok != false)
        return false;

    // All must remain unchanged
    if (a.chunkState != CHUNK_STATE_UNLOADED ||
        b.chunkState != CHUNK_STATE_CPU_ONLY ||
        c.chunkState != CHUNK_STATE_CPU_GPU)
        return false;

    return true;
}

static bool test_chunkState_cpu_gpu_classification(void)
{
    Chunk_t chunk = {0};

    // UNLOADED: not cpu, not gpu
    chunk.chunkState = CHUNK_STATE_UNLOADED;
    if (chunkState_cpu(&chunk) != false)
        return false;
    if (chunkState_gpu(&chunk) != false)
        return false;

    // CPU_EMPTY: cpu, not gpu
    chunk.chunkState = CHUNK_STATE_CPU_EMPTY;
    if (chunkState_cpu(&chunk) != true)
        return false;
    if (chunkState_gpu(&chunk) != false)
        return false;

    // CPU_LOADING: cpu, not gpu
    chunk.chunkState = CHUNK_STATE_CPU_LOADING;
    if (chunkState_cpu(&chunk) != true)
        return false;
    if (chunkState_gpu(&chunk) != false)
        return false;

    // CPU_FAILED: not cpu, not gpu
    chunk.chunkState = CHUNK_STATE_CPU_FAILED;
    if (chunkState_cpu(&chunk) != false)
        return false;
    if (chunkState_gpu(&chunk) != false)
        return false;

    // CPU_ONLY: cpu, not gpu
    chunk.chunkState = CHUNK_STATE_CPU_ONLY;
    if (chunkState_cpu(&chunk) != true)
        return false;
    if (chunkState_gpu(&chunk) != false)
        return false;

    // CPU_GPU: cpu + gpu
    chunk.chunkState = CHUNK_STATE_CPU_GPU;
    if (chunkState_cpu(&chunk) != true)
        return false;
    if (chunkState_gpu(&chunk) != true)
        return false;

    // Invalid state: should be treated as neither cpu nor gpu
    chunk.chunkState = (ChunkState_e)CHUNK_STATE_COUNT;
    if (chunkState_cpu(&chunk) != false)
        return false;
    if (chunkState_gpu(&chunk) != false)
        return false;

    return true;
}

int chunkState_tests_run(void)
{
    fails += ut_assert(test_chunkState_set_basic_and_invalid() == true,
                       "ChunkState set basic + invalid");
    fails += ut_assert(test_chunkState_setBatch_basic() == true,
                       "ChunkState setBatch basic");
    fails += ut_assert(test_chunkState_setBatch_null_element_and_invalid_state() == true,
                       "ChunkState setBatch null element + invalid state");
    fails += ut_assert(test_chunkState_cpu_gpu_classification() == true,
                       "ChunkState cpu/gpu classification");
    return fails;
}
