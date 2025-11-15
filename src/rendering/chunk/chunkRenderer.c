#pragma region Includes
#include <stdbool.h>
#include "core/types/state_t.h"
#include "world/worldState_t.h"
#include "collection/dynamicStack_t.h"
#include "rendering/chunk/chunkRendering.h"
#include "world/chunkManager.h"
#pragma endregion
#pragma region Defines
#define DEFAULT_QUEUE_SIZE 32
#define MAX_REMESH_PER_FRAME 1
#pragma endregion
#pragma region Operations
bool chunkRenderer_enqueueRemesh(WorldState_t *restrict pWorldState, Chunk_t *restrict pChunk)
{
    if (!pWorldState || !pWorldState->chunkRenderer.pRemeshCtxQueue || !pChunk)
        return false;

    RenderChunk_t *pRenderChunk = pChunk->pRenderChunk;

    // Avoid duplicates
    if (pRenderChunk && pRenderChunk->queuedForRemesh)
        return true;

    if (dynamicStack_pushUnique(pWorldState->chunkRenderer.pRemeshCtxQueue, pChunk))
        if (pRenderChunk)
            pRenderChunk->queuedForRemesh = true;

    return true;
}

static bool chunkRenderer_dequeueRemesh(State_t *restrict pState, const Vec3u8_t *restrict pPOINTS,
                                        const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                                        const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK)
{
    if (!pState || !pState->pWorldState || !pState->pWorldState->chunkRenderer.pRemeshCtxQueue)
        return false;

    Chunk_t *pChunk = (Chunk_t *)dynamicStack_pop(pState->pWorldState->chunkRenderer.pRemeshCtxQueue);
    if (!pChunk)
        return false;

    chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pChunk);

    Chunk_t **ppNeighbors = chunkManager_getChunkNeighbors(pState, pChunk->chunkPos);
    if (!ppNeighbors)
        return false;

    for (uint8_t i = 0; i < 6; i++)
    {
        Chunk_t *pN = ppNeighbors[i];
        if (pN)
            chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pN);
    }

    if (pChunk->pRenderChunk)
        pChunk->pRenderChunk->queuedForRemesh = false;

    free(ppNeighbors);
    return true;
}

void chunkRenderer_remeshChunks(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    size_t remeshCount = 0;

    const Vec3u8_t *pPOINTS = cmath_chunkPoints_Get();
    Vec3u8_t *pNEIGHBOR_BLOCK_POS = cmath_chunk_blockNeighborPoints_Get();
    bool *pNEIGHBOR_BLOCK_IN_CHUNK = cmath_chunk_blockNeighborPointsInChunkBool_Get();
    if (!pPOINTS || !pNEIGHBOR_BLOCK_POS || !pNEIGHBOR_BLOCK_IN_CHUNK)
        return;

    while (remeshCount < MAX_REMESH_PER_FRAME && chunkRenderer_dequeueRemesh(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK))
    {
        remeshCount++;
    }
}
#pragma endregion
#pragma region Create/Destroy
bool chunkRenderer_create(WorldState_t *pWorldState)
{
    if (!pWorldState)
        return false;

    pWorldState->chunkRenderer.pRemeshCtxQueue = dynamicStack_create(DEFAULT_QUEUE_SIZE);
    if (!pWorldState->chunkRenderer.pRemeshCtxQueue)
        return false;

    return true;
}

void chunkRenderer_destroy(WorldState_t *pWorldState)
{
    if (!pWorldState)
        return;

    dynamicStack_destroy(pWorldState->chunkRenderer.pRemeshCtxQueue);
}
#pragma endregion