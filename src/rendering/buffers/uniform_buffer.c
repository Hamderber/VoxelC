#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "rendering/types/uniformBufferObject_t.h"
#include "rendering/buffers/buffers.h"

void uniformBuffersCreate(State_t *state)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject_t);
    state->renderer.pUniformBuffers = malloc(sizeof(VkBuffer) * state->config.maxFramesInFlight);
    state->renderer.pUniformBufferMemories = malloc(sizeof(VkDeviceMemory) * state->config.maxFramesInFlight);
    state->renderer.pUniformBuffersMapped = malloc(sizeof(void *) * state->config.maxFramesInFlight);

    for (size_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        bufferCreate(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &state->renderer.pUniformBuffers[i], &state->renderer.pUniformBufferMemories[i]);

        logs_logIfError(vkMapMemory(state->context.device, state->renderer.pUniformBufferMemories[i], 0, bufferSize, 0,
                                    &state->renderer.pUniformBuffersMapped[i]),
                        "Failed to map memory for uniform buffer!");
    }
}

void uniformBuffersDestroy(State_t *state)
{
    for (size_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        vkDestroyBuffer(state->context.device, state->renderer.pUniformBuffers[i], state->context.pAllocator);
        vkUnmapMemory(state->context.device, state->renderer.pUniformBufferMemories[i]);
        vkFreeMemory(state->context.device, state->renderer.pUniformBufferMemories[i], state->context.pAllocator);
        state->renderer.pUniformBuffersMapped[i] = NULL;
    }

    free(state->renderer.pUniformBuffers);
    state->renderer.pUniformBuffers = NULL;
    free(state->renderer.pUniformBufferMemories);
    state->renderer.pUniformBufferMemories = NULL;
}

void updateUniformBuffer(State_t *state)
{
    // For animation we want the total time so that the shaders can rotate/etc objects and display them where they should be
    // instead of having stutter with frames.

    float rotateDegreesY = 0.0F;
    float rotateDegreesX = 0.0F;
    float rotateDegreesZ = 0.0F;
    float farClippingPlane = 50.0F;

    Quaternionf_t qYaw = cmath_quat_fromAxisAngle(cmath_deg2radF(rotateDegreesY) * (float)state->time.frameTimeTotal, VEC3_Y_AXIS);
    Quaternionf_t qPitch = cmath_quat_fromAxisAngle(cmath_deg2radF(rotateDegreesX) * (float)state->time.frameTimeTotal, VEC3_X_AXIS);
    Quaternionf_t qRoll = cmath_quat_fromAxisAngle(cmath_deg2radF(rotateDegreesZ) * (float)state->time.frameTimeTotal, VEC3_Z_AXIS);
    Quaternionf_t qTemp = cmath_quat_mult_quat(qYaw, qPitch);
    Quaternionf_t qCombined = cmath_quat_mult_quat(qTemp, qRoll);
    Mat4c_t model = cmath_quat2mat(qCombined);

    float fov = state->context.camera.fov;
    Vec3f_t pos = VEC3_ZERO;
    Quaternionf_t rot = state->context.camera.rotation;

    // EntityComponentData_t *cameraData;
    // if (em_entityDataGet(state->context.pCamera, ENTITY_COMPONENT_TYPE_CAMERA, &cameraData))
    // {
    //     fov = cameraData->cameraData->fov;
    // }

    EntityComponentData_t *playerPhysicsData;
    if (em_entityDataGet(state->worldState->pPlayerEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &playerPhysicsData))
    {
        // Blend position for camera because its updated in physics but not required for rotation at this time
        float alpha = (float)(state->time.fixedTimeAccumulated / state->config.fixedTimeStep);
        alpha = cmath_clampF(alpha, 0.0F, 1.0F);

        Vec3f_t posPrev = playerPhysicsData->physicsData->posOld;
        Vec3f_t posCurr = playerPhysicsData->physicsData->pos;
        pos = cmath_vec3f_lerpF(posPrev, posCurr, alpha);
    }

    // Derive forward/up vectors from quaternion orientation
    Vec3f_t forward = cmath_quat_rotateVec3(rot, VEC3_FORWARD);
    Vec3f_t up = cmath_quat_rotateVec3(rot, VEC3_UP);

    Mat4c_t view = cmath_lookAt(pos, cmath_vec3f_add_vec3f(pos, forward), up);

    UniformBufferObject_t ubo = {
        .model = model,
        .view = view,
        .projection = cmath_perspective(cmath_deg2radF(fov),
                                        state->window.swapchain.imageExtent.width / (float)state->window.swapchain.imageExtent.height,
                                        0.1F, farClippingPlane),
    };

    memcpy(state->renderer.pUniformBuffersMapped[state->renderer.currentFrame], &ubo, sizeof(ubo));
}
