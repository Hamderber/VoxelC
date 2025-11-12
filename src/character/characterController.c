#pragma region Includes
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
#include "input/types/inputActionQuery_t.h"
#include "input/input.h"
#include "collection/flags64_t.h"
#pragma endregion
#pragma region Axial Input
EventResult_t character_onAxialInput(State_t *pState, Event_t *pEvent, void *pCtx)
{
    pCtx;
    if (!pState || !pEvent)
        return EVENT_RESULT_ERROR;

    // Don't even bother with axial input if a menu of any type is open
    if (pState->gui.menuDepth != GUI_ID_NONE)
        return EVENT_RESULT_PASS;

    const InputActionQuery_t pQUERY[] = {
        {.mapping = INPUT_ACTION_JUMP,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_JUMP,
         .actionCtx = CTX_INPUT_ACTION_END},
        {.mapping = INPUT_ACTION_CROUCH,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_CROUCH,
         .actionCtx = CTX_INPUT_ACTION_END},
        {.mapping = INPUT_ACTION_MOVE_FORWARD,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_MOVE_FORWARD,
         .actionCtx = CTX_INPUT_ACTION_END},
        {.mapping = INPUT_ACTION_MOVE_BACKWARD,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_MOVE_BACKWARD,
         .actionCtx = CTX_INPUT_ACTION_END},
        {.mapping = INPUT_ACTION_MOVE_LEFT,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_MOVE_LEFT,
         .actionCtx = CTX_INPUT_ACTION_END},
        {.mapping = INPUT_ACTION_MOVE_RIGHT,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_MOVE_RIGHT,
         .actionCtx = CTX_INPUT_ACTION_END}};

    InputAction_t pQueryResult[sizeof pQUERY / sizeof pQUERY[0]];
    const size_t SIZE = input_inputAction_matchQuery(pEvent, pQUERY, sizeof pQUERY / sizeof pQUERY[0], pQueryResult);

    if (SIZE > 0)
        for (size_t i = 0; i < SIZE; i++)
        {
            const InputAction_t ACTION = pQueryResult[i];

            switch (ACTION.action)
            {
            case INPUT_ACTION_JUMP:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Character jump (pressed)");
                    // flight mode
                    pState->input.axialInput.y += 1.0F;
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Character jump (released)");
                    // flight mode
                    pState->input.axialInput.y -= 1.0F;
                }
                break;
            case INPUT_ACTION_CROUCH:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Character crouch (pressed)");
                    // flight mode
                    pState->input.axialInput.y -= 1.0F;
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Character crouch (released)");
                    // flight mode
                    pState->input.axialInput.y += 1.0F;
                }
                break;
            case INPUT_ACTION_MOVE_FORWARD:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Character moving forward (pressed)");
                    pState->input.axialInput.z -= 1.0F;
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Character moving forward (released)");
                    pState->input.axialInput.z += 1.0F;
                }
                break;
            case INPUT_ACTION_MOVE_BACKWARD:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Character moving backward (pressed)");
                    pState->input.axialInput.z += 1.0F;
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Character moving backward (released)");
                    pState->input.axialInput.z -= 1.0F;
                }
                break;
            case INPUT_ACTION_MOVE_LEFT:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Character moving left (pressed)");
                    pState->input.axialInput.x -= 1.0F;
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Character moving left (released)");
                    pState->input.axialInput.x += 1.0F;
                }
                break;
            case INPUT_ACTION_MOVE_RIGHT:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Character moving right (pressed)");
                    pState->input.axialInput.x += 1.0F;
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Character moving right (released)");
                    pState->input.axialInput.x -= 1.0F;
                }
                break;
            }
        }

    // Protect against faulty input events
    pState->input.axialInput.x = cmath_clampF(pState->input.axialInput.x, -1.0F, 1.0F);
    pState->input.axialInput.y = cmath_clampF(pState->input.axialInput.y, -1.0F, 1.0F);
    pState->input.axialInput.z = cmath_clampF(pState->input.axialInput.z, -1.0F, 1.0F);

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Sprint
void character_sprintToggle(State_t *pState, Character_t *pCharacter)
{
    EntityComponentData_t *pComponentData;
    if (!em_entityDataGet(pState->pWorldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &pComponentData))
        return;

    pCharacter->isSprinting = !pCharacter->isSprinting;

    float baseSpeed = pComponentData->pPhysicsData->uniformSpeedBase;
    float speed = pComponentData->pPhysicsData->uniformSpeed;
    pComponentData->pPhysicsData->uniformSpeed = pCharacter->isSprinting ? speed * SPRINT_SPEED_MULTIPLIER : baseSpeed;

    camera_sprintFOV_toggle(pState, pCharacter->isSprinting);

    logs_log(LOG_DEBUG, "Sprint toggled. Character %s speed is %lfm/s", pCharacter->pName, pComponentData->pPhysicsData->uniformSpeed);
}

