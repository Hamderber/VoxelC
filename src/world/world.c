#pragma region Includes
#include <string.h>
#include <stdlib.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "world/world_t.h"
#include "character/characterType_t.h"
#include "character/character.h"
#include "rendering/chunk/chunkRendering.h"
#include "chunk.h"
#include "core/random.h"
#include "world/chunkManager.h"
#pragma endregion
#pragma region Defrag
void world_chunk_defrag(State_t *pState)
{
    if (!pState || !pState->pWorldState || !pState->pWorldState->ppChunks)
        return;

    // Read and write indexes increment in unison until a null spot is found. Every time there is a null spot, only read index increments.
    // Because of this, when the next existing chunk is found, that next write index is used and then finally incremented
    size_t writeIndex = 0;
    for (size_t readIndex = 0; readIndex < pState->pWorldState->chunkCapacity; readIndex++)
    {
        Chunk_t *pC = pState->pWorldState->ppChunks[readIndex];
        if (pC)
        {
            if (writeIndex != readIndex)
            {
                pState->pWorldState->ppChunks[writeIndex] = pC;
                pState->pWorldState->ppChunks[readIndex] = NULL;
            }

            writeIndex++;
        }
    }

    pState->pWorldState->chunkCount = (uint32_t)writeIndex;

    if (pState->pWorldState->chunkCount == pState->pWorldState->chunkCapacity)
    {
        // A resize will never actually happen if 0 becase 0*2 = 0
        static const uint32_t FALLBACK_CHUNK_CAPACITY = 64;
        if (pState->pWorldState->chunkCapacity == 0)
            pState->pWorldState->chunkCapacity = FALLBACK_CHUNK_CAPACITY;

        size_t newCap = pState->pWorldState->chunkCapacity * 2;
        Chunk_t **ppChunksNew = realloc(pState->pWorldState->ppChunks, sizeof(Chunk_t *) * newCap);
        if (ppChunksNew)
        {
            pState->pWorldState->ppChunks = ppChunksNew;
            pState->pWorldState->chunkCapacity = (uint32_t)newCap;

            memset(pState->pWorldState->ppChunks + pState->pWorldState->chunkCount, 0,
                   sizeof(Chunk_t *) * (pState->pWorldState->chunkCapacity - pState->pWorldState->chunkCount));
        }
    }
}
#pragma endregion
#pragma region Add Chunk to Col.
void world_chunk_addToCollection(State_t *pState, Chunk_t *pChunk)
{
    if (!pState || !pChunk)
        return;

    size_t slot = pState->pWorldState->chunkCount;
    while (slot < pState->pWorldState->chunkCapacity && pState->pWorldState->ppChunks[slot] != NULL)
    {
        slot++;
        if (slot >= pState->pWorldState->chunkCapacity || pState->pWorldState->chunkCount == pState->pWorldState->chunkCapacity)
        {
            world_chunk_defrag(pState);
            slot = pState->pWorldState->chunkCount;
        }
    }

    pState->pWorldState->ppChunks[slot] = pChunk;
    pState->pWorldState->chunkCount++;
}
#pragma endregion
#pragma region Chunks Init
static void world_chunks_init(State_t *pState)
{
    int chunksPerAxis = 6;
    uint32_t chunkCount = chunksPerAxis * chunksPerAxis * chunksPerAxis;
    pState->pWorldState->chunkCapacity = chunkCount;
    pState->pWorldState->ppChunks = calloc(pState->pWorldState->chunkCapacity, sizeof(Chunk_t *));

    ChunkPos_t *pChunkPos = calloc(pState->pWorldState->chunkCapacity, sizeof(ChunkPos_t));

    for (int i = 0; i < chunksPerAxis; i++)
        for (int j = 0; j < chunksPerAxis; j++)
            for (int k = 0; k < chunksPerAxis; k++)
            {
                size_t idx = (size_t)i * chunksPerAxis * chunksPerAxis + (size_t)j * chunksPerAxis + (size_t)k;
                ChunkPos_t pos = {i, j, k};
                pChunkPos[idx] = pos;
            }

    chunkManager_chunk_createBatch(pState, pChunkPos, chunkCount);

    free(pChunkPos);
}
#pragma endregion
#pragma region Create
static void Init(State_t *pState)
{
    pState->pWorldState = calloc(1, sizeof(WorldState_t));

    pState->pWorldState->world.pPlayer = character_create(pState, CHARACTER_TYPE_PLAYER);

    world_chunks_init(pState);
}

void world_load(State_t *pState)
{
    Init(pState);
}
#pragma endregion
#pragma region Destroy
void world_destroy(State_t *pState)
{
    if (!pState || !pState->pWorldState || !pState->pWorldState->ppChunks)
        return;

    // Ensure nothing is in-flight that still uses these buffers
    vkDeviceWaitIdle(pState->context.device);

    size_t count = (size_t)pState->pWorldState->chunkCapacity;
    for (size_t i = 0; i < count; ++i)
    {
        if (!pState->pWorldState->ppChunks[i])
            continue;

        RenderChunk_t *pRenderChunk = pState->pWorldState->ppChunks[i]->pRenderChunk;
        if (!pRenderChunk)
            continue;

        chunk_renderDestroy(pState, pRenderChunk);
        pState->pWorldState->ppChunks[i] = NULL;
    }

    free(pState->pWorldState->ppChunks);
    pState->pWorldState->ppChunks = NULL;
    pState->pWorldState = NULL;
}
#pragma endregion