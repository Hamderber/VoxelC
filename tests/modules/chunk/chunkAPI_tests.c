#include "../../unit_tests.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "api/chunk/chunk_t.h"
#include "api/chunk/chunkSource_t.h"
#include "api/chunk/chunkSourceVTable_t.h"
#include "api/chunk/chunkAPI.h"

static int fails = 0;

static int g_loadCalls = 0;
static int g_unloadCalls = 0;
static int g_tickCalls = 0;
static int g_destroyCalls = 0;

static struct ChunkSource_t *g_lastLoadSource = NULL;
static struct Chunk_t **g_lastLoadChunks = NULL;
static size_t g_lastLoadCount = 0;

static struct Chunk_t ***g_lastLoadOutBadParam = NULL;
static size_t *g_lastLoadOutCountPtr = NULL;

static struct ChunkSource_t *g_lastUnloadSource = NULL;
static struct Chunk_t **g_lastUnloadChunks = NULL;
static size_t g_lastUnloadCount = 0;

static struct ChunkSource_t *g_lastTickSource = NULL;
static double g_lastTickDelta = 0.0;

static struct ChunkSource_t *g_lastDestroySource = NULL;

// Static "bad chunk" buffer for mock load
static struct Chunk_t *g_mockBadChunks[2] = {
    (struct Chunk_t *)0x1,
    (struct Chunk_t *)0x2};

static void reset_mock_state(void)
{
    g_loadCalls = g_unloadCalls = g_tickCalls = g_destroyCalls = 0;

    g_lastLoadSource = NULL;
    g_lastLoadChunks = NULL;
    g_lastLoadCount = 0;
    g_lastLoadOutBadParam = NULL;
    g_lastLoadOutCountPtr = NULL;

    g_lastUnloadSource = NULL;
    g_lastUnloadChunks = NULL;
    g_lastUnloadCount = 0;

    g_lastTickSource = NULL;
    g_lastTickDelta = 0.0;

    g_lastDestroySource = NULL;
}

static bool mock_loadChunks(struct ChunkSource_t *restrict pSource,
                            struct Chunk_t **ppChunks,
                            size_t count,
                            struct Chunk_t ***pppOutChunksBad,
                            size_t *restrict pOutCount)
{
    g_loadCalls++;
    g_lastLoadSource = pSource;
    g_lastLoadChunks = ppChunks;
    g_lastLoadCount = count;

    // Track the PARAMETER (address of caller's variable)
    g_lastLoadOutBadParam = pppOutChunksBad;
    g_lastLoadOutCountPtr = pOutCount;

    if (pppOutChunksBad)
        *pppOutChunksBad = g_mockBadChunks;
    if (pOutCount)
        *pOutCount = 2;

    return true;
}

static void mock_unloadChunks(struct ChunkSource_t *restrict pSource,
                              struct Chunk_t **restrict ppChunks,
                              size_t count)
{
    g_unloadCalls++;
    g_lastUnloadSource = pSource;
    g_lastUnloadChunks = ppChunks;
    g_lastUnloadCount = count;
}

static void mock_tick(struct ChunkSource_t *pSource, double deltaTime)
{
    g_tickCalls++;
    g_lastTickSource = pSource;
    g_lastTickDelta = deltaTime;
}

static void mock_destroy(struct ChunkSource_t *pSource)
{
    g_destroyCalls++;
    g_lastDestroySource = pSource;
}

static bool test_chunkSource_loadChunks_basic_forwarding(void)
{
    reset_mock_state();

    ChunkSourceVTable_t vtable = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };

    ChunkSource_t source = {
        .pCHUNK_MANAGER = NULL,
        .pVTABLE = &vtable,
        .pImplData = NULL,
    };

    struct Chunk_t dummyChunks[3];
    struct Chunk_t *pChunks[3] = {
        &dummyChunks[0],
        &dummyChunks[1],
        &dummyChunks[2]};

    struct Chunk_t **pBad = NULL;
    size_t outCount = 0;

    bool ok = chunkSource_loadChunks(&source, pChunks, 3, &pBad, &outCount);
    if (!ok)
        return false;

    if (g_loadCalls != 1)
        return false;
    if (g_lastLoadSource != &source)
        return false;
    if (g_lastLoadChunks != pChunks)
        return false;
    if (g_lastLoadCount != 3)
        return false;

    // The vtable must have been called with the ADDRESS of pBad/outCount
    if (g_lastLoadOutBadParam != &pBad)
        return false;
    if (g_lastLoadOutCountPtr != &outCount)
        return false;

    // mock sets these
    if (pBad != g_mockBadChunks)
        return false;
    if (outCount != 2)
        return false;

    return true;
}

