#pragma region Includes
#include <stdbool.h>
#include "core/types/state_t.h"
#include "world/worldState_t.h"
#include "collection/dynamicStack_t.h"
#include "rendering/chunk/chunkRendering.h"
#include "world/chunkManager.h"
#include "core/cpuManager.h"
#include "api/chunk/chunkAPI.h"
#pragma endregion
#pragma region Defines
#define DEFAULT_QUEUE_SIZE 32
#define MAX_REMESH_PER_FRAME 5
#pragma endregion
#pragma region Operations
// TODO: Change this whole thing to use a FIFO appraoch vice stack
bool chunkRenderer_enqueueRemesh(WorldState_t *restrict pWorldState, Chunk_t *restrict pChunk)
{
    if (!pWorldState || !pWorldState->chunkRenderer.pRemeshCtxQueue || !pChunk)
        return false;

    RenderChunk_t *pRenderChunk = pChunk->pRenderChunk;

    // Avoid duplicates
    if (pRenderChunk && pRenderChunk->needsRemesh)
        return true;

    if (dynamicStack_pushUnique(pWorldState->chunkRenderer.pRemeshCtxQueue, pChunk))
        if (pRenderChunk)
            pRenderChunk->needsRemesh = true;

    return true;
}

static bool chunkRenderer_meshBatch(State_t *restrict pState, const uint32_t BATCH_SIZE, Vec3u8_t *restrict pPOINTS,
                                    const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                                    const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK)
{
    // Max size is assuming each chunk in this batch somehow has no neighbor overlaps, so each remeshed chunk actually causes
    // itself + its 6 neighbors to be meshed
    size_t MAX_SIZE = BATCH_SIZE * 7;
    DynamicStack_t *pStack = dynamicStack_create(MAX_SIZE);
    if (!pStack)
        return false;

    uint32_t chunkPosIndex = 0;
    Vec3i_t *pChunkPos = malloc(sizeof(Vec3i_t) * BATCH_SIZE);
    if (!pChunkPos)
    {
        dynamicStack_destroy(pStack);
        return false;
    }

    for (uint32_t i = 0; i < BATCH_SIZE; i++)
    {
        Chunk_t *pChunk = (Chunk_t *)dynamicStack_pop(pState->pWorldState->chunkRenderer.pRemeshCtxQueue);
        if (!pChunk)
            continue;

        dynamicStack_pushUnique(pStack, pChunk);
        pChunkPos[chunkPosIndex++] = pChunk->chunkPos;
    }

    if (chunkPosIndex == 0)
    {
        free(pChunkPos);
        dynamicStack_destroy(pStack);
        return false;
    }

    size_t neighborCount = 0;
    Vec3i_t *pNeighborPos = cmath_chunk_GetNeighborsPosUnique_get(pChunkPos, chunkPosIndex, &neighborCount);
    Chunk_t **ppNeighbors = chunkManager_getChunks(pState, pNeighborPos, neighborCount, &neighborCount, true);

    if (neighborCount > 0)
        for (uint32_t i = 0; i < neighborCount; i++)
            dynamicStack_pushUnique(pStack, ppNeighbors[i]);

    free(pChunkPos);
    free(pNeighborPos);
    free(ppNeighbors);

    Chunk_t *pRemesh = NULL;
    do
    {
        pRemesh = (Chunk_t *)dynamicStack_pop(pStack);
        if (!pRemesh)
            break;

        chunkRenderer_chunk_remesh(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pRemesh);
    } while (pRemesh);

    dynamicStack_destroy(pStack);
    return true;
}

void chunkRenderer_remeshChunks(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    Vec3u8_t *pPOINTS = cmath_chunkPoints_Get();
    Vec3u8_t *pNEIGHBOR_BLOCK_POS = cmath_chunk_blockNeighborPoints_Get();
    bool *pNEIGHBOR_BLOCK_IN_CHUNK = cmath_chunk_blockNeighborPointsInChunkBool_Get();
    if (!pPOINTS || !pNEIGHBOR_BLOCK_POS || !pNEIGHBOR_BLOCK_IN_CHUNK)
        return;

    uint32_t batchSize = MAX_REMESH_PER_FRAME;
    // If the CPU is overloaded, remesh the bare minimum
    if (cpuManager_lightenTheLoad(pState) && pState->renderer.currentFrame % 2 == 0)
        batchSize = 1;

    chunkRenderer_meshBatch(pState, batchSize, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK);
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