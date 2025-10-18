#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/random.h"
#include "events/eventTypes.h"
#include "events/eventBus.h"
#include "events/context/CtxInputMapped_t.h"
#include "entity/entityManager.h"
#include "c_math/c_math.h"
#include <stdlib.h>
#include <string.h>

EventResult_t camera_onInput(struct State_t *state, Event_t *event, void *ctx)
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
                    logs_log(LOG_DEBUG, "Camera moving up (pressed)");
                    state->input.axialInput.y += 1.0F;
                    break;
                case INPUT_ACTION_CROUCH:
                    logs_log(LOG_DEBUG, "Camera moving down (pressed)");
                    state->input.axialInput.y -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_FORWARD:
                    logs_log(LOG_DEBUG, "Camera moving forward (pressed)");
                    state->input.axialInput.z -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_BACKWARD:
                    logs_log(LOG_DEBUG, "Camera moving backward (pressed)");
                    state->input.axialInput.z += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_LEFT:
                    logs_log(LOG_DEBUG, "Camera moving left (pressed)");
                    state->input.axialInput.x -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_RIGHT:
                    logs_log(LOG_DEBUG, "Camera moving right (pressed)");
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
                    logs_log(LOG_DEBUG, "Camera moving up (released)");
                    state->input.axialInput.y -= 1.0F;
                    break;
                case INPUT_ACTION_CROUCH:
                    logs_log(LOG_DEBUG, "Camera moving down (released)");
                    state->input.axialInput.y += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_FORWARD:
                    logs_log(LOG_DEBUG, "Camera moving forward (released)");
                    state->input.axialInput.z += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_BACKWARD:
                    logs_log(LOG_DEBUG, "Camera moving backward (released)");
                    state->input.axialInput.z -= 1.0F;
                    break;
                case INPUT_ACTION_MOVE_LEFT:
                    logs_log(LOG_DEBUG, "Camera moving left (released)");
                    state->input.axialInput.x += 1.0F;
                    break;
                case INPUT_ACTION_MOVE_RIGHT:
                    logs_log(LOG_DEBUG, "Camera moving right (released)");
                    state->input.axialInput.x -= 1.0F;
                    break;
                default:
                    break;
                }
            }

            // logs_log(LOG_DEBUG, "Accumulated axial input: %lf, %lf, %lf",
            //          state->input.axialInput.x, state->input.axialInput.y, state->input.axialInput.z);
        }

        // Protect against faulty input events
        state->input.axialInput.x = cm_clampf(state->input.axialInput.x, -1.0F, 1.0F);
        state->input.axialInput.y = cm_clampf(state->input.axialInput.y, -1.0F, 1.0F);
        state->input.axialInput.z = cm_clampf(state->input.axialInput.z, -1.0F, 1.0F);
    }

    return EVENT_RESULT_PASS;
}

void camera_eventsSubscribe(State_t *state)
{
    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT, camera_onInput, false, false, NULL);
}

void camera_dataCreate(AppConfig_t *cfg, Entity_t *cameraEntity)
{
    cameraEntity->componentCount = 2;
    cameraEntity->components = calloc(cameraEntity->componentCount, sizeof(EntityComponent_t));

    cameraEntity->components[0] = (EntityComponent_t){
        .data = calloc(1, sizeof(EntityComponentData_t)),
        .type = ENTITY_COMPONENT_TYPE_CAMERA,
    };
    cameraEntity->components[0].data->cameraData = calloc(1, sizeof(EntityDataCamera_t));

    // float randRange = 10.0F;
    // Vec3f_t pos = rand_vec3f(-randRange, randRange, -randRange, randRange, -randRange, randRange);
    Vec3f_t pos = (Vec3f_t){-3.0F, -3.0F, 15.0F};
    cameraEntity->components[1] = (EntityComponent_t){
        .data = calloc(1, sizeof(EntityComponentData_t)),
        .type = ENTITY_COMPONENT_TYPE_PHYSICS,
    };
    cameraEntity->components[1].data->physicsData = calloc(1, sizeof(EntityDataPhysics_t));

    float fov = cfg->cameraFOV;
    cameraEntity->components[0].data->cameraData->fov = fov;

    float uniformSpeed = 8.0F;
    cameraEntity->components[1].data->physicsData->uniformSpeed = uniformSpeed;
    cameraEntity->components[1].data->physicsData->pos = pos;
    // Adjust the drag to get the right "floatiness" while in flying
    cameraEntity->components[1].data->physicsData->drag = 2.0F;
    cameraEntity->components[1].data->physicsData->useLocalAxes = true;
    // Verbose here to make sure the values are actually in the entity data
    logs_log(LOG_DEBUG, "Camera is at random position (%lf, %lf, %lf) with FOV %lf and uniformSpeed %lfm/s",
             cameraEntity->components[1].data->physicsData->pos.x,
             cameraEntity->components[1].data->physicsData->pos.y,
             cameraEntity->components[1].data->physicsData->pos.z,
             fov, cameraEntity->components[1].data->physicsData->uniformSpeed);
}

// Update the camera's physics entity component with the move intention accumulated by input monitoring
void camera_physicsIntentUpdate(State_t *state)
{
    EntityComponentData_t *componentData;
    if (!em_entityDataGet(state->context.pCameraEntity,
                          ENTITY_COMPONENT_TYPE_PHYSICS,
                          &componentData))
        return;

    EntityDataPhysics_t *phys = componentData->physicsData;
    phys->moveIntention = state->input.axialInput;

    // const float radPerPixel = 0.02f * (float)state->config.mouseSensitivity;
    // const float dx = -(float)state->gui.mouse.dx * radPerPixel; // yaw
    // const float dy = (float)state->gui.mouse.dy * radPerPixel;  // pitch

    // // TODO implement rotation of camera

    // state->gui.mouse.dx = 0.0;
    // state->gui.mouse.dy = 0.0;

    logs_log(LOG_DEBUG,
             "Camera pos: %.6f, %.6f, %.6f Quaternion: (%.5f, %.5f, %.5f, %.5f)",
             phys->pos.x, phys->pos.y, phys->pos.z,
             phys->rotation.qx, phys->rotation.qy,
             phys->rotation.qz, phys->rotation.qw);
}

void camera_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing camera...");

    Entity_t *cameraEntity = em_entityCreateHeap();
    cameraEntity->type = ENTITY_TYPE_CAMERA;

    em_entityAddToCollection(&state->entityManager.entityCollections[ENTITY_COLLECTION_PHYSICS], cameraEntity);

    state->context.pCameraEntity = cameraEntity;
    camera_dataCreate(&state->config, cameraEntity);

    camera_eventsSubscribe(state);
}