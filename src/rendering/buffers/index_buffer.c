#include <stdlib.h>
#include <string.h>
#include "core/logs.h"
#include <vulkan/vulkan.h>
#include "rendering/buffers/buffers.h"

void indexBufferCreateFromData(State_t *state, uint16_t *indices, uint32_t indexCount)
{
    VkDeviceSize bufferSize = sizeof(uint16_t) * indexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    bufferCreate(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingMemory);

    void *data;
    logs_logIfError(vkMapMemory(state->context.device, stagingMemory, 0, bufferSize, 0, &data),
                    "Failed to map index staging buffer memory.");
    memcpy(data, indices, (size_t)bufferSize);
    vkUnmapMemory(state->context.device, stagingMemory);

    bufferCreate(state, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->renderer.indexBuffer, &state->renderer.indexBufferMemory);

    bufferCopy(state, stagingBuffer, state->renderer.indexBuffer, bufferSize);

    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingMemory, state->context.pAllocator);

    logs_log(LOG_DEBUG, "Created index buffer (%u indices, %zu bytes).", indexCount, (size_t)bufferSize);
}

void indexBufferDestroy(State_t *state)
{
    vkDestroyBuffer(state->context.device, state->renderer.indexBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.indexBufferMemory, state->context.pAllocator);
}