#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>
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

    Quaternion_t qYaw = cm_quatFromAxisAngle(cm_deg2radf(rotateDegreesY) * (float)state->time.frameTimeTotal, Y_AXIS);
    Quaternion_t qPitch = cm_quatFromAxisAngle(cm_deg2radf(rotateDegreesX) * (float)state->time.frameTimeTotal, X_AXIS);
    Quaternion_t qRoll = cm_quatFromAxisAngle(cm_deg2radf(rotateDegreesZ) * (float)state->time.frameTimeTotal, Z_AXIS);
    Quaternion_t qTemp = cm_quatMultiply(qYaw, qPitch);
    Quaternion_t qCombined = cm_quatMultiply(qTemp, qRoll);
    Mat4c_t model = cm_quat2mat(qCombined);

    float fov = 0.0F;
    Vec3f_t pos = VEC3_ZERO;
    Quaternion_t rot = QUATERNION_IDENTITY;

    EntityComponentData_t *cameraData;
    if (em_entityDataGet(state->context.pCameraEntity, ENTITY_COMPONENT_TYPE_CAMERA, &cameraData))
    {
        fov = cameraData->cameraData->fov;
    }

    EntityComponentData_t *cameraPhysicsData;
    if (em_entityDataGet(state->context.pCameraEntity, ENTITY_COMPONENT_TYPE_PHYSICS, &cameraPhysicsData))
    {
        pos = cameraPhysicsData->physicsData->pos;
        rot = cameraPhysicsData->physicsData->rotation;
    }

    // Derive forward/up vectors from quaternion orientation
    Vec3f_t forward = cm_quatRotateVec3(rot, FORWARD);
    Vec3f_t up = cm_quatRotateVec3(rot, UP);

    Mat4c_t view = cm_lookAt(pos, cm_vec3fSum(pos, forward), up);

    UniformBufferObject_t ubo = {
        .model = model,
        .view = view,
        .projection = cm_perspective(cm_deg2radf(fov),
                                     state->window.swapchain.imageExtent.width / (float)state->window.swapchain.imageExtent.height,
                                     0.1F, farClippingPlane),
    };

    memcpy(state->renderer.pUniformBuffersMapped[state->renderer.currentFrame], &ubo, sizeof(ubo));
}
