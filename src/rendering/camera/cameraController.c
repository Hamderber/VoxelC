#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/random.h"
#include "events/eventTypes.h"
#include "events/eventBus.h"
#include "events/context/CtxInputMapped_t.h"
#include "entity/entityManager.h"
#include <stdlib.h>
#include <string.h>

EventResult_t camera_onInput(struct State_t *state, Event_t *event, void *ctx)
{
    if (event == NULL)
    {
        // Null event? This should never happen
        return EVENT_RESULT_ERROR;
    }

    // Consider accumulating motion into a normalized 3d vector at some point

    if (event->type == EVENT_TYPE_INPUT_MAPPED && event->data.inputMapped != NULL)
    {
        for (size_t i = 0; i < event->data.inputMapped->actionCount; i++)
        {
            switch (event->data.inputMapped->inputActions[i].action)
            {
            case INPUT_ACTION_JUMP:
                logs_log(LOG_DEBUG, "Camera moving up");
                break;
            case INPUT_ACTION_CROUCH:
                logs_log(LOG_DEBUG, "Camera moving down");
                break;
            case INPUT_ACTION_MOVE_FORWARD:
                logs_log(LOG_DEBUG, "Camera moving forward");
                break;
            case INPUT_ACTION_MOVE_BACKWARD:
                logs_log(LOG_DEBUG, "Camera moving backward");
                break;
            case INPUT_ACTION_MOVE_LEFT:
                logs_log(LOG_DEBUG, "Camera moving left");
                break;
            case INPUT_ACTION_MOVE_RIGHT:
                logs_log(LOG_DEBUG, "Camera moving right");
                break;
            default:
                // No-op on actions that aren't applicable. When there is any input action, they will all be iterated here
                break;
            }
        }
    }

    return EVENT_RESULT_PASS;
}

void camera_eventsSubscribe(State_t *state)
{
    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT, camera_onInput, false, false, NULL);
}

Camera_t *camera_dataCreate(Entity_t *cameraEntity)
{
    cameraEntity->data.cameraData = calloc(1, sizeof(Camera_t));

    float randRange = 10.0F;
    Vec3f_t pos = rand_vec3f(-randRange, randRange, -randRange, randRange, -randRange, randRange);
    cameraEntity->data.cameraData->pos = pos;
    logs_log(LOG_DEBUG, "Camera is at random position (%lf, %lf, %lf)", pos.x, pos.y, pos.z);

    return cameraEntity->data.cameraData;
}

void camera_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing camera...");

    Entity_t *cameraEntity = em_entityCreateHeap();
    cameraEntity->type = ENTITY_TYPE_CAMERA;

    em_entityAddToCollection(&state->entityManager.entityCollections[ENTITY_COLLECTION_MOVABLE], cameraEntity);

    state->context.pCamera = camera_dataCreate(cameraEntity);

    camera_eventsSubscribe(state);
}