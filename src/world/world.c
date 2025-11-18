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
#include "core/random.h"
#include "world/chunkManager.h"
#include "chunkGenerator.h"
#include "rendering/chunk/chunkRenderer.h"
#include "chunk/chunkManagerNew.h"
#include "chunk/chunkSource_local.h"
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
void world_chunks_load(State_t *restrict pState, Entity_t *restrict pLoadingEntity, const Vec3i_t CHUNK_POS, const uint32_t RADIUS)
{
    pLoadingEntity;

    size_t size;
    Vec3i_t *pPoints = cmath_algo_expandingCubicShell(CHUNK_POS, RADIUS, &size);

    size_t newCount = size;
    size_t existingCount = size;
    Chunk_t **ppNewChunks = NULL;
    Chunk_t **ppExistingChunk = NULL;

    if (!chunkManager_chunks_aquire(pState->pWorldState->pChunkManager, pPoints, size, &ppNewChunks, &newCount,
                                    &ppExistingChunk, &existingCount))
    {
        logs_log(LOG_ERROR, "Failed to aquire %zu chunks!", size);
    }

    if (newCount > 0)
    {
        if (!chunkManager_chunks_populateNew(pState, pState->pWorldState->pChunkManager, pState->pWorldState->pChunkSource,
                                             ppNewChunks, newCount))
        {
            logs_log(LOG_ERROR, "Failed to populate %zu chunks!", size);
        }
    }

    // TODO: Re-implement rendering

    free(ppNewChunks);
    free(ppExistingChunk);
}
#pragma endregion
#pragma region Create
static void spawn_generate(State_t *pState)
{
    const Vec3i_t SPAWN_ORIGIN = VEC3I_ZERO;
    const int SPAWN_RAD = pState->pWorldConfig->spawnChunkLoadingRadius;
    world_chunks_load(pState, pState->pWorldState->pChunkLoadingEntity, SPAWN_ORIGIN, SPAWN_RAD);
    logs_log(LOG_DEBUG, "Spawn created for world %p.", pState->pWorldState);
}

static void world_chunks_init(State_t *pState)
{
    if (!pState->pWorldState->pChunkManager->pChunksLL)
        return;

    Entity_t *pChunkLoadingEntity = em_entityCreateHeap();

    pState->pWorldState->pChunkLoadingEntity = pChunkLoadingEntity;

    spawn_generate(pState);
}

static void init(State_t *pState)
{
    pState->pWorldState = calloc(1, sizeof(WorldState_t));
    pState->pWorldState->pChunkManager = chunkManager_createNew(pState);
    pState->pWorldState->pChunkSource = chunkSource_createLocal(pState->pWorldState->pChunkManager, pState->pWorldConfig, NULL);

    chunkRenderer_create(pState->pWorldState);

    pState->pWorldState->world.pPlayer = character_create(pState, CHARACTER_TYPE_PLAYER);
    world_chunks_init(pState);

    // The world position is set when the character is created and it should be announced here on world join
    entity_player_chunkPos_update_publish(pState, pState->pWorldState->pPlayerEntity, NULL);
}

void world_load(State_t *pState)
{
    init(pState);
    pState->pWorldState->isLoaded = true;
}
#pragma endregion
#pragma region Destroy
void world_destroy(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    pState->pWorldState->isLoaded = false;

    // Ensure nothing is in-flight that still uses these buffers
    vkDeviceWaitIdle(pState->context.device);

    chunkManager_destroyNew(pState, pState->pWorldState->pChunkManager);

    pState->pWorldState = NULL;
}
#pragma endregion