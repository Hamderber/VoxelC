#pragma region Includes
#pragma once
#include "api/chunk/chunk_t.h"
#include "api/chunk/chunkSource_t.h"
#include "api/chunk/chunkSourceVTable_t.h"
#include "api/chunk/chunkState_e.h"
#pragma endregion
#pragma region VTable Wrappers
static inline bool chunkSource_loadChunk(ChunkSource_t *pSource, Chunk_t *pChunk)
{
    return pSource->pVTABLE->pLoadChunkFunc(pSource, pChunk);
}

static inline void chunkSource_unloadChunk(ChunkSource_t *pSource, Chunk_t *pChunk)
{
    pSource->pVTABLE->pUnloadChunkFunc(pSource, pChunk);
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