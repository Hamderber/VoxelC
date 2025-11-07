#pragma region Includes
#include <string.h>
#include <stdlib.h>
#include "core/logs.h"
#include "character/character.h"
#include "character/characterType_t.h"
#include "world/worldState_t.h"
#include "character/characterController.h"
#include "events/eventBus.h"
#include "world/chunk.h"
#include "world/chunkManager.h"
#pragma endregion
#pragma region Player
Vec3f_t character_player_positionLerped_get(const State_t *pSTATE)
{
    Vec3f_t playerPosition = VEC3_ZERO;

    EntityComponentData_t *playerPhysicsData;
    if (em_entityDataGet(pSTATE->pWorldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &playerPhysicsData))
    {
        // Blend position for camera because its updated in physics but not required for rotation at this time
        // because rotation is per frame (mouse control)
        float alpha = (float)(pSTATE->time.fixedTimeAccumulated / pSTATE->config.fixedTimeStep);
        alpha = cmath_clampF(alpha, 0.0F, 1.0F);

        Vec3f_t posPrev = playerPhysicsData->pPhysicsData->worldPosOld;
        Vec3f_t posCurr = playerPhysicsData->pPhysicsData->worldPos;
        playerPosition = cmath_vec3f_lerpF(posPrev, posCurr, alpha);
    }

    return playerPosition;
}

void character_chunkPos_update_publish(State_t *pState, Character_t *pCharacter)
{
    if (!pCharacter || !pCharacter->pEntity)
        return;

    EntityComponentData_t *pComponentData;
    if (!em_entityDataGet(pCharacter->pEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &pComponentData))
        return;

    ChunkPos_t chunkPos = pComponentData->pPhysicsData->chunkPos;
    logs_log(LOG_DEBUG, "Announcing that Entity %p is now in chunk (%d, %d, %d).",
             pCharacter->pEntity, chunkPos.x, chunkPos.y, chunkPos.z);

    Chunk_t *pChunk = chunkManager_getChunk(pState, chunkPos);
    if (!pChunk)
    {
        logs_log(LOG_ERROR, "Chunk not found");
        return;
    }

    CtxChunk_t ctx = {
        .pCharacterEventSource = pCharacter,
        .pChunk = pChunk,
    };

    Event_t event = {
        .type = EVENT_TYPE_CHUNK_PLAYER_CHUNKPOS_CHANGE,
        .data.pChunkEvntData = &ctx,
    };

    events_publish(pState, &pState->eventBus, EVENT_CHANNEL_CHUNK, event);
}
#pragma endregion
#pragma region Create
Character_t *character_create(State_t *pState, CharacterType_t characterType)
{
    Character_t *pCharacter = calloc(1, sizeof(Character_t));
    pCharacter->type = characterType;

    switch (characterType)
    {
    case CHARACTER_TYPE_PLAYER:
        pCharacter->pName = "Player";
        pCharacter->nameLength = strlen(pCharacter->pName);
        logs_log(LOG_DEBUG, "Created player character '%s'", pCharacter->pName);
        pCharacter->pEntity = character_init(pState, pCharacter);
        break;
    case CHARACTER_TYPE_MOB:
    default:
        logs_log(LOG_WARN, "Character type not yet implemented!");
        break;
    }

    return pCharacter;
}
#pragma endregion