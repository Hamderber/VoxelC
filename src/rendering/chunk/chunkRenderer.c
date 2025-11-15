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

    Chunk_t *pChunk = pCtx->pChunk;
    chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pChunk);

    for (uint8_t i = 0; i < pCtx->loadedNeighborCount; i++)
    {
        Chunk_t *pN = pCtx->ppLoadedNeighbors[i];
        if (pN)
            chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pN);
    }

    if (pChunk->pRenderChunk)
        pChunk->pRenderChunk->queuedForRemesh = false;

    remeshContext_destroy(pCtx);
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
#pragma region Undefines

#pragma endregion