// For count == 0, wrapper SHOULD be a no-op success: don't call vtable, zero outputs.
static bool test_chunkSource_loadChunks_zero_count_noop(void)
{
    reset_mock_state();

    ChunkSourceVTable_t vtable = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };

    ChunkSource_t source = {
        .pCHUNK_MANAGER = NULL,
        .pVTABLE = &vtable,
        .pImplData = NULL,
    };

    struct Chunk_t **pBad = (struct Chunk_t **)(uintptr_t)0xDEADBEEF;
    size_t outCount = 123;

    bool ok = chunkSource_loadChunks(&source, NULL, 0, &pBad, &outCount);

    if (!ok)
        return false;
    if (g_loadCalls != 0)
        return false;
    if (pBad != NULL)
        return false;
    if (outCount != 0)
        return false;

    return true;
}

// Invalid arg behavior: wrapper SHOULD fail safely and not call vtable.
static bool test_chunkSource_loadChunks_invalid_args(void)
{
    reset_mock_state();

    ChunkSourceVTable_t vtable = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };
    ChunkSource_t goodSource = {
        .pCHUNK_MANAGER = NULL,
        .pVTABLE = &vtable,
        .pImplData = NULL,
    };

    struct Chunk_t dummyChunks[2];
    struct Chunk_t *pChunks[2] = {&dummyChunks[0], &dummyChunks[1]};
    bool ok;
    struct Chunk_t **pBad;
    size_t outCount;

    // NULL source
    pBad = (struct Chunk_t **)(uintptr_t)0xDEADBEEF;
    outCount = 999;
    ok = chunkSource_loadChunks(NULL, pChunks, 2, &pBad, &outCount);
    if (ok != false)
        return false;
    if (g_loadCalls != 0)
        return false;
    if (pBad != NULL || outCount != 0)
        return false;

    // NULL vtable
    reset_mock_state();
    ChunkSource_t srcNoVT = {.pCHUNK_MANAGER = NULL, .pVTABLE = NULL, .pImplData = NULL};
    pBad = (struct Chunk_t **)(uintptr_t)0xDEADBEEF;
    outCount = 999;
    ok = chunkSource_loadChunks(&srcNoVT, pChunks, 2, &pBad, &outCount);
    if (ok != false)
        return false;
    if (g_loadCalls != 0)
        return false;
    if (pBad != NULL || outCount != 0)
        return false;

    // NULL pLoadChunksFunc
    reset_mock_state();
    ChunkSourceVTable_t vtableNoLoad = {
        .pLoadChunksFunc = NULL,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };
    ChunkSource_t srcNoLoad = {.pCHUNK_MANAGER = NULL, .pVTABLE = &vtableNoLoad, .pImplData = NULL};
    pBad = (struct Chunk_t **)(uintptr_t)0xDEADBEEF;
    outCount = 999;
    ok = chunkSource_loadChunks(&srcNoLoad, pChunks, 2, &pBad, &outCount);
    if (ok != false)
        return false;
    if (g_loadCalls != 0)
        return false;
    if (pBad != NULL || outCount != 0)
        return false;

    // count > 0 but ppChunks == NULL
    reset_mock_state();
    pBad = (struct Chunk_t **)(uintptr_t)0xDEADBEEF;
    outCount = 999;
    ok = chunkSource_loadChunks(&goodSource, NULL, 2, &pBad, &outCount);
    if (ok != false)
        return false;
    if (g_loadCalls != 0)
        return false;
    if (pBad != NULL || outCount != 0)
        return false;

    // NULL pppOutChunksBad
    reset_mock_state();
    outCount = 999;
    ok = chunkSource_loadChunks(&goodSource, pChunks, 2, NULL, &outCount);
    if (ok != false)
        return false;
    if (g_loadCalls != 0)
        return false;

    // NULL pOutCount
    reset_mock_state();
    pBad = (struct Chunk_t **)(uintptr_t)0xDEADBEEF;
    ok = chunkSource_loadChunks(&goodSource, pChunks, 2, &pBad, NULL);
    if (ok != false)
        return false;
    if (g_loadCalls != 0)
        return false;

    return true;
}

static bool test_chunkSource_unloadChunks_basic(void)
{
    reset_mock_state();

    ChunkSourceVTable_t vtable = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };
    ChunkSource_t source = {
        .pCHUNK_MANAGER = NULL,
        .pVTABLE = &vtable,
        .pImplData = NULL,
    };

    struct Chunk_t dummyChunks[2];
    struct Chunk_t *pChunks[2] = {&dummyChunks[0], &dummyChunks[1]};

    chunkSource_unloadChunks(&source, pChunks, 2);

    if (g_unloadCalls != 1)
        return false;
    if (g_lastUnloadSource != &source)
        return false;
    if (g_lastUnloadChunks != pChunks)
        return false;
    if (g_lastUnloadCount != 2)
        return false;

    return true;
}