EventResult_t character_onSprintTogglePress(State_t *pState, Event_t *pEvent, void *pCtx)
{
    if (pEvent == NULL)
        return EVENT_RESULT_ERROR;

    const InputActionQuery_t pQUERY[] = {
        {.mapping = INPUT_ACTION_SPRINT_TOGGLE,
         .actionCtx = CTX_INPUT_ACTION_START},
        {.mapping = INPUT_ACTION_SPRINT_TOGGLE,
         .actionCtx = CTX_INPUT_ACTION_END}};

    InputAction_t pQueryResult[sizeof pQUERY / sizeof pQUERY[0]];
    const size_t SIZE = input_inputAction_matchQuery(pEvent, pQUERY, sizeof pQUERY / sizeof pQUERY[0], pQueryResult);

    if (SIZE > 0)
        for (size_t i = 0; i < SIZE; i++)
        {
            const InputAction_t ACTION = pQueryResult[i];

            switch (ACTION.action)
            {
            case INPUT_ACTION_SPRINT_TOGGLE:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    Character_t *pCharacter = (Character_t *)pCtx;
                    logs_log(LOG_DEBUG, "Sprint toggle (pressed)");
                    pCharacter->sprintIntention = true;
                    character_sprintToggle(pState, pCharacter);
                    return EVENT_RESULT_PASS;
                }
                if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    // Only toggle sprint intention, let movement intention hold sprinting active until z motion stops
                    Character_t *pCharacter = (Character_t *)pCtx;
                    pCharacter->sprintIntention = false;
                    return EVENT_RESULT_PASS;
                }
                break;
            }
        }

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Physics
void player_physicsIntentUpdate(State_t *pState)
{
    EntityComponentData_t *componentData;
    if (!em_entityDataGet(pState->pWorldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &componentData))
        return;

    EntityDataPhysics_t *pPhys = componentData->pPhysicsData;
    pPhys->moveIntention = pState->input.axialInput;
    // For now, just directly rotate the player by the camera's rotation. Later will probably have to just do pitch
    pPhys->rotation = pState->context.camera.rotation;

    // Stop sprinting if no longer moving forward
    if (!pState->pWorldState->world.pPlayer->sprintIntention && pState->pWorldState->world.pPlayer->isSprinting &&
        fabsf(pPhys->moveIntention.z) < CMATH_EPSILON_F)
    {
        logs_log(LOG_DEBUG, "Player stopped moving trying to move forward, automatically toggling sprinting.");
        character_sprintToggle(pState, pState->pWorldState->world.pPlayer);
    }
}
#pragma endregion
#pragma region Entity Flags
static void entity_flags_player(Entity_t *pEntity)
{
    flag64_setInline(&pEntity->entityFlags, ENTITY_FLAG_PLAYER, true);
}
#pragma endregion
#pragma region Entity Create
Entity_t *character_entityCreate(State_t *pState, Character_t *pCharacter)
{
    Entity_t *pCharacterEntity = em_entityCreateHeap();
    pCharacterEntity->type = ENTITY_TYPE_CREATURE;

    em_entityAddToCollection(&pState->entityManager.entityCollections[ENTITY_COLLECTION_PHYSICS], pCharacterEntity);

    pCharacterEntity->componentCount = 1;
    pCharacterEntity->pComponents = calloc(pCharacterEntity->componentCount, sizeof(EntityComponent_t));

    pCharacterEntity->pComponents[0] = (EntityComponent_t){
        .pComponentData = calloc(1, sizeof(EntityComponentData_t)),
        .type = ENTITY_COMPONENT_TYPE_PHYSICS};
    pCharacterEntity->pComponents[0].pComponentData->pPhysicsData = calloc(1, sizeof(EntityDataPhysics_t));
    pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->uniformSpeed = DEFAULT_UNIFORM_SPEED;
    pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->uniformSpeedBase = DEFAULT_UNIFORM_SPEED;
    pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->useLocalAxes = true;

    Vec3f_t worldPos = VEC3F_ZERO;
    switch (pCharacter->type)
    {
    case CHARACTER_TYPE_PLAYER:
        pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->worldPos = worldPos;

        // Adjust the drag to get the right "floatiness" while in flying
        pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->drag = 2.0F;

        pState->pWorldState->pPlayerEntity = pCharacterEntity;

        entity_flags_player(pCharacterEntity);
        break;
    default:
        pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->drag = 3.0F;
        break;
    }

    // Store initial chunkPos directly
    pCharacterEntity->pComponents[0].pComponentData->pPhysicsData->chunkPos = worldPosf_to_chunkPos(worldPos);

    return pCharacterEntity;
}
#pragma endregion
#pragma region Init
static void character_eventsSubscribe(State_t *pState, Character_t *pCharacter)
{
    const bool CONSUME_LISTENER = false;
    const bool CONSUME_EVENT = false;
    const void *pSubCtx = NULL;

    events_subscribe(&pState->eventBus, EVENT_CHANNEL_INPUT_ACTIONS, character_onAxialInput,
                     CONSUME_LISTENER, CONSUME_EVENT, &pSubCtx);

    events_subscribe(&pState->eventBus, EVENT_CHANNEL_INPUT_ACTIONS, character_onSprintTogglePress,
                     CONSUME_LISTENER, CONSUME_EVENT, (void *)pCharacter);
}

Entity_t *character_init(State_t *pState, Character_t *pCharacter)
{
    Entity_t *pEntity = NULL;
    switch (pCharacter->type)
    {
    case CHARACTER_TYPE_PLAYER:
        camera_init(pState);
        pEntity = character_entityCreate(pState, pCharacter);
        character_eventsSubscribe(pState, pCharacter);
        break;
    case CHARACTER_TYPE_MOB:
    default:
        logs_log(LOG_WARN, "Character type not yet implemented!");
        break;
    }

    return pEntity;
}
#pragma endregion