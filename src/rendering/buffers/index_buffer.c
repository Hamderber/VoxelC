#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "rendering/buffers/buffers.h"
#include "core/crash_handler.h"
#include "core/vkWrappers.h"
#pragma endregion
#pragma region Defines
// #define INDEX_BUFFER_DEBUG
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
        if (vkMapMemory_wrapper("Generic Index Buffer", pState->context.device, stagingMemory, OFFSET, BUFFER_SIZE, FLAGS, &pData) !=
            VK_SUCCESS)
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
#ifdef INDEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Created index buffer (%" PRIu32 " indices, %" PRIu32 " bytes).", INDEX_COUNT, (uint32_t)BUFFER_SIZE);
#endif
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine), "The program cannot continue without creating an index buffer.");
}

void indexBuffer_createEmpty(State_t *restrict pState, const uint32_t CAPACITY, VkBuffer *restrict pOutBuffer,
                             VkDeviceMemory *restrict pOutMemory)
{
    int crashLine = 0;
    do
    {
        if (CAPACITY == 0)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted to create a vertex buffer with capacity 0!");
            break;
        }

        if (!pState || !pOutBuffer || !pOutMemory)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        const VkDeviceSize BUFFER_SIZE = sizeof(uint32_t) * CAPACITY;

        // Create actual GPU index buffer
        bufferCreate(pState, BUFFER_SIZE,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     pOutBuffer, pOutMemory);

#ifdef VERTEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Created index buffer (empty) (%" PRIu32 " vertices, %" PRIu32 " bytes).", CAPACITY, (uint32_t)BUFFER_SIZE);
#endif
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine), "The program cannot continue without creating an index buffer.");
}

void indexBuffer_updateFromData_Voxel(State_t *restrict pState, uint32_t *restrict pVertices, uint32_t indexCursor,
                                      VkBuffer buffer)
{
    int crashLine = 0;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    do
    {
        if (indexCursor == 0)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted to update a index buffer with a cursor at 0!");
            break;
        }

        if (!pState || !pVertices)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        const VkDeviceSize BUFFER_SIZE = sizeof(uint32_t) * indexCursor;

        // Create staging buffer (CPU visible)
        bufferCreate(pState, BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stagingBuffer, &stagingMemory);

        void *pData;
        if (vkMapMemory_wrapper("Chunk Vertex Index (staging)", pState->context.device, stagingMemory, 0, BUFFER_SIZE, 0, &pData) !=
            VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to map index staging buffer memory!");
            break;
        }

        memcpy(pData, pVertices, (size_t)BUFFER_SIZE);
        vkUnmapMemory(pState->context.device, stagingMemory);

        // Copy from staging to GPU
        bufferCopy(pState, stagingBuffer, buffer, BUFFER_SIZE);
#ifdef VERTEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Updated index buffer (%" PRIu32 " vertices, %" PRIu32 " bytes).", indexCursor, (uint32_t)BUFFER_SIZE);
#endif
    } while (0);

    if (stagingBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(pState->context.device, stagingBuffer, pState->context.pAllocator);
    if (stagingMemory != VK_NULL_HANDLE)
        vkFreeMemory(pState->context.device, stagingMemory, pState->context.pAllocator);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without index buffers.");
}

void indexBuffer_createFromData_Voxel(State_t *pState, uint32_t *pIndices, const uint32_t INDEX_COUNT,
                                      VkBuffer *pOutBuffer, VkDeviceMemory *pOutMemory)
{
    int crashLine = 0;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    do
    {
        if (!pState || !pIndices || !pOutBuffer || !pOutMemory)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved invalid pointers!");
            break;
        }

        const VkDeviceSize BUFFER_SIZE = sizeof(uint32_t) * INDEX_COUNT;

        bufferCreate(pState, BUFFER_SIZE, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stagingBuffer, &stagingMemory);

        const VkDeviceSize OFFSET = 0;
        const VkMemoryMapFlags FLAGS = 0;
        void *pData;
        if (vkMapMemory_wrapper("Chunk Index Buffer", pState->context.device, stagingMemory, OFFSET, BUFFER_SIZE, FLAGS, &pData) != VK_SUCCESS)
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
                     pOutBuffer, pOutMemory);

        bufferCopy(pState, stagingBuffer, *pOutBuffer, BUFFER_SIZE);
#ifdef INDEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Created index buffer (%" PRIu32 " indices, %" PRIu32 " bytes).", INDEX_COUNT, (uint32_t)BUFFER_SIZE);
#endif
    } while (0);

    if (stagingBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(pState->context.device, stagingBuffer, pState->context.pAllocator);
    if (stagingMemory != VK_NULL_HANDLE)
        vkFreeMemory(pState->context.device, stagingMemory, pState->context.pAllocator);

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