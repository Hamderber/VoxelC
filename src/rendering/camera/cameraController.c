#pragma region Includes
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
#include "input/types/inputActionQuery_t.h"
#include "input/input.h"
#pragma endregion
#pragma region Rotation Update
void camera_rotation_updateNow(State_t *pState)
{
    const float DX = (float)pState->gui.mouse.dx;
    const float DY = (float)pState->gui.mouse.dy;

    // magic number constant to convert mouse sensitivity to a reasonable radians per pixel scalar
    const float RAD_PER_PIXEL = 0.0025F * (float)pState->config.mouseSensitivity;
    // yaw left/right (neg for natural feel)
    const float YAW_DELTA = -DX * RAD_PER_PIXEL;
    // pitch up/down
    const float PITCH_DELTA = DY * RAD_PER_PIXEL;

    /* Rotation is applied first with YAW in WORLD space (rotating around y axis). Then, PITCH is applied in LOCAL space
    (rotating around z axis). This is because the looking left/right is a world perspective reference while looking up/down
    is dependent on the direction of the local z axis. */

    // WORLD
    const Quaternionf_t YAW = cmath_quat_fromAxisAngle(YAW_DELTA, VEC3_UP);
    pState->context.camera.rotation = cmath_quat_normalize(cmath_quat_mult_quat(YAW, pState->context.camera.rotation));

    // LOCAL
    const Vec3f_t RIGHT = cmath_quat_rotateVec3(pState->context.camera.rotation, VEC3_RIGHT);
    const Quaternionf_t PITCH = cmath_quat_fromAxisAngle(PITCH_DELTA, RIGHT);
    pState->context.camera.rotation = cmath_quat_normalize(cmath_quat_mult_quat(PITCH, pState->context.camera.rotation));

    const Vec3f_t FWD = cmath_quat_rotateVec3(pState->context.camera.rotation, VEC3_FORWARD);

    // camera look direction vs horizon
    const float PITCH_F = asinf(FWD.y);
    // Slightly less than pi/2 so that wrap-around/quat errors are avoided
    const float MAX_PITCH = (PI_F * 0.5F) - 0.001F;
    if (PITCH_F > MAX_PITCH || PITCH_F < -MAX_PITCH)
    {
        // undo the last pitch if it exceeded clamp
        float correction = PITCH_F > 0 ? (PITCH_F - MAX_PITCH) : (PITCH_F + MAX_PITCH);
        Quaternionf_t qUndo = cmath_quat_fromAxisAngle(-correction, RIGHT);
        pState->context.camera.rotation = cmath_quat_normalize(cmath_quat_mult_quat(qUndo, pState->context.camera.rotation));
    }
}
#pragma endregion
#pragma region Zoom
static const float FOV_ZOOM_MULTIPLIER = 0.333F;

/// @brief Resets camera FOV
static inline void zoomFOV_undoZoom(State_t *pState) { pState->context.camera.fov = pState->config.cameraFOV; }

/// @brief Sets camera FOV to zoom
static inline void zoomFOV_zoom(State_t *pState) { pState->context.camera.fov = pState->config.cameraFOV * FOV_ZOOM_MULTIPLIER; }

/// @brief Toggles camera zoom
static EventResult_t zoom_onPress(State_t *pState, Event_t *pEvent, void *pCtx)
{
    pCtx;
    if (pEvent == NULL)
        return EVENT_RESULT_ERROR;

    const InputActionQuery_t pQUERY[] = {
        {
            .mapping = INPUT_ACTION_FOV_ZOOM,
            .actionCtx = CTX_INPUT_ACTION_START,
        },
        {
            .mapping = INPUT_ACTION_FOV_ZOOM,
            .actionCtx = CTX_INPUT_ACTION_END,
        },
    };

    InputAction_t pQueryResult[sizeof pQUERY / sizeof pQUERY[0]];
    const size_t SIZE = input_inputAction_matchQuery(pEvent, pQUERY, sizeof pQUERY / sizeof pQUERY[0], pQueryResult);

    if (SIZE > 0)
        for (size_t i = 0; i < SIZE; i++)
        {
            const InputAction_t ACTION = pQueryResult[i];

            switch (ACTION.action)
            {
            case INPUT_ACTION_FOV_ZOOM:
                if (ACTION.actionState == CTX_INPUT_ACTION_START)
                {
                    logs_log(LOG_DEBUG, "Camera FOV zoom (pressed)");
                    zoomFOV_zoom(pState);
                }
                else if (ACTION.actionState == CTX_INPUT_ACTION_END)
                {
                    logs_log(LOG_DEBUG, "Camera FOV zoom (released)");
                    zoomFOV_undoZoom(pState);
                }
                break;
            }
        }

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Sprint FOV
const float SPRINT_FOV_INCREASE = 20.0F;
void camera_sprintFOV_toggle(State_t *pState, const bool TOGGLE)
{
    if (TOGGLE)
        pState->context.camera.fov += SPRINT_FOV_INCREASE;
    else
        pState->context.camera.fov -= SPRINT_FOV_INCREASE;
}

#pragma endregion
#pragma region Create
/// @brief Subscribe events for camera
static inline void eventsSubscribe(State_t *pState)
{
    events_subscribe(&pState->eventBus, EVENT_CHANNEL_INPUT_ACTIONS, zoom_onPress, false, false, NULL);
}

/// @brief Create camera data
static void data_create(State_t *pState)
{
    pState->context.camera.fov = pState->config.cameraFOV;
    logs_log(LOG_DEBUG, "Camera has FOV %lf", pState->context.camera.fov);

    pState->context.camera.farClippingPlane = pState->config.cameraFarClippingPlane;
    pState->context.camera.nearClippingPlane = pState->config.cameraNearClippingPlane;
    logs_log(LOG_DEBUG, "Camera has a clipping plane of %lf to %lf meters.",
             pState->context.camera.nearClippingPlane, pState->context.camera.farClippingPlane);
}

void camera_init(State_t *pState)
{
    logs_log(LOG_DEBUG, "Initializing camera...");

    data_create(pState);

    eventsSubscribe(pState);
}
#pragma endregion