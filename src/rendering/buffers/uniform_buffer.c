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

    float rotateDegreesY = 45.0F;
    float rotateDegreesX = 45.0F;
    float rotateDegreesZ = 45.0F;

    Quaternion_t qYaw = cm_quatAngleAxis(cm_deg2radf(rotateDegreesY) * (float)state->time.frameTimeTotal, Y_AXIS);
    Quaternion_t qPitch = cm_quatAngleAxis(cm_deg2radf(rotateDegreesX) * (float)state->time.frameTimeTotal, X_AXIS);
    Quaternion_t qRoll = cm_quatAngleAxis(cm_deg2radf(rotateDegreesZ) * (float)state->time.frameTimeTotal, Z_AXIS);
    Quaternion_t qTemp = cm_quatMultiply(qYaw, qPitch);
    Quaternion_t qCombined = cm_quatMultiply(qTemp, qRoll);
    Mat4c_t model = cm_quat2mat(qCombined);

    UniformBufferObject_t ubo = {
        .model = model,
        .view = cm_lookAt((Vec3f_t){0.0F, 3.0F, -3.0F}, // camera position
                          VEC3_ZERO,                    // look at origin
                          UP),                          // up = +Y
        .projection = cm_perspective(cm_deg2radf(45.0F),
                                     state->window.swapchain.imageExtent.width / (float)state->window.swapchain.imageExtent.height,
                                     0.1F, 10.0F),
    };

    memcpy(state->renderer.pUniformBuffersMapped[state->renderer.currentFrame], &ubo, sizeof(ubo));
}
