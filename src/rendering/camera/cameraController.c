#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/random.h"
#include "events/eventTypes.h"
#include "events/eventBus.h"
#include "events/context/CtxInputMapped_t.h"
#include "entity/entityManager.h"
#include "cmath/cmath.h"
#include <stdlib.h>
#include <string.h>
#include "character/characterController.h"

static const float FOV_ZOOM_MULTIPLIER = 0.333F;

void camera_dataCreate(State_t *pState)
{
    pState->context.camera.fov = pState->config.cameraFOV;
    logs_log(LOG_DEBUG, "Camera has FOV %lf", pState->context.camera.fov);

    pState->context.camera.farClippingPlane = pState->config.cameraFarClippingPlane;
    pState->context.camera.nearClippingPlane = pState->config.cameraNearClippingPlane;
    logs_log(LOG_DEBUG, "Camera has a clipping plane of %lf to %lf meters.",
             pState->context.camera.nearClippingPlane, pState->context.camera.farClippingPlane);
}

void camera_rotationUpdateNow(State_t *state)
{
    float dx = (float)state->gui.mouse.dx;
    float dy = (float)state->gui.mouse.dy;

    const float radPerPixel = 0.0025F * (float)state->config.mouseSensitivity;
    // yaw left/right (neg for natural feel)
    float yawDelta = -dx * radPerPixel;
    // pitch up/down
    float pitchDelta = dy * radPerPixel;

    // Apply yaw in WORLD space
    Quaternionf_t qYaw = cmath_quat_fromAxisAngle(yawDelta, VEC3_UP);
    state->context.camera.rotation = cmath_quat_normalize(cmath_quat_mult_quat(qYaw, state->context.camera.rotation));

    // Apply pitch in LOCAL space
    Vec3f_t right = cmath_quat_rotateVec3(state->context.camera.rotation, VEC3_RIGHT);
    Quaternionf_t qPitch = cmath_quat_fromAxisAngle(pitchDelta, right);
    state->context.camera.rotation = cmath_quat_normalize(cmath_quat_mult_quat(qPitch, state->context.camera.rotation));

    // Clamp pitc
    Vec3f_t fwd = cmath_quat_rotateVec3(state->context.camera.rotation, VEC3_FORWARD);
    float pitch = asinf(fwd.y); // camera look direction vs horizon
    const float maxPitch = (PI_F * 0.5f) - 0.001f;
    if (pitch > maxPitch || pitch < -maxPitch)
    {
        // undo the last pitch if it exceeded clamp
        float correction = pitch > 0 ? (pitch - maxPitch) : (pitch + maxPitch);
        Quaternionf_t qUndo = cmath_quat_fromAxisAngle(-correction, right);
        state->context.camera.rotation = cmath_quat_normalize(cmath_quat_mult_quat(qUndo, state->context.camera.rotation));
    }

    // logs_log(LOG_DEBUG,
    //          "Camera quat (%.4f, %.4f, %.4f, %.4f)",
    //          state->context.camera.rotation.qx, state->context.camera.rotation.qy,
    //          state->context.camera.rotation.qz, state->context.camera.rotation.qw);
    // }
}

void camera_zoomFOV_undo(State_t *state)
{
    state->context.camera.fov = state->config.cameraFOV;
}

void camera_zoomFOV(State_t *state)
{
    state->context.camera.fov = state->config.cameraFOV * FOV_ZOOM_MULTIPLIER;
}

EventResult_t camera_onZoomPress(State_t *state, Event_t *event, void *ctx)
{
    // Don't zoom if a menu is open
    if (state->gui.menuDepth != GUI_ID_NONE)
    {
        return EVENT_RESULT_PASS;
    }

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
                case INPUT_ACTION_FOV_ZOOM:
                    logs_log(LOG_DEBUG, "Camera FOV zoom (pressed)");
                    camera_zoomFOV(state);
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
                case INPUT_ACTION_FOV_ZOOM:
                    logs_log(LOG_DEBUG, "Camera FOV zoom (released)");
                    camera_zoomFOV_undo(state);
                    break;
                default:
                    break;
                }
            }
        }
    }

    return EVENT_RESULT_PASS;
}

void camera_eventsSubscribe(State_t *state)
{
    events_subscribe(&state->eventBus, EVENT_CHANNEL_INPUT_ACTIONS, camera_onZoomPress, false, false, NULL);
}

void camera_init(State_t *state)
{
    logs_log(LOG_DEBUG, "Initializing camera...");

    camera_dataCreate(state);

    camera_eventsSubscribe(state);
}