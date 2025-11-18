#pragma region Includes
#pragma once
#include "api/chunk/chunk_t.h"
#include "api/chunk/chunkSource_t.h"
#include "api/chunk/chunkSourceVTable_t.h"
#include "api/chunk/chunkState_e.h"
#pragma endregion
#pragma region VTable Wrappers
static inline bool chunkSource_loadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count)
{
    return pSource->pVTABLE->pLoadChunksFunc(pSource, ppChunks, count);
}

static inline void chunkSource_unloadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count)
{
    pSource->pVTABLE->pUnloadChunksFunc(pSource, ppChunks, count);
}

static inline void chunkSource_tick(ChunkSource_t *pSource, double deltaTime)
{
    if (pSource->pVTABLE->pTickFunc)
        pSource->pVTABLE->pTickFunc(pSource, deltaTime);
}

static inline void chunkSource_destroy(ChunkSource_t *pSource)
{
    if (pSource->pVTABLE->pDestroyFunc)
        pSource->pVTABLE->pDestroyFunc(pSource);
}
#pragma endregion