#pragma region Includes
#include <stdlib.h>
#include "core/types/state_t.h"
#include "chunk.h"
#include "rendering/chunk/chunkRendering.h"
#include "world/voxel/block_t.h"
#include "world/chunkSolidityGrid.h"
#include "collection/linkedList_t.h"
#pragma endregion
#pragma region Defines
#define DEBUG_CHUNK
#if defined(DEBUG_CHUNK)
static size_t chunkAllocatedCount = 0;
static size_t chunkFreedCount = 0;
#endif
#pragma endregion
#pragma region Destroy
static void entitiesLoadingLL_destroy(Chunk_t *pChunk)
{
    if (!pChunk)
        return;

    linkedList_destroy(&pChunk->pEntitiesLoadingChunkLL, NULL, NULL);

    pChunk->pEntitiesLoadingChunkLL = NULL;
}

void chunk_world_destroy(Chunk_t *pChunk)
{
    // Verify that the chunk isn't still connected to the GPU. It shouldn't be at this point
    if (chunkState_gpu(pChunk))
    {
        logs_log(LOG_ERROR, "Attempted to destroy the world portion of a chunk while it is still associated with the GPU!");
        return;
    }

    entitiesLoadingLL_destroy(pChunk);
    free(pChunk->pBlockVoxels);
    chunkSolidityGrid_destroy(pChunk->pTransparencyGrid);

    chunkState_set(pChunk, CHUNK_STATE_CPU_EMPTY);
}

void chunk_destroy(void *pCtx, Chunk_t *pChunk)
{
    // Cast ctx to state so this can be used in a linked list destructor
    if (!pCtx || !pChunk)
        return;

#if defined(DEBUG_CHUNK)
    const Vec3i_t CHUNK_POS = pChunk->chunkPos;
#endif

    State_t *pState = (State_t *)pCtx;

    // Only attempt to destroy the GPU-side if there is a gpu-associated state
    if (chunkState_gpu(pChunk))
    {
        chunk_renderDestroy(pState, pChunk->pRenderChunk);
        chunkState_set(pChunk, CHUNK_STATE_CPU_ONLY);
    }

    // Verify that there is a cpu-side for the chunk. This should always be the case
    if (chunkState_cpu(pChunk))
        chunk_world_destroy(pChunk);

#if defined(DEBUG_CHUNK)
    chunkFreedCount++;
    logs_log(LOG_DEBUG, "[Chunk free # %u] freed %u bytes at %p for chunk at (%d, %d, %d).", chunkFreedCount,
             sizeof(Chunk_t), pChunk, CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
#endif

    free(pChunk);
}
#pragma endregion
#pragma region Create
Chunk_t *chunk_world_create(const Vec3i_t CHUNK_POS)
{
    Chunk_t *pChunk = calloc(1, sizeof(Chunk_t));
    if (!pChunk)
        return NULL;

    chunkState_set(pChunk, CHUNK_STATE_CPU_EMPTY);
    pChunk->pBlockVoxels = malloc(sizeof(BlockVoxel_t) * CHUNK_BLOCK_CAPACITY);
    pChunk->chunkPos = CHUNK_POS;

#if defined(DEBUG_CHUNK)
    chunkAllocatedCount++;
    logs_log(LOG_DEBUG, "[Chunk alloc # %u] Allocated %u bytes at %p for chunk at (%d, %d, %d).", chunkAllocatedCount,
             sizeof(Chunk_t), pChunk, CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
#endif
    return pChunk;
}
#pragma endregion
#pragma region
#undef DEBUG_CHUNK
#pragma endregion