#include <stdbool.h>
#include <stdlib.h>
#include "core/types/state_t.h"
#include "core/logs.h"
#include "events/eventTypes.h"
#include "events/eventBus.h"
#include "character/characterType_t.h"
#include "character/character.h"
#include "rendering/camera/cameraController.h"
#include "entity/entityManager.h"
#include "characterController.h"
#include "entity/entity_t.h"

EventResult_t character_onAxialInput(struct State_t *state, Event_t *event, void *ctx)
{
    ctx = NULL;
    if (event == NULL)
    {
        // Null event? This should never happen
        return EVENT_RESULT_ERROR;
    }

    if (event->type == EVENT_TYPE_INPUT_MAPPED && event->data.inputMapped != NULL)
    {
        for (size_t i = 0; i < event->data.inputMapped->actionCount; i++)
        {
            if (event->data.inputMapped->inputActions[i].actionState == CTX_INPUT_ACTION_START)
            {
                switch (event->data.inputMapped->inputActions[i].action)
                {
                case INPUT_ACTION_JUMP:
                    logs_log(LOG_DEBUG, "Character moving up (pressed)");
                    state->input.axialInput.y += 1.0F;
                    break;
                case INPUT_ACTION_CROUCH:
                    logs_log(LOG_DEBUG, "Character moving down (pressed)");
                    state->input.axialInput.y -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_FORWARD:
                    logs_log(LOG_DEBUG, "Character moving forward (pressed)");
                    state->input.axialInput.z -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_BACKWARD:
                    logs_log(LOG_DEBUG, "Character moving backward (pressed)");
                    state->input.axialInput.z += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_LEFT:
                    logs_log(LOG_DEBUG, "Character moving left (pressed)");
                    state->input.axialInput.x -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_RIGHT:
                    logs_log(LOG_DEBUG, "Character moving right (pressed)");
                    state->input.axialInput.x += 1.0F;
                    break;
                default:
                    // No-op on actions that aren't applicable. When there is any input action, they will all be iterated here
                    break;
                }
            }
            else if (event->data.inputMapped->inputActions[i].actionState == CTX_INPUT_ACTION_END)
            {
                switch (event->data.inputMapped->inputActions[i].action)
                {
                case INPUT_ACTION_JUMP:
                    logs_log(LOG_DEBUG, "Character moving up (released)");
                    state->input.axialInput.y -= 1.0F;
                    break;
                case INPUT_ACTION_CROUCH:
                    logs_log(LOG_DEBUG, "Character moving down (released)");
                    state->input.axialInput.y += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_FORWARD:
                    logs_log(LOG_DEBUG, "Character moving forward (released)");
                    state->input.axialInput.z += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_BACKWARD:
                    logs_log(LOG_DEBUG, "Character moving backward (released)");
                    state->input.axialInput.z -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_LEFT:
                    logs_log(LOG_DEBUG, "Character moving left (released)");
                    state->input.axialInput.x += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_RIGHT:
                    logs_log(LOG_DEBUG, "Character moving right (released)");
                    state->input.axialInput.x -= 1.0F;
                    break;
                default:
                    break;
                }
            }
        }

        // Protect against faulty input events
        state->input.axialInput.x = cmath_clampF(state->input.axialInput.x, -1.0F, 1.0F);
        state->input.axialInput.y = cmath_clampF(state->input.axialInput.y, -1.0F, 1.0F);
        state->input.axialInput.z = cmath_clampF(state->input.axialInput.z, -1.0F, 1.0F);
    }

    return EVENT_RESULT_PASS;
}

void character_sprintToggle(State_t *state, Character_t *character)
{
    EntityComponentData_t *componentData;
    if (!em_entityDataGet(state->worldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &componentData))
        return;

    character->isSprinting = !character->isSprinting;

    float baseSpeed = componentData->physicsData->uniformSpeedBase;
    float speed = componentData->physicsData->uniformSpeed;
    componentData->physicsData->uniformSpeed = character->isSprinting ? speed * SPRINT_SPEED_MULTIPLIER : baseSpeed;

    logs_log(LOG_DEBUG, "Sprint toggled. Character %s speed is %lfm/s", character->name, componentData->physicsData->uniformSpeed);
}

