#pragma region Includes
#include <stdbool.h>
#include <string.h>
#include "core/types/state_t.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/types/shaderVertexVoxel_t.h"
#include "chunkManager.h"
#include "world/chunkSolidityGrid.h"
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
#include "api/chunk/chunkAPI.h"
#include "chunk/chunkSource_local.h"
#include "chunk/chunkManager_t.h"
#pragma endregion
#pragma region Defines
#if defined(DEBUG)
#define DEBUG_CHUNKMANAGER
#endif
#pragma endregion
#pragma region Get Chunk
Chunk_t *chunkManager_getChunk(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    if (!pSTATE || !pSTATE->pWorldState)
    {
        logs_log(LOG_ERROR, "getChunk: bad state");
        return NULL;
    }
    if (!pSTATE->pWorldState->pChunkManager->pChunksLL)
    {
        logs_log(LOG_ERROR, "getChunk: chunk list is undefined");
        return NULL;
    }

    for (LinkedList_t *pNode = pSTATE->pWorldState->pChunkManager->pChunksLL; pNode; pNode = pNode->pNext)
    {
        if (!pNode->pData)
            continue;
        const Chunk_t *pC = (const Chunk_t *)pNode->pData;
        if (cmath_vec3i_equals(pC->chunkPos, CHUNK_POS, 0))
            return (Chunk_t *)pC;
    }

#if defined(DEBUG_CHUNKMANAGER)
    logs_log(LOG_DEBUG, "Failed to get chunk at (%d, %d, %d) belonging to chunk manager %p.", CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z,
             pSTATE->pWorldState->pChunkManager);
#endif

    return NULL;
}

Chunk_t **chunkManager_getChunks(const State_t *restrict pSTATE, const Vec3i_t *restrict pCHUNK_POS, const size_t numChunkPos,
                                 size_t *restrict pCount, const bool RESIZE)
{
    if (!pSTATE || !pCHUNK_POS || !pCount)
    {
        if (pCount)
            *pCount = 0;
        return NULL;
    }

    Chunk_t **ppChunks = calloc(numChunkPos, sizeof(Chunk_t *));
    if (!ppChunks)
    {
        *pCount = 0;
        return NULL;
    }

    size_t index = 0;

    // cheaper to iterate chunk pos as main loop because the collection of chunks is bigger than the collection of points
    for (size_t i = 0; i < numChunkPos; i++)
    {
        for (LinkedList_t *pNode = pSTATE->pWorldState->pChunkManager->pChunksLL; pNode; pNode = pNode->pNext)
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

    if (RESIZE)
    {
        ppChunks = realloc(ppChunks, sizeof(Chunk_t *) * index);
        if (!ppChunks)
        {
            *pCount = 0;
            free(ppChunks);
            ppChunks = NULL;
            return NULL;
        }
    }

    *pCount = index;

    return ppChunks;
}

Chunk_t **chunkManager_getChunkNeighbors(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    Vec3i_t *pPOS = cmath_chunk_chunkNeighborPos_get(CHUNK_POS);
    if (!pPOS)
        return NULL;

    size_t count = 0;
    const bool RESIZE = false;
    Chunk_t **ppNEIGHBORS_UNSORTED = chunkManager_getChunks(pSTATE, pPOS, CMATH_GEOM_CUBE_FACES, &count, RESIZE);
    free(pPOS);

    if (!ppNEIGHBORS_UNSORTED)
        return NULL;

    Chunk_t **ppFace = calloc(CMATH_GEOM_CUBE_FACES, sizeof(Chunk_t *));
    if (!ppFace)
    {
        free(ppNEIGHBORS_UNSORTED);
        return NULL;
    }

    for (size_t i = 0; i < count; i++)
    {
        Chunk_t *pN = ppNEIGHBORS_UNSORTED[i];
        if (!pN)
            continue;

        Vec3i_t delta = cmath_vec3i_sub_vec3i(pN->chunkPos, CHUNK_POS);

        for (int face = 0; face < CMATH_GEOM_CUBE_FACES; face++)
        {
            Vec3i_t offset = pCMATH_CUBE_NEIGHBOR_OFFSETS[face];

            const int TOLERANCE = 0;
            if (cmath_vec3i_equals(delta, offset, TOLERANCE))
            {
                ppFace[face] = pN;
                break;
            }
        }
    }

    free(ppNEIGHBORS_UNSORTED);
    return ppFace;
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
            linkedList_data_addUnique(&ppChunks[i]->pEntitiesLoadingChunkLL, (void *)(pEntity));
        else
        {
            logs_log(LOG_ERROR, "Attempted to add a loading entity to a NULL chunk! That chunk was skipped.");
            continue;
        }
    }

    return true;
}

bool chunk_isLoaded(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    // logs_log(LOG_DEBUG, "Checking if chunk (%d, %d, %d) is loaded.", CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
    return chunkManager_getChunk(pSTATE, CHUNK_POS) != NULL;
}
#pragma endregion