static bool test_chunkSource_unloadChunks_invalid_args(void)
{
    reset_mock_state();

    ChunkSourceVTable_t vtable = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };
    ChunkSource_t goodSource = {
        .pCHUNK_MANAGER = NULL,
        .pVTABLE = &vtable,
        .pImplData = NULL,
    };

    struct Chunk_t dummyChunks[1];
    struct Chunk_t *pChunks[1] = {&dummyChunks[0]};

    // NULL source
    chunkSource_unloadChunks(NULL, pChunks, 1);
    if (g_unloadCalls != 0)
        return false;

    // NULL vtable
    reset_mock_state();
    ChunkSource_t srcNoVT = {.pCHUNK_MANAGER = NULL, .pVTABLE = NULL, .pImplData = NULL};
    chunkSource_unloadChunks(&srcNoVT, pChunks, 1);
    if (g_unloadCalls != 0)
        return false;

    // NULL pUnloadChunksFunc
    reset_mock_state();
    ChunkSourceVTable_t vtableNoUnload = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = NULL,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };
    ChunkSource_t srcNoUnload = {.pCHUNK_MANAGER = NULL, .pVTABLE = &vtableNoUnload, .pImplData = NULL};
    chunkSource_unloadChunks(&srcNoUnload, pChunks, 1);
    if (g_unloadCalls != 0)
        return false;

    // count > 0 and ppChunks == NULL -> no call
    reset_mock_state();
    chunkSource_unloadChunks(&goodSource, NULL, 1);
    if (g_unloadCalls != 0)
        return false;

    // count == 0 with NULL ppChunks is allowed (no call)
    reset_mock_state();
    chunkSource_unloadChunks(&goodSource, NULL, 0);
    if (g_unloadCalls != 0)
        return false;

    return true;
}

static bool test_chunkSource_tick_and_destroy(void)
{
    reset_mock_state();

    ChunkSourceVTable_t vtable = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = mock_tick,
        .pDestroyFunc = mock_destroy,
    };
    ChunkSource_t source = {
        .pCHUNK_MANAGER = NULL,
        .pVTABLE = &vtable,
        .pImplData = NULL,
    };

    // Normal tick
    chunkSource_tick(&source, 0.016);
    if (g_tickCalls != 1)
        return false;
    if (g_lastTickSource != &source)
        return false;
    if (g_lastTickDelta != 0.016)
        return false;

    // Normal destroy
    chunkSource_destroy(&source);
    if (g_destroyCalls != 1)
        return false;
    if (g_lastDestroySource != &source)
        return false;

    // NULL source -> no-op
    reset_mock_state();
    chunkSource_tick(NULL, 1.0);
    chunkSource_destroy(NULL);
    if (g_tickCalls != 0 || g_destroyCalls != 0)
        return false;

    // NULL vtable -> no-op
    reset_mock_state();
    ChunkSource_t srcNoVT = {.pCHUNK_MANAGER = NULL, .pVTABLE = NULL, .pImplData = NULL};
    chunkSource_tick(&srcNoVT, 1.0);
    chunkSource_destroy(&srcNoVT);
    if (g_tickCalls != 0 || g_destroyCalls != 0)
        return false;

    // NULL tick/destroy funcs -> no-op
    reset_mock_state();
    ChunkSourceVTable_t vtableNoTickDestroy = {
        .pLoadChunksFunc = mock_loadChunks,
        .pUnloadChunksFunc = mock_unloadChunks,
        .pTickFunc = NULL,
        .pDestroyFunc = NULL,
    };
    ChunkSource_t srcNoTickDestroy = {.pCHUNK_MANAGER = NULL, .pVTABLE = &vtableNoTickDestroy, .pImplData = NULL};
    chunkSource_tick(&srcNoTickDestroy, 0.1);
    chunkSource_destroy(&srcNoTickDestroy);
    if (g_tickCalls != 0 || g_destroyCalls != 0)
        return false;

    return true;
}

int chunkAPI_tests_run(void)
{
    fails += ut_assert(test_chunkSource_loadChunks_basic_forwarding() == true,
                       "chunkSource_loadChunks basic forwarding");
    fails += ut_assert(test_chunkSource_loadChunks_zero_count_noop() == true,
                       "chunkSource_loadChunks zero count no-op");
    fails += ut_assert(test_chunkSource_loadChunks_invalid_args() == true,
                       "chunkSource_loadChunks invalid args");
    fails += ut_assert(test_chunkSource_unloadChunks_basic() == true,
                       "chunkSource_unloadChunks basic");
    fails += ut_assert(test_chunkSource_unloadChunks_invalid_args() == true,
                       "chunkSource_unloadChunks invalid args");
    fails += ut_assert(test_chunkSource_tick_and_destroy() == true,
                       "chunkSource_tick and chunkSource_destroy behavior");

    return fails;
}