EventResult_t character_onSprintTogglePress(struct State_t *state, Event_t *event, void *ctx)
{
    if (event == NULL)
    {
        return EVENT_RESULT_ERROR;
    }

    if (event->type == EVENT_TYPE_INPUT_MAPPED && event->data.inputMapped != NULL)
    {
        for (size_t i = 0; i < event->data.inputMapped->actionCount; i++)
        {
            if (event->data.inputMapped->inputActions[i].actionState == CTX_INPUT_ACTION_START)
            {
                switch (event->data.inputMapped->inputActions[i].action)
                {
                case INPUT_ACTION_SPRINT_TOGGLE:
                    Character_t *character = (Character_t *)ctx;
                    logs_log(LOG_DEBUG, "Sprint toggle (pressed)");
                    character_sprintToggle(state, character);
                    return EVENT_RESULT_PASS;
                    break;
                }
            }
        }
    }

    return EVENT_RESULT_PASS;
}

void player_physicsIntentUpdate(State_t *state)
{
    EntityComponentData_t *componentData;
    if (!em_entityDataGet(state->worldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &componentData))
        return;

    EntityDataPhysics_t *phys = componentData->physicsData;
    phys->moveIntention = state->input.axialInput;
    // For now, just directly rotate the player by the camera's rotation. Later will probably have to just do pitch
    phys->rotation = state->context.camera.rotation;

    // Stop sprinting if no longer moving forward
    if (state->worldState->world.pPlayer->isSprinting && fabsf(phys->moveIntention.z) < CMATH_EPSILON_F)
    {
        logs_log(LOG_DEBUG, "Player stopped moving trying to move forward, automatically toggling sprinting.");
        character_sprintToggle(state, state->worldState->world.pPlayer);
    }
}

void character_entityCreate(State_t *state, Character_t *character)
{
    Entity_t *characterEntity = em_entityCreateHeap();
    characterEntity->type = ENTITY_TYPE_CREATURE;

    em_entityAddToCollection(&state->entityManager.entityCollections[ENTITY_COLLECTION_PHYSICS], characterEntity);

    characterEntity->componentCount = 1;
    characterEntity->components = calloc(characterEntity->componentCount, sizeof(EntityComponent_t));
    characterEntity->components[0] = (EntityComponent_t){
        .data = calloc(1, sizeof(EntityComponentData_t)),
        .type = ENTITY_COMPONENT_TYPE_PHYSICS,
    };
    characterEntity->components[0].data->physicsData = calloc(1, sizeof(EntityDataPhysics_t));
    characterEntity->components[0].data->physicsData->uniformSpeed = DEFAULT_UNIFORM_SPEED;
    characterEntity->components[0].data->physicsData->uniformSpeedBase = DEFAULT_UNIFORM_SPEED;
    characterEntity->components[0].data->physicsData->useLocalAxes = true;

    switch (character->type)
    {
    case CHARACTER_TYPE_PLAYER:

        Vec3f_t pos = (Vec3f_t){-3.0F, -3.0F, 15.0F};
        characterEntity->components[0].data->physicsData->pos = pos;

        // Adjust the drag to get the right "floatiness" while in flying
        characterEntity->components[0].data->physicsData->drag = 2.0F;

        state->worldState->pPlayerEntity = characterEntity;
        break;
    default:
        characterEntity->components[0].data->physicsData->drag = 3.0F;
        break;
    }
}

void character_eventsSubscribe(State_t *state, Character_t *character)
{
    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT, character_onAxialInput, false, false, NULL);
    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT, character_onSprintTogglePress, false, false, (void *)character);
}

void character_init(State_t *state, Character_t *character)
{
    switch (character->type)
    {
    case CHARACTER_TYPE_PLAYER:
        camera_init(state);
        character_entityCreate(state, character);
        character_eventsSubscribe(state, character);
        break;
    case CHARACTER_TYPE_MOB:
    default:
        logs_log(LOG_WARN, "Character type not yet implemented!");
        break;
    }
}