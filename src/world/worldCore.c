#pragma region Includes
#include "api/chunk/chunkAPI.h"
#include "core/types/state_t.h"
#include "core/app_time.h"
#include "world/worldType_e.h"
#include "chunk/chunkManagerNew.h"
#include "chunk/chunkSource_local.h"
#include "chunk/chunkSource_network.h"
#pragma endregion
#pragma region Defines
#if defined(DEBUG)
#define DEBUG_WORLDCORE
#endif
#pragma endregion
#pragma region Operations
void worldCore_tick(State_t *pState, double deltaTime)
{
    deltaTime;
    if (!pState || !pState->pWorldState || !pState->pWorldState->isLoaded)
        return;

    // Per cpu frame

    // Fixed time ticks (same as physics ticks. Combine the tracking?)
    // See TODO in WorldState_t
}
#pragma endregion
#pragma region Create / Destroy
WorldState_t *worldCore_create(State_t *pState, const WorldType_e TYPE)
{
    if (!pState)
        return NULL;

#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "Initializing world state of type %s...", pWORLD_TYPE_NAMES[TYPE]);
#endif

    WorldState_t *pWorldState = NULL;
    ChunkManager_t *pChunkManager = NULL;
    ChunkSource_t *pChunkSource = NULL;
    Entity_t *pChunkLoadingEntity = NULL;
    do
    {
        pWorldState = calloc(1, sizeof(WorldState_t));
        if (!pWorldState)
            break;
#if defined(DEBUG_WORLDCORE)
        logs_log(LOG_DEBUG, "Allocated world state %p.", pWorldState);
#endif
        pWorldState->worldType = TYPE;

        pChunkManager = chunkManager_createNew(pState);
        if (!pChunkManager)
            break;
#if defined(DEBUG_WORLDCORE)
        logs_log(LOG_DEBUG, "Created chunk manager %p.", pChunkManager);
#endif

        switch (TYPE)
        {
        case WORLD_TYPE_LOCAL:
            pChunkSource = chunkSource_createLocal(pChunkManager, pState->pWorldConfig, NULL);
            break;
        case WORLD_TYPE_SERVER:
            logs_log(LOG_ERROR, "Server world type not yet implemented!");
            pChunkSource = NULL;
            break;
        }

        if (!pChunkSource)
            break;
#if defined(DEBUG_WORLDCORE)
        logs_log(LOG_DEBUG, "Created chunk source %p for world type %s.", pChunkSource, pWORLD_TYPE_NAMES[TYPE]);
#endif

        pChunkLoadingEntity = em_entityCreateHeap();
        if (!pChunkLoadingEntity)
            break;
#if defined(DEBUG_WORLDCORE)
        logs_log(LOG_DEBUG, "Created permanent chunk loading entity %p.", pChunkLoadingEntity);
#endif

        pWorldState->pChunkManager = pChunkManager;
        pWorldState->pChunkSource = pChunkSource;
        pWorldState->pChunkLoadingEntity = pChunkLoadingEntity;
        pWorldState->isLoaded = true;

#if defined(DEBUG_WORLDCORE)
        logs_log(LOG_DEBUG, "World %p initialized.", pWorldState);
#endif

        return pWorldState;
    } while (false);

    free(pWorldState);
    chunkManager_destroyNew(pState, pChunkManager);
    chunkSource_destroy(pChunkSource);
    em_entityDestroy(pState, &pChunkLoadingEntity);
    logs_log(LOG_ERROR, "Failed to initialize world of type %s!", pWORLD_TYPE_NAMES[TYPE]);
    return NULL;
}

void worldCore_destroy(State_t *restrict pState, WorldState_t *restrict pWorldState)
{
#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "Destroying world %p...", pWorldState);
#endif
    if (!pWorldState)
        return;
#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "Destroying chunk manager %p.", pWorldState->pChunkManager);
#endif
    chunkManager_destroyNew(pState, pWorldState->pChunkManager);
#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "Destroying chunk source %p.", pWorldState->pChunkSource);
#endif
    chunkSource_destroy(pWorldState->pChunkSource);
#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "Destroying permanent chunk loading entity %p.", pWorldState->pChunkLoadingEntity);
#endif
    em_entityDestroy(pState, &pWorldState->pChunkLoadingEntity);
#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "Freeing world state %p.", pWorldState);
#endif
    free(pWorldState);
#if defined(DEBUG_WORLDCORE)
    logs_log(LOG_DEBUG, "World destroyed.");
#endif
}
#pragma endregion