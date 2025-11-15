#pragma region Includes
#include <stdbool.h>
#include "core/types/state_t.h"
#include "world/worldState_t.h"
#include "rendering/types/chunkRemeshCtx_t.h"
#include "collection/dynamicStack_t.h"
#include "rendering/chunk/chunkRendering.h"
#pragma endregion
#pragma region Defines
#define DEFAULT_QUEUE_SIZE 32
#define MAX_REMESH_PER_FRAME 1
#pragma endregion
#pragma region Operations
bool chunkRenderer_enqueueRemesh(WorldState_t *restrict pWorldState, ChunkRemeshCtx_t *restrict pCtx)
{
    if (!pWorldState || !pWorldState->chunkRenderer.pRemeshCtxQueue || !pCtx)
        return false;

    dynamicStack_push(pWorldState->chunkRenderer.pRemeshCtxQueue, pCtx);

    return true;
}

static bool chunkRenderer_dequeueRemesh(State_t *restrict pState, const Vec3u8_t *restrict pPOINTS,
                                        const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                                        const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK)
{
    if (!pState || !pState->pWorldState || !pState->pWorldState->chunkRenderer.pRemeshCtxQueue)
        return false;

    ChunkRemeshCtx_t *pCtx = (ChunkRemeshCtx_t *)dynamicStack_pop(pState->pWorldState->chunkRenderer.pRemeshCtxQueue);
    if (!pCtx || !pCtx->pChunk || !pCtx->ppLoadedNeighbors)
        return false;

    // If the chunk exists but the rendermesh doesn't, then this remesh is just the first time that it has been created. (lazy)
    if (!pCtx->pChunk->pRenderChunk)
        chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pCtx->pChunk);

    Chunk_t *pChunk = pCtx->pChunk;
    // logs_log(LOG_DEBUG, "Remesh neighbors for Chunk at (%d, %d, %d)", pChunk->chunkPos.x, pChunk->chunkPos.y, pChunk->chunkPos.z);

    Chunk_t *pN = NULL;
    for (uint8_t i = 0; i < pCtx->loadedNeighborCount; i++)
    {
        pN = pCtx->ppLoadedNeighbors[i];
        if (pN)
            chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pN);
        // logs_log(LOG_DEBUG, "Remesh chunk at (%d, %d, %d)", pN->chunkPos.x, pN->chunkPos.y, pN->chunkPos.z);
    }

    pChunk->pRenderChunk->queuedForRemesh = false;
    remeshContext_destroy(pCtx);
    return true;
}

void chunkRenderer_remeshChunks(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    size_t remeshCount = 0;
    bool cont = true;

    const Vec3u8_t *pPOINTS = cmath_chunkPoints_Get();
    Vec3u8_t *pNEIGHBOR_BLOCK_POS = cmath_chunk_blockNeighborPoints_Get();
    bool *pNEIGHBOR_BLOCK_IN_CHUNK = cmath_chunk_blockNeighborPointsInChunkBool_Get();
    if (!pPOINTS || !pNEIGHBOR_BLOCK_POS || !pNEIGHBOR_BLOCK_IN_CHUNK)
        return;

    // vkDeviceWaitIdle(pState->context.device);

    while (cont)
    {
        cont = chunkRenderer_dequeueRemesh(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK) &&
               remeshCount < MAX_REMESH_PER_FRAME;
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
#pragma region Undefines

#pragma endregion