#pragma once

#include "core/types/state_t.h"
#include "cmath/cmath.h"
#include "character/character.h"

/// @brief Calculates the camera's perspective view matrix based off of the state's player position and camera rotation
static inline Mat4c_t camera_viewMatrix_get(const State_t *pSTATE)
{
    // Derive forward/up vectors from quaternion orientation
    const Quaternionf_t CAMERA_ROTATION = pSTATE->context.camera.rotation;
    const Vec3f_t CAMERA_FWD = cmath_quat_rotateVec3(CAMERA_ROTATION, VEC3_FORWARD);
    const Vec3f_t CAMERA_UP = cmath_quat_rotateVec3(CAMERA_ROTATION, VEC3_UP);

    const Vec3f_t PLAYER_POSITION = character_player_positionLerped_get(pSTATE);

    return cmath_lookAt(PLAYER_POSITION, cmath_vec3f_add_vec3f(PLAYER_POSITION, CAMERA_FWD), CAMERA_UP);
}

/// @brief Calculates the camera's projection matrix based off of the camera's clipping planes and the image aspect
static inline Mat4c_t camera_projectionMatrix_get(const State_t *pSTATE)
{
    const float FOV = pSTATE->context.camera.fov;
    const float FAR_CLIPPING_PLANE = pSTATE->context.camera.farClippingPlane;
    const float NEAR_CLIPPING_PLANE = pSTATE->context.camera.nearClippingPlane;
    const float ASPECT = (float)pSTATE->window.swapchain.imageExtent.width / (float)pSTATE->window.swapchain.imageExtent.height;

    return cmath_perspective(cmath_deg2radF(FOV), ASPECT, NEAR_CLIPPING_PLANE, FAR_CLIPPING_PLANE);
}

/// @brief Updates camera rotation using current mouse position
void camera_rotation_updateNow(State_t *pState);

/// @brief Initalizes the camera system
void camera_init(State_t *pState);