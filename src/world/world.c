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
#pragma endregion
#pragma region Add Chunk to Col.
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

    // Chunk_t *pData = (Chunk_t *)(pAdd->pData);
    // logs_log(LOG_DEBUG, "Adding chunk %p at (%d, %d, %d) to chunk linked list.",
    //          pData, pData->chunkPos.x, pData->chunkPos.y, pData->chunkPos.z);
}
#pragma endregion
#pragma region Chunks Init
static void world_chunks_init(State_t *pState)
{
    pState->pWorldState->pChunksLL = calloc(1, sizeof(LinkedList_t));
    if (!pState->pWorldState->pChunksLL)
        return;

    size_t size;
    // Spawn
    const Vec3i_t SPAWN = {0, 0, 0};
    const int SPAWN_RAD = pState->worldConfig.spawnChunkLoadingRadius;
    Vec3i_t *pPoints = cmath_algo_expandingCubicShell(SPAWN, SPAWN_RAD, &size);

    // spawn chunks are permanently chunkloaded
    chunkManager_chunk_createBatch(pState, pPoints, size, NULL);
    free(pPoints);
}
#pragma endregion
#pragma region Load
// world_chunks_update
#pragma endregion
#pragma region Create
static void init(State_t *pState)
{
    pState->pWorldState = calloc(1, sizeof(WorldState_t));

    pState->pWorldState->world.pPlayer = character_create(pState, CHARACTER_TYPE_PLAYER);
    world_chunks_init(pState);

    // The world position is set when the character is created and it should be announced here on world join
    entity_player_chunkPos_update_publish(pState, pState->pWorldState->pPlayerEntity, NULL);
}

void world_load(State_t *pState)
{
    chunkManager_create(pState);
    init(pState);
}
#pragma endregion
#pragma region Destroy
void world_destroy(State_t *pState)
{
    if (!pState || !pState->pWorldState)
        return;

    // Ensure nothing is in-flight that still uses these buffers
    vkDeviceWaitIdle(pState->context.device);

    chunkManager_linkedList_destroy(pState, &pState->pWorldState->pChunksLL);

    pState->pWorldState = NULL;

    chunkManager_destroy(pState);
}
#pragma endregion