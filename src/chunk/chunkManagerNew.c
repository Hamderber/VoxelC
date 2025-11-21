#pragma region Includes
#include <stdbool.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "api/chunk/chunkAPI.h"
#include "collection/linkedList_t.h"
#include "chunk/chunkManager_t.h"
#include "events/eventTypes.h"
#include "events/eventBus.h"
#include "world/world.h"
#include "chunk/chunk.h"
// TODO: Finish move to chunkManagerNew
#include "world/chunkManager.h"
#pragma endregion
#pragma region Defines
#if defined(DEBUG)
#define DEBUG_CHUNKMANAGER
#endif
#pragma endregion
#pragma region Chunk Registration
static size_t addedChunks = 0;
void chunkManager_chunk_register(ChunkManager_t *restrict pChunkManager, Chunk_t *restrict pChunk)
{
    if (!pChunkManager || !pChunk)
        return;

    LinkedList_t *pAdd = linkedList_data_addUnique(&pChunkManager->pChunksLL, (void *)pChunk);
    if (!pAdd)
    {
#if defined(DEBUG_CHUNKMANAGER)
        logs_log(LOG_ERROR, "Failed to add chunk %p to chunk manager %p's chunk linked list!", pChunk, pChunkManager);
#endif
        return;
    }
    addedChunks++;
}

void chunkManager_chunk_deregister(ChunkManager_t *restrict pChunkManager, Chunk_t *restrict pChunk)
{
    if (!pChunkManager || !pChunk)
        return;

    if (linkedList_data_remove(&pChunkManager->pChunksLL, (void *)pChunk))
    {
#if defined(DEBUG_CHUNKMANAGER)
        logs_log(LOG_ERROR, "Removed chunk %p from chunk manager %p's chunk linked list.", pChunk, pChunkManager);
#endif
        addedChunks--;
    }
}
#pragma endregion
#pragma region Events
EventResult_e chunkEvents_player_onChunkChange(State_t *pState, Event_t *pEvent, void *pCtx)
{
    // TODO: get unloaded chunks around player based off of simulation distance and load them
    if (!pState || !pEvent || !pEvent->data.pGeneric)
        return EVENT_RESULT_ERROR;

    uint32_t simDist = pState->pWorldConfig->chunkSimulationDistance;
    ChunkManager_t *pChunkManager = (ChunkManager_t *)pCtx;
    if (!pChunkManager)
        return EVENT_RESULT_ERROR;

    if (!pState || !pEvent)
        return EVENT_RESULT_ERROR;

    CtxChunk_t *pChunkEventData = pEvent->data.pChunkEvntData;
    Entity_t *pEntity = pChunkEventData->pEntitySource;
    Chunk_t *pChunk = pChunkEventData->pChunk;
    if (!pChunk)
        return EVENT_RESULT_ERROR;
    Vec3i_t chunkPos = pChunk->chunkPos;

#if defined(DEBUG_CHUNKMANAGER)
    logs_log(LOG_DEBUG, "Entity %p is now in chunk (%d, %d, %d) of ChunkManager %p.", pEntity,
             chunkPos.x, chunkPos.y, chunkPos.z, pChunkManager);
#endif

    world_chunks_load(pState, pEntity, chunkPos, simDist);

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Chunk API
bool chunkManager_chunks_aquire(ChunkManager_t *restrict pChunkManager, const Vec3i_t *restrict pCHUNK_POS, size_t count,
                                Chunk_t ***restrict pppNewChunks, size_t *restrict pNewCount, Chunk_t ***restrict pppExistingChunks,
                                size_t *restrict pExistingCount)
{
    if (!pChunkManager || !pCHUNK_POS || !pppNewChunks || !pNewCount || !pppExistingChunks || !pExistingCount)
    {
        if (pNewCount)
            *pNewCount = 0;
        if (pExistingCount)
            *pExistingCount = 0;
        return false;
    }

#if defined(DEBUG_CHUNKMANAGER)
    logs_log(LOG_DEBUG, "Trying to aquire %u chunks from chunk manager %p.", count, pChunkManager);
#endif

    // Max possible sizes = count
    Chunk_t **pNew = calloc(count, sizeof(Chunk_t *));
    Chunk_t **pExisting = calloc(count, sizeof(Chunk_t *));
    bool *pFound = calloc(count, sizeof(bool));

    if (!pNew || !pExisting || !pFound)
    {
        free(pNew);
        free(pExisting);
        free(pFound);
        *pNewCount = 0;
        *pExistingCount = 0;
        return false;
    }

    size_t existingCount = 0;
    size_t newCount = 0;

    // If there are existing chunks, walk the list once and match against requested positions
    for (LinkedList_t *pNode = pChunkManager->pChunksLL; pNode; pNode = pNode->pNext)
    {
        Chunk_t *pC = (Chunk_t *)pNode->pData;
        if (!pC)
            continue;

        const Vec3i_t EXISTING_POS = pC->chunkPos;

        for (size_t i = 0; i < count; ++i)
        {
            if (pFound[i])
                continue;

            const Vec3i_t REQ_POS = pCHUNK_POS[i];

            if (cmath_vec3i_equals(EXISTING_POS, REQ_POS, 0))
            {
                pExisting[existingCount++] = pC;
                pFound[i] = true;
                break;
            }
        }
    }

    // For any requested position not found above, create a new chunk
    for (size_t i = 0; i < count; ++i)
    {
        if (pFound[i])
            continue;

        const Vec3i_t CHUNK_POS = pCHUNK_POS[i];
        Chunk_t *pChunk = chunk_world_create(CHUNK_POS);
        if (!pChunk)
        {
            // handle cleanup during failure
            for (size_t j = 0; j < newCount; ++j)
                chunk_world_destroy(pNew[j]);

            free(pNew);
            free(pExisting);
            free(pFound);
            *pNewCount = 0;
            *pExistingCount = 0;
            return false;
        }

        chunkManager_chunk_register(pChunkManager, pChunk);

        pNew[newCount++] = pChunk;
    }

    free(pFound);

    if (existingCount > 0)
    {
        Chunk_t **pTemp = realloc(pExisting, sizeof(Chunk_t *) * existingCount);
        if (pTemp)
            pExisting = pTemp;
    }
    else
    {
        free(pExisting);
        pExisting = NULL;
    }

    if (newCount > 0)
    {
        Chunk_t **pTemp = realloc(pNew, sizeof(Chunk_t *) * newCount);
        if (pTemp)
            pNew = pTemp;
    }
    else
    {
        free(pNew);
        pNew = NULL;
    }

    *pppExistingChunks = pExisting;
    *pppNewChunks = pNew;
    *pExistingCount = existingCount;
    *pNewCount = newCount;

#if defined(DEBUG_CHUNKMANAGER)
    logs_log(LOG_DEBUG, "Aquired %u new chunk(s) and %u existing chunk(s) from chunk manager %p",
             *pNewCount, *pExistingCount, pChunkManager);
#endif
    return true;
}

bool chunkManager_chunks_populateNew(State_t *pState, ChunkManager_t *pChunkManager, ChunkSource_t *pSource,
                                     Chunk_t **ppNewChunks, size_t count)
{
    Chunk_t **ppOutChunksBad = NULL;
    size_t outCount = 0;

    if (chunkSource_loadChunks(pSource, ppNewChunks, count, &ppOutChunksBad, &outCount))
    {
#if defined(DEBUG_CHUNKMANAGER)
        logs_log(LOG_DEBUG, "Loaded %zu chunks (%zu failed) from chunk manager %p.", count - outCount, outCount, pChunkManager);
#endif
        if (outCount > 0)
        {
#if defined(DEBUG_CHUNKMANAGER)
            logs_log(LOG_DEBUG, "Failed to load %zu chunks from chunk manager %p. Destroying those chunks.", outCount, pChunkManager);
#endif
            for (size_t i = 0; i < outCount; i++)
            {
                chunkManager_chunk_deregister(pChunkManager, ppOutChunksBad[i]);
                // Don't worry about the render side because this must pass first to get tangible data for the rendering side
                // to do anything
                chunk_destroy(pState, ppOutChunksBad[i]);
            }
            free(ppOutChunksBad);
        }
        return true;
    }
    else
    {
#if defined(DEBUG_CHUNKMANAGER)
        logs_log(LOG_DEBUG, "Chunk population failed for %u chunks with chunk manager %p.", count, pChunkManager);
#endif
        free(ppOutChunksBad);
        return false;
    }
}
#pragma endregion
#pragma region Create/Destroy
ChunkManager_t *chunkManager_createNew(State_t *pState)
{
    ChunkManager_t *pChunkManager = calloc(1, sizeof(ChunkManager_t));
    if (!pChunkManager)
    {
        logs_log(LOG_ERROR, "Failed to create chunk manager!");
        return NULL;
    }

    pChunkManager->pChunksLL = linkedList_create();

    void *pSUB_CTX = (void *)pChunkManager;
    const bool CONSUME_LISTENER = false;
    const bool CONSUME_EVENT = false;
    if (events_subscribe(&pState->eventBus, EVENT_CHANNEL_CHUNK, chunkEvents_player_onChunkChange,
                         CONSUME_LISTENER, CONSUME_EVENT, pSUB_CTX) != EVENT_SUBSCRIBE_RESULT_PASS)
    {
        logs_log(LOG_ERROR, "Failed to subscribe chunk manager to events!");
        linkedList_destroy(&pChunkManager->pChunksLL, NULL, NULL);
        free(pChunkManager);
        return NULL;
    }

    return pChunkManager;
}

void chunkManager_destroyNew(State_t *restrict pState, ChunkManager_t *restrict pChunkManager)
{
    if (!pChunkManager)
        return;

#if defined(DEBUG_CHUNKMANAGER)
    logs_log(LOG_DEBUG, "%d chunk(s) were still registered with chunk manager %p at the chunk manager's end-of-life.",
             addedChunks, pChunkManager);
#endif

    events_unsubscribe(&pState->eventBus, EVENT_CHANNEL_CHUNK, chunkEvents_player_onChunkChange);

    linkedList_destroy(&pChunkManager->pChunksLL, chunk_destroy, pState);
}
#pragma endregion