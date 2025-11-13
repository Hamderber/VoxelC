#pragma region Includes
#include <stdbool.h>
#include <string.h>
#include "core/types/state_t.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/types/shaderVertexVoxel_t.h"
#include "world/chunk.h"
#include "chunkManager.h"
#include "rendering/uvs.h"
#include "rendering/chunk/chunkRendering.h"
#include "core/random.h"
#include "world/world.h"
#include "core/randomNoise.h"
#include "cmath/weightedMap_t.h"
#include "chunkGenerator.h"
#include "core/logs.h"
#include "cmath/weightedMaps.h"
#include "events/eventBus.h"
#pragma endregion
#pragma region Get Chunk
Chunk_t *chunkManager_getChunk(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    if (!pSTATE || !pSTATE->pWorldState)
    {
        logs_log(LOG_ERROR, "getChunk: bad state");
        return NULL;
    }
    if (!pSTATE->pWorldState->pChunksLL)
    {
        logs_log(LOG_ERROR, "getChunk: chunk list is empty");
        return NULL;
    }

    for (LinkedList_t *pNode = pSTATE->pWorldState->pChunksLL; pNode; pNode = pNode->pNext)
    {
        if (!pNode->pData)
            continue;
        const Chunk_t *pC = (const Chunk_t *)pNode->pData;
        if (cmath_vec3i_equals(pC->chunkPos, CHUNK_POS, 0))
            return (Chunk_t *)pC;
    }
    return NULL;
}

/// @brief Iterates the provided chunk positions and returns a heap array of chunk positions matching query from that collection.
/// Places the length of that collection into pCount. If there are duplicate chunkpos in the query then there will be duplicate
/// results.
static Chunk_t **chunks_getChunks(State_t *restrict pSTATE, const Vec3i_t *restrict pCHUNK_POS, const size_t numChunkPos,
                                  size_t *pCount)
{
    if (!pSTATE || !pCHUNK_POS || !pCount)
    {
        if (pCount)
            *pCount = 0;
        return NULL;
    }

    Chunk_t **ppChunks = malloc(sizeof(Chunk_t *) * numChunkPos);
    if (!ppChunks)
    {
        *pCount = 0;
        return NULL;
    }

    size_t index = 0;

    // cheaper to iterate chunk pos as main loop because the collection of chunks is bigger than the collection of points
    for (size_t i = 0; i < numChunkPos; i++)
    {
        for (LinkedList_t *pNode = pSTATE->pWorldState->pChunksLL; pNode; pNode = pNode->pNext)
        {
            if (!pNode->pData)
                continue;

            Chunk_t *pC = (Chunk_t *)pNode->pData;
            const int TOLERANCE = 0;
            if (cmath_vec3i_equals(pC->chunkPos, pCHUNK_POS[i], TOLERANCE))
            {
                bool chunkIsLoaded = chunk_isLoaded(pSTATE, pCHUNK_POS[i]);
                if (chunkIsLoaded)
                {
                    ppChunks[index++] = pC;
                    break;
                }
            }
        }
    }

    ppChunks = realloc(ppChunks, sizeof(Chunk_t *) * index);
    if (!ppChunks)
    {
        *pCount = 0;
        free(ppChunks);
        ppChunks = NULL;
        return NULL;
    }

    *pCount = index;

    return ppChunks;
}

