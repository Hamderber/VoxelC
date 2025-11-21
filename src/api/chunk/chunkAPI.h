#pragma region Includes
#pragma once
#include "api/chunk/chunk_t.h"
#include "api/chunk/chunkSource_t.h"
#include "api/chunk/chunkSourceVTable_t.h"
#include "api/chunk/chunkState_e.h"
#pragma endregion
#pragma region VTable Wrappers
/// @brief To actually call the vtable function, the following contract is promised (verified by this wrapper):
/// pOutCount exists and is set to 0.
/// ppOutChunksBad exists and is set to NULL.
/// pSource, its vtable, and the loadChunks function exist.
/// No-op success on count == 0.
/// ppChunks exists.
/// Returns the result of the vtable loadChunk function of the afformentioned criteria is met.
static inline bool chunkSource_loadChunks(ChunkSource_t *restrict pSource, Chunk_t **ppChunks, size_t count,
                                          Chunk_t ***pppOutChunksBad, size_t *restrict pOutCount)
{
    if (pOutCount)
        *pOutCount = 0;

    if (pppOutChunksBad)
        *pppOutChunksBad = NULL;

    if (!pSource || !pSource->pVTABLE || !pSource->pVTABLE->pLoadChunksFunc)
        return false;

    if (count == 0)
        return true;

    if (!ppChunks || !pppOutChunksBad || !pOutCount)
        return false;

    return pSource->pVTABLE->pLoadChunksFunc(pSource, ppChunks, count, pppOutChunksBad, pOutCount);
}

/// @brief To actually call the vtable function, the following contract is promised (verified by this wrapper):
/// pSource, its vtable, and the unloadChunks function exist.
/// No-op success on count == 0.
/// ppChunks exists.
/// Calls the vtable unloadChunk function of the afformentioned criteria is met.
static inline void chunkSource_unloadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count)
{
    if (!pSource || !pSource->pVTABLE || !pSource->pVTABLE->pUnloadChunksFunc)
        return;

    if (count == 0)
        return;

    if (!ppChunks)
        return;

    pSource->pVTABLE->pUnloadChunksFunc(pSource, ppChunks, count);
}

/// @brief To actually call the vtable function, the following contract is promised (verified by this wrapper):
/// pSource, its vtable, and the tick function exist.
/// Calls the vtable tick function of the afformentioned criteria is met.
static inline void chunkSource_tick(ChunkSource_t *pSource, double deltaTime)
{
    if (!pSource || !pSource->pVTABLE || !pSource->pVTABLE->pTickFunc)
        return;

    pSource->pVTABLE->pTickFunc(pSource, deltaTime);
}

/// @brief To actually call the vtable function, the following contract is promised (verified by this wrapper):
/// pSource, its vtable, and the destroy function exist.
/// Calls the vtable destroy function of the afformentioned criteria is met.
static inline void chunkSource_destroy(ChunkSource_t *pSource)
{
    if (!pSource || !pSource->pVTABLE || !pSource->pVTABLE->pDestroyFunc)
        return;

    pSource->pVTABLE->pDestroyFunc(pSource);
}
#pragma endregion