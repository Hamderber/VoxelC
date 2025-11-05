#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "rendering/buffers/buffers.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Create
void indexBuffer_createFromData(State_t *pState, uint32_t *pIndices, const uint32_t INDEX_COUNT)
{
    int crashLine = 0;
    do
    {
        if (!pState || !pIndices)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved invalid pointers!");
            break;
        }

        const VkDeviceSize BUFFER_SIZE = sizeof(uint32_t) * INDEX_COUNT;

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;
        bufferCreate(pState, BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stagingBuffer, &stagingMemory);

        const VkDeviceSize OFFSET = 0;
        const VkMemoryMapFlags FLAGS = 0;
        void *pData;
        if (vkMapMemory(pState->context.device, stagingMemory, OFFSET, BUFFER_SIZE, FLAGS, &pData) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to map index staging buffer memory!");
            break;
        }

        memcpy(pData, pIndices, (size_t)BUFFER_SIZE);
        vkUnmapMemory(pState->context.device, stagingMemory);

        bufferCreate(pState, BUFFER_SIZE,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     &pState->renderer.indexBuffer, &pState->renderer.indexBufferMemory);

        bufferCopy(pState, stagingBuffer, pState->renderer.indexBuffer, BUFFER_SIZE);

        vkDestroyBuffer(pState->context.device, stagingBuffer, pState->context.pAllocator);
        vkFreeMemory(pState->context.device, stagingMemory, pState->context.pAllocator);

        logs_log(LOG_DEBUG, "Created index buffer (%" PRIu32 " indices, %" PRIu32 " bytes).", INDEX_COUNT, (uint32_t)BUFFER_SIZE);
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine), "The program cannot continue without creating an index buffer.");
}
#pragma endregion
#pragma region Destroy
void indexBuffer_destroy(State_t *pState)
{
    vkDestroyBuffer(pState->context.device, pState->renderer.indexBuffer, pState->context.pAllocator);
    vkFreeMemory(pState->context.device, pState->renderer.indexBufferMemory, pState->context.pAllocator);
}
#pragma endregion