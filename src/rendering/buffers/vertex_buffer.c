#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include <vulkan/vulkan.h>
#include "rendering/buffers/buffers.h"
#include "rendering/types/shaderVertex_t.h"

void vertexBufferCreateFromData(State_t *state, ShaderVertex_t *vertices, uint32_t vertexCount)
{
    VkDeviceSize bufferSize = sizeof(ShaderVertex_t) * vertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // Create staging buffer (CPU visible)
    bufferCreate(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingMemory);

    void *data;
    logs_logIfError(vkMapMemory(state->context.device, stagingMemory, 0, bufferSize, 0, &data),
                    "Failed to map vertex staging buffer memory.");
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(state->context.device, stagingMemory);

    // Create actual GPU vertex buffer
    bufferCreate(state, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->renderer.vertexBuffer, &state->renderer.vertexBufferMemory);

    // Copy from staging to GPU
    bufferCopy(state, stagingBuffer, state->renderer.vertexBuffer, bufferSize);

    // Cleanup staging
    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingMemory, state->context.pAllocator);

    logs_log(LOG_DEBUG, "Created vertex buffer (%u vertices, %zu bytes).", vertexCount, (size_t)bufferSize);
}

void vertexBufferDestroy(State_t *state)
{
    vkDestroyBuffer(state->context.device, state->renderer.vertexBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.vertexBufferMemory, state->context.pAllocator);
}