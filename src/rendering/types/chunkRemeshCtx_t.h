#pragma region Includes
#pragma once
#include <stdint.h>
#pragma endregion
#pragma region Defines
typedef struct ChunkRemeshCtx_t
{
    struct Chunk_t *restrict pChunk;
    struct Chunk_t **restrict ppLoadedNeighbors;
    uint8_t loadedNeighborCount;
} ChunkRemeshCtx_t;
#pragma endregion
#pragma region Create/Destroy
static inline ChunkRemeshCtx_t *remeshContext_create(Chunk_t *restrict pChunk, Chunk_t **restrict ppNeighbors)
{
    if (!pChunk || !ppNeighbors)
        // if (!pChunk || !pChunk->pRenderChunk || !ppNeighbors)
        return NULL;

    // // Avoid duplicate requeues
    // if (pChunk->pRenderChunk->queuedForRemesh)
    //     return NULL;

    ChunkRemeshCtx_t *pCtx = malloc(sizeof(ChunkRemeshCtx_t));
    if (!pCtx)
        return NULL;

    pCtx->ppLoadedNeighbors = calloc(6, sizeof(Chunk_t *));
    if (!pCtx->ppLoadedNeighbors)
    {
        free(pCtx);
        return NULL;
    }

    pCtx->pChunk = pChunk;
    pCtx->loadedNeighborCount = 0;

    for (uint8_t i = 0; i < 6; i++)
        // There will always be 6 neighbors, but the chunk will be null if it isnt found/isnt loaded at the time of initial
        // query
        if (ppNeighbors[i])
        {
            // logs_log(LOG_DEBUG, "Added chunk (%d, %d, %d) as a neighbor for remeshing to chunk (%d, %d, %d).",
            //          ppNeighbors[i]->chunkPos.x, ppNeighbors[i]->chunkPos.y, ppNeighbors[i]->chunkPos.z,
            //          pChunk->chunkPos.x, pChunk->chunkPos.y, pChunk->chunkPos.z);

            pCtx->ppLoadedNeighbors[pCtx->loadedNeighborCount++] = ppNeighbors[i];
        }

    // pChunk->pRenderChunk->queuedForRemesh = true;

    return pCtx;
}

static inline void remeshContext_destroy(ChunkRemeshCtx_t *pCtx)
{
    if (pCtx)
    {
        free(pCtx->ppLoadedNeighbors);
        free(pCtx);
    }
}
#pragma endregion