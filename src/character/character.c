#include <string.h>
#include <stdlib.h>
#include "core/logs.h"
#include "character/character.h"
#include "character/characterType_t.h"
#include "world/worldState_t.h"
#include "character/characterController.h"

Vec3f_t character_player_positionLerped_get(const State_t *pSTATE)
{
    Vec3f_t playerPosition = VEC3_ZERO;

    EntityComponentData_t *playerPhysicsData;
    if (em_entityDataGet(pSTATE->worldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &playerPhysicsData))
    {
        // Blend position for camera because its updated in physics but not required for rotation at this time
        // because rotation is per frame (mouse control)
        float alpha = (float)(pSTATE->time.fixedTimeAccumulated / pSTATE->config.fixedTimeStep);
        alpha = cmath_clampF(alpha, 0.0F, 1.0F);

        Vec3f_t posPrev = playerPhysicsData->physicsData->posOld;
        Vec3f_t posCurr = playerPhysicsData->physicsData->pos;
        playerPosition = cmath_vec3f_lerpF(posPrev, posCurr, alpha);
    }

    return playerPosition;
}

Character_t *character_create(State_t *state, CharacterType_t characterType)
{
    Character_t *character = calloc(1, sizeof(Character_t));
    character->type = characterType;

    switch (characterType)
    {
    case CHARACTER_TYPE_PLAYER:
        character->name = "Player"; // string literal (read-only)
        character->nameLength = strlen(character->name);
        logs_log(LOG_DEBUG, "Created player character '%s'", character->name);
        character_init(state, character);
        break;
    case CHARACTER_TYPE_MOB:
    default:
        logs_log(LOG_WARN, "Character type not yet implemented!");
        break;
    }

    return character;
}