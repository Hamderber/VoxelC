#pragma region Includes
#include <string.h>
#include <stdlib.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "world/world_t.h"
#include "world/worldState_t.h"
#include "character/characterType_t.h"
#include "character/character.h"
#include "rendering/chunk/chunkRendering.h"
#include "chunk.h"
#include "core/random.h"
#include "world/chunkManager.h"
#include "chunkGenerator.h"
#include "rendering/chunk/chunkRenderer.h"
#pragma endregion
#pragma region Add Chunk to Col.
static size_t addedChunks = 0;
void world_chunk_addToCollection(State_t *pState, Chunk_t *pChunk)
{
    if (!pState || !pChunk)
        return;

    LinkedList_t *pAdd = linkedList_data_add(&pState->pWorldState->pChunksLL, (void *)pChunk);
    if (!pAdd)
    {
        logs_log(LOG_ERROR, "Failed to add chunk %p to the world's chunk linked list!", pChunk);
        return;
    }
    addedChunks++;
}
#pragma endregion
#pragma region Chunks Init
static void spawn_generate(State_t *pState)
{
    size_t size;
    const Vec3i_t SPAWN_ORIGIN = VEC3I_ZERO;
    const int SPAWN_RAD = pState->worldConfig.spawnChunkLoadingRadius;
    Vec3i_t *pPoints = cmath_algo_expandingCubicShell(SPAWN_ORIGIN, SPAWN_RAD, &size);

    // spawn chunks are permanently chunkloaded
    size_t newChunkCount = size, alreadyLoadedChunkCount = size;
    Vec3i_t *pChunkPosUnloaded = NULL, *pChunkPosLoaded = NULL;
    Chunk_t **ppNewChunks = chunkManager_chunk_createBatch(pState, pPoints, size,
                                                           pChunkPosUnloaded, &newChunkCount,
                                                           pChunkPosLoaded, &alreadyLoadedChunkCount);

    if (ppNewChunks)
    {
        // Permanently load these because this is spawn
        chunkManager_chunk_permanentlyLoad(pState, ppNewChunks, newChunkCount);

        for (size_t i = 0; i < newChunkCount; i++)
        {
            Chunk_t **ppNeighbors = chunkManager_getChunkNeighbors(pState, ppNewChunks[i]->chunkPos);
            ChunkRemeshCtx_t *pCtx = remeshContext_create(ppNewChunks[i], ppNeighbors);
            chunkRenderer_enqueueRemesh(pState->pWorldState, pCtx);
            free(ppNeighbors);
        }
    }
}

static void world_chunks_init(State_t *pState)
{
    pState->pWorldState->pChunksLL = calloc(1, sizeof(LinkedList_t));
    if (!pState->pWorldState->pChunksLL)
        return;

    Entity_t *pChunkLoadingEntity = em_entityCreateHeap();

    pState->pWorldState->pChunkLoadingEntity = pChunkLoadingEntity;

    spawn_generate(pState);
}
#pragma endregion
#pragma region Loop
void world_loop(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    chunkRenderer_remeshChunks(pState);
}
#pragma endregion
#pragma region Load
void world_chunks_load(State_t *pState, Entity_t *pLoadingEntity, const Vec3i_t CHUNK_POS, const uint32_t RADIUS)
{
    pState;
    pLoadingEntity;
    CHUNK_POS;
    RADIUS;

    // size_t size;
    // Vec3i_t *pPoints = cmath_algo_expandingCubicShell(CHUNK_POS, RADIUS, &size);

    // size_t newChunkCount = size, alreadyLoadedChunkCount = size;
    // Vec3i_t *pChunkPosUnloaded = NULL, *pChunkPosLoaded = NULL;
    // Chunk_t **ppNewChunks = chunkManager_chunk_createBatch(pState, pPoints, size,
    //                                                        pChunkPosUnloaded, &newChunkCount,
    //                                                        pChunkPosLoaded, &alreadyLoadedChunkCount);

    // if (ppNewChunks)
    // {
    //     // Permanently load these because this is spawn
    //     chunkManager_chunk_addLoadingEntity(ppNewChunks, newChunkCount, pLoadingEntity);

    //     for (size_t i = 0; i < newChunkCount; i++)
    //     {
    //         Chunk_t **ppNeighbors = chunkManager_getChunkNeighbors(pState, ppNewChunks[i]->chunkPos);
    //         ChunkRemeshCtx_t *pCtx = remeshContext_create(ppNewChunks[i], ppNeighbors);
    //         chunkRenderer_enqueueRemesh(pState->pWorldState, pCtx);
    //         free(ppNeighbors);
    //     }
    // }
}
#pragma endregion
#pragma region Create
static void init(State_t *pState)
{
    pState->pWorldState = calloc(1, sizeof(WorldState_t));

    chunkRenderer_create(pState->pWorldState);

    pState->pWorldState->world.pPlayer = character_create(pState, CHARACTER_TYPE_PLAYER);
    world_chunks_init(pState);

    // The world position is set when the character is created and it should be announced here on world join
    entity_player_chunkPos_update_publish(pState, pState->pWorldState->pPlayerEntity, NULL);
}

void world_load(State_t *pState)
{
    chunkManager_create(pState);
    init(pState);
    pState->pWorldState->isLoaded = true;
}
#pragma endregion
#pragma region Destroy
void world_destroy(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    logs_log(LOG_DEBUG, "Chunks added: %d", addedChunks);

    pState->pWorldState->isLoaded = false;

    // Ensure nothing is in-flight that still uses these buffers
    vkDeviceWaitIdle(pState->context.device);

    chunkManager_destroy(pState);
    chunkRendering_debug();

    pState->pWorldState = NULL;
}
#pragma endregion