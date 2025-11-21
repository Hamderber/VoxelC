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
#if defined(DEBUG)
// #define DEBUG_CHUNK
#if defined(DEBUG_CHUNK)
static size_t chunkAllocatedCount = 0;
static size_t chunkFreedCount = 0;
#endif
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
    // Unused in this instance
    pCtx;

    if (!pChunk)
        return;

    const Vec3i_t CHUNK_POS = pChunk->chunkPos;

    // Fall-through for handling different chunk states during destruction!
    switch (pChunk->chunkState)
    {
    // Something has gone wrong and the chunk is being destroyed early because of it
    case CHUNK_STATE_CPU_LOADING:
        logs_log(LOG_WARN, "Destroying chunk %p at (%d, %d, %d) despite it still being in a loading state! \
Something must've gone wrong for this to happen.",
                 pChunk, CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);

    case CHUNK_STATE_CPU_GPU:
        // Only attempt to destroy the GPU-side if there is a gpu-associated state
        if (chunkState_gpu(pChunk))
        {
            // TODO: Finish decoupling the state from the chunk system. There is only one state and only one renderer, after all
            chunk_render_Destroy(pChunk->pRenderChunk);
            (pChunk)->pRenderChunk = NULL;
            chunkState_set(pChunk, CHUNK_STATE_CPU_ONLY);
        }

    case CHUNK_STATE_CPU_ONLY:
    case CHUNK_STATE_CPU_EMPTY:
    default:
        chunk_world_destroy(pChunk);

#if defined(DEBUG_CHUNK)
        chunkFreedCount++;
        logs_log(LOG_DEBUG, "[Chunk free # %u] freed %u bytes at %p for chunk at (%d, %d, %d).", chunkFreedCount,
                 sizeof(Chunk_t), pChunk, CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
#endif
        // There is no need to set chunk state to empty because its just freed. This means that when debugging chunk state,
        // the last entry will be "Chunk 0000022C822F68A0 at (0, 0, 0) state CHUNK_STATE_CPU_LOADING -> CHUNK_STATE_CPU_EMPTY."
        free(pChunk);
        break;
    };
}
#pragma endregion
#pragma region Create
Chunk_t *chunk_world_create(const Vec3i_t CHUNK_POS)
{
    Chunk_t *pChunk = calloc(1, sizeof(Chunk_t));
    if (!pChunk)
        return NULL;

    chunkState_set(pChunk, CHUNK_STATE_CPU_EMPTY);
    pChunk->pBlockVoxels = calloc(CMATH_CHUNK_BLOCK_CAPACITY, sizeof(BlockVoxel_t));
    pChunk->chunkPos = CHUNK_POS;

#if defined(DEBUG_CHUNK)
    chunkAllocatedCount++;
    logs_log(LOG_DEBUG, "[Chunk alloc # %u] Allocated %u bytes at %p for chunk at (%d, %d, %d).", chunkAllocatedCount,
             sizeof(Chunk_t), pChunk, CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
#endif
    return pChunk;
}
#pragma endregion