Chunk_t **chunkManager_getChunkNeighbors(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    Chunk_t **ppChunks = malloc(sizeof(Chunk_t *) * CUBE_FACE_COUNT);
    if (!ppChunks)
        return NULL;

    for (int cubeFace = 0; cubeFace < CUBE_FACE_COUNT; ++cubeFace)
    {
        int nx = CHUNK_POS.x + pCMATH_CUBE_NEIGHBOR_OFFSETS[cubeFace].x;
        int ny = CHUNK_POS.y + pCMATH_CUBE_NEIGHBOR_OFFSETS[cubeFace].y;
        int nz = CHUNK_POS.z + pCMATH_CUBE_NEIGHBOR_OFFSETS[cubeFace].z;

        Vec3i_t nChunkPos = {nx, ny, nz};
        ppChunks[cubeFace] = chunkManager_getChunk(pSTATE, nChunkPos);
    }

    return ppChunks;
}
#pragma endregion
#pragma region Allocate Voxels
static Chunk_t *chunk_voxels_allocate(const State_t *pSTATE, const BlockDefinition_t *const *pBLOCK_DEFINITIONS, const Vec3i_t CHUNK_POS)
{
    Chunk_t *pChunk = malloc(sizeof(Chunk_t));
    if (!pChunk)
        return NULL;

    pChunk->pBlockVoxels = calloc(CHUNK_BLOCK_CAPACITY, sizeof(BlockVoxel_t));
    pChunk->chunkPos = CHUNK_POS;
    pChunk->pRenderChunk = NULL;

    if (!chunkGen_genChunk(pSTATE, pBLOCK_DEFINITIONS, pChunk))
        return NULL;

    return pChunk;
}
#pragma endregion
#pragma region Create Chunk
/// @brief Iterates the provided chunk positions and returns a heap array of chunk positions matching query from that collection.
/// Places the length of that collection into pCount
static bool chunks_getChunksPos(State_t *restrict pState, const Vec3i_t *restrict pCHUNK_POS, const size_t COUNT_MAX,
                                Vec3i_t **restrict ppUnloadedPos, size_t *restrict pUnloadedCount,
                                Vec3i_t **restrict ppLoadedPos, size_t *restrict pLoadedCount, const ChunkQuery_e QUERY)
{
    if (!pState || !pCHUNK_POS || !pLoadedCount || !pUnloadedCount)
    {
        *pLoadedCount = 0;
        *pUnloadedCount = 0;
        return false;
    }

    switch (QUERY)
    {
    case QUERY_CHUNK_LOADED:
        *ppLoadedPos = calloc(COUNT_MAX, sizeof(Vec3i_t));

        if (!*ppLoadedPos)
        {
            *pLoadedCount = 0;
            *pUnloadedCount = 0;
            return false;
        }
        break;
    case QUERY_CHUNK_UNLOADED:
        *ppUnloadedPos = calloc(COUNT_MAX, sizeof(Vec3i_t));

        if (!*ppUnloadedPos)
        {
            *pLoadedCount = 0;
            *pUnloadedCount = 0;
            return false;
        }
        break;
    case QUERY_CHUNK_LOADED_AND_UNLOADED:
        *ppLoadedPos = calloc(COUNT_MAX, sizeof(Vec3i_t));
        *ppUnloadedPos = calloc(COUNT_MAX, sizeof(Vec3i_t));

        if (!*ppLoadedPos || !*ppUnloadedPos)
        {
            *pLoadedCount = 0;
            *pUnloadedCount = 0;
            return false;
        }
        break;
    default:
        return false;
        break;
    }

    size_t loadedCount = 0;
    size_t unloadedCount = 0;
    bool fail = false;
    do
    {
        for (size_t i = 0; i < COUNT_MAX && !fail; i++)
        {
            bool chunkIsLoaded = chunk_isLoaded(pState, pCHUNK_POS[i]);
            switch (QUERY)
            {
            case QUERY_CHUNK_LOADED:
                if (chunkIsLoaded)
                    (*ppLoadedPos)[loadedCount++] = pCHUNK_POS[i];
                break;
            case QUERY_CHUNK_UNLOADED:
                if (!chunkIsLoaded)
                    (*ppUnloadedPos)[unloadedCount++] = pCHUNK_POS[i];
                break;
            case QUERY_CHUNK_LOADED_AND_UNLOADED:
                if (chunkIsLoaded)
                    (*ppLoadedPos)[loadedCount++] = pCHUNK_POS[i];
                else
                    (*ppUnloadedPos)[unloadedCount++] = pCHUNK_POS[i];
                break;
            default:
                fail = true;
                break;
            }
        }

        if (fail)
            break;

        *ppLoadedPos = realloc(*ppLoadedPos, sizeof(Vec3i_t) * loadedCount);
        *ppUnloadedPos = realloc(*ppUnloadedPos, sizeof(Vec3i_t) * unloadedCount);
        *pLoadedCount = loadedCount;
        *pUnloadedCount = unloadedCount;

        return true;
    } while (0);

    free(*ppLoadedPos);
    free(*ppUnloadedPos);
    *pLoadedCount = 0;
    *pUnloadedCount = 0;

    logs_log(LOG_ERROR, "Failed to query chunk(s)!");

    return false;
}

Chunk_t **chunkManager_chunk_createBatch(State_t *restrict pState, const Vec3i_t *restrict pCHUNK_POS, const size_t COUNT_MAX,
                                         Vec3i_t *restrict pChunkPosUnloaded, size_t *restrict pUnloadedCount,
                                         Vec3i_t *restrict pChunkPosLoaded, size_t *restrict pLoadedCount)
{
    if (!pState || !pCHUNK_POS)
        return NULL;

    chunks_getChunksPos(pState, pCHUNK_POS, COUNT_MAX,
                        &pChunkPosUnloaded, pUnloadedCount,
                        &pChunkPosLoaded, pLoadedCount, QUERY_CHUNK_LOADED_AND_UNLOADED);

    Chunk_t **ppNewChunks = calloc(*pUnloadedCount, sizeof(Chunk_t *));
    const BlockDefinition_t *const *pBLOCK_DEFINITIONS = block_defs_getAll();
    if (!ppNewChunks || !pBLOCK_DEFINITIONS)
    {
        logs_log(LOG_ERROR, "Failed to allocate memory for new chunnks!");
        *pUnloadedCount = 0;
        free(ppNewChunks);
        ppNewChunks = NULL;
        return NULL;
    }

    for (size_t i = 0; i < *pUnloadedCount; i++)
    {
        ppNewChunks[i] = chunk_voxels_allocate(pState, pBLOCK_DEFINITIONS, pChunkPosUnloaded[i]);
        ppNewChunks[i]->pEntitiesLoadingChunkLL = linkedList_root();
        world_chunk_addToCollection(pState, ppNewChunks[i]);
    }

    return ppNewChunks;
}

bool chunkManager_chunk_addLoadingEntity(Chunk_t **ppChunks, size_t numChunks, Entity_t *pEntity)
{
    if (numChunks == 0)
        return false;

    if (!ppChunks)
    {
        logs_log(LOG_ERROR, "Attempted to add a loading entity to chunk(s) with an invalid collection of chunks!");
        return false;
    }

    if (!pEntity)
    {
        logs_log(LOG_WARN, "Attempted to add a loading entity of NULL to chunk(s)! If you were trying to permanently load \
chunk(s), use the correct (not this one) function for that instead.");
        return false;
    }

    for (size_t i = 0; i < numChunks; i++)
    {
        if (ppChunks[i])
            linkedList_data_add(&ppChunks[i]->pEntitiesLoadingChunkLL, (void *)(pEntity));
        else
        {
            logs_log(LOG_ERROR, "Attempted to add a loading entity to a NULL chunk! That chunk was skipped.");
            continue;
        }
    }

    return true;
}

bool chunkManager_chunk_permanentlyLoad(State_t *pState, Chunk_t **ppChunks, size_t numChunks)
{
    if (numChunks == 0)
        return false;

    if (!ppChunks)
    {
        logs_log(LOG_ERROR, "Attempted to permanently load an invalid collection of chunks!");
        return false;
    }

    if (!pState || !pState->pWorldState || !pState->pWorldState->pChunkLoadingEntity)
    {
        logs_log(LOG_ERROR, "Attempted to permanently load chunk(s) with an invalid state reference!");
        return false;
    }

    for (size_t i = 0; i < numChunks; i++)
    {
        if (ppChunks[i] && ppChunks[i]->pEntitiesLoadingChunkLL)
            linkedList_data_addUnique(&ppChunks[i]->pEntitiesLoadingChunkLL, (void *)(pState->pWorldState->pChunkLoadingEntity));
        else
        {
            logs_log(LOG_ERROR, "Attempted to permanently load a NULL chunk or a chunk with NULL members! That chunk was skipped.");
            continue;
        }
    }

    return true;
}

#pragma endregion
#pragma region Destroy Chunk
void chunk_entitiesLoadingLL_destroy(Chunk_t *pChunk)
{
    if (!pChunk)
        return;

    linkedList_destroy(&pChunk->pEntitiesLoadingChunkLL, NULL, NULL);

    pChunk->pEntitiesLoadingChunkLL = NULL;
}

void chunk_destroy(void *pCtx, Chunk_t *pChunk)
{
    // Cast ctx to state so this can be used in a linked list destructor
    if (!pCtx || !pChunk)
        return;

    State_t *pState = (State_t *)pCtx;
    chunk_renderDestroy(pState, pChunk->pRenderChunk);
    chunk_entitiesLoadingLL_destroy(pChunk);
    free(pChunk->pBlockVoxels);
    chunkSolidityGrid_destroy(pChunk->pTransparencyGrid);
}
#pragma endregion
#pragma region Chunk (Un)Load
bool chunk_isLoaded(State_t *pState, const Vec3i_t CHUNK_POS)
{
    // logs_log(LOG_DEBUG, "Checking if chunk (%d, %d, %d) is loaded.", CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
    return chunkManager_getChunk(pState, CHUNK_POS) != NULL;
}

EventResult_t player_onChunkChange(State_t *pState, Event_t *pEvent, void *pCtx)
{
    // TODO: get unloaded chunks around player based off of simulation distance and load them
    if (!pState || !pEvent || !pEvent->data.pGeneric)
        return EVENT_RESULT_ERROR;

    uint32_t simDist = pState->worldConfig.chunkSimulationDistance;
    pCtx;

    if (!pState || !pEvent)
        return EVENT_RESULT_ERROR;

    CtxChunk_t *pChunkEventData = pEvent->data.pChunkEvntData;
    Entity_t *pEntity = pChunkEventData->pEntitySource;
    Chunk_t *pChunk = pChunkEventData->pChunk;
    if (!pChunk)
        return EVENT_RESULT_ERROR;
    Vec3i_t chunkPos = pChunk->chunkPos;

    logs_log(LOG_DEBUG, "Entity %p is now in chunk (%d, %d, %d).", pEntity, chunkPos.x, chunkPos.y, chunkPos.z);

    world_chunks_load(pState, pEntity, chunkPos, simDist);

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Create
void chunkManager_create(State_t *pState)
{
    const void *pSUB_CTX = NULL;
    static const bool CONSUME_LISTENER = false;
    static const bool CONSUME_EVENT = false;
    events_subscribe(&pState->eventBus, EVENT_CHANNEL_CHUNK, player_onChunkChange, CONSUME_LISTENER, CONSUME_EVENT, &pSUB_CTX);
}
#pragma endregion
#pragma region Destroy
void chunkManager_destroy(State_t *pState)
{
    events_unsubscribe(&pState->eventBus, EVENT_CHANNEL_CHUNK, player_onChunkChange);

    linkedList_destroy(&pState->pWorldState->pChunksLL, chunk_destroy, pState);
}
#pragma endregion