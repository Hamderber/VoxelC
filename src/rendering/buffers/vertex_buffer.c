#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <inttypes.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "rendering/buffers/buffers.h"
#include "rendering/types/shaderVertexModel_t.h"
#include "rendering/types/shaderVertexVoxel_t.h"
#include "core/crash_handler.h"
#include "core/vkWrappers.h"
#pragma endregion
#pragma region Defines
// #define VERTEX_BUFFER_DEBUG
#pragma endregion
#pragma region Create
void vertexBuffer_createFromData_Model(State_t *pState, ShaderVertexModel_t *pVertices, uint32_t vertexCount)
{
    int crashLine = 0;
    do
    {
        if (!pState || !pVertices)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        VkDeviceSize bufferSize = sizeof(ShaderVertexModel_t) * vertexCount;
        VkBuffer stagingBuffer = NULL;
        VkDeviceMemory stagingMemory = NULL;

        // Create staging buffer (CPU visible)
        bufferCreate(pState, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stagingBuffer, &stagingMemory);

        void *pData;
        if (vkMapMemory_wrapper("Model Vertex Buffer", pState->context.device, stagingMemory, 0, bufferSize, 0, &pData) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to map vertex staging buffer memory!");
            break;
        }

        memcpy(pData, pVertices, (size_t)bufferSize);
        vkUnmapMemory(pState->context.device, stagingMemory);

        // Create actual GPU vertex buffer
        bufferCreate(pState, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     &pState->renderer.vertexBuffer, &pState->renderer.vertexBufferMemory);

        // Copy from staging to GPU
        bufferCopy(pState, stagingBuffer, pState->renderer.vertexBuffer, bufferSize);

        // Cleanup staging
        vkDestroyBuffer(pState->context.device, stagingBuffer, pState->context.pAllocator);
        vkFreeMemory(pState->context.device, stagingMemory, pState->context.pAllocator);

#ifdef VERTEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Created vertex buffer (%" PRIu32 " vertices, %" PRIu32 " bytes).", vertexCount, (uint32_t)bufferSize);
#endif
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without vertex buffers.");
}

void vertexBuffer_createEmpty(State_t *restrict pState, uint32_t capacity, VkBuffer *restrict pOutBuffer,
                              VkDeviceMemory *restrict pOutMemory)
{
    int crashLine = 0;
    do
    {
        if (capacity == 0)
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

        VkDeviceSize bufferSize = sizeof(ShaderVertexVoxel_t) * capacity;

        // Create actual GPU vertex buffer
        bufferCreate(pState, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     pOutBuffer, pOutMemory);

#ifdef VERTEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Created vertex buffer (empty) (%" PRIu32 " vertices, %" PRIu32 " bytes).", capacity, (uint32_t)bufferSize);
#endif
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without vertex buffers.");
}

void vertexBuffer_updateFromData_Voxel(State_t *restrict pState, ShaderVertexVoxel_t *restrict pVertices, uint32_t vertexCursor,
                                       VkBuffer buffer)
{
    int crashLine = 0;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    do
    {
        if (vertexCursor == 0)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted to update a vertex buffer with a cursor at 0!");
            break;
        }

        if (!pState || !pVertices)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        VkDeviceSize bufferSize = sizeof(ShaderVertexVoxel_t) * vertexCursor;

        // Create staging buffer (CPU visible)
        bufferCreate(pState, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stagingBuffer, &stagingMemory);

        void *pData;
        if (vkMapMemory_wrapper("Chunk Vertex Buffer (staging)", pState->context.device, stagingMemory, 0, bufferSize, 0, &pData) !=
            VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to map vertex staging buffer memory!");
            break;
        }

        memcpy(pData, pVertices, (size_t)bufferSize);
        vkUnmapMemory(pState->context.device, stagingMemory);

        // Copy from staging to GPU
        bufferCopy(pState, stagingBuffer, buffer, bufferSize);
#ifdef VERTEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Updated vertex buffer (%" PRIu32 " vertices, %" PRIu32 " bytes).", vertexCursor, (uint32_t)bufferSize);
#endif
    } while (0);

    if (stagingBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(pState->context.device, stagingBuffer, pState->context.pAllocator);
    if (stagingMemory != VK_NULL_HANDLE)
        vkFreeMemory(pState->context.device, stagingMemory, pState->context.pAllocator);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without vertex buffers.");
}

void vertexBuffer_createFromData_Voxel(State_t *pState, ShaderVertexVoxel_t *pVertices, uint32_t vertexCount,
                                       VkBuffer *pOutBuffer, VkDeviceMemory *pOutMemory)
{
    int crashLine = 0;
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    do
    {
        if (!pState || !pVertices || !pOutBuffer || !pOutMemory)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        VkDeviceSize bufferSize = sizeof(ShaderVertexVoxel_t) * vertexCount;

        // Create staging buffer (CPU visible)
        bufferCreate(pState, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &stagingBuffer, &stagingMemory);

        void *pData;
        if (vkMapMemory_wrapper("Chunk Vertex Buffer", pState->context.device, stagingMemory, 0, bufferSize, 0, &pData) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to map vertex staging buffer memory!");
            break;
        }

        memcpy(pData, pVertices, (size_t)bufferSize);
        vkUnmapMemory(pState->context.device, stagingMemory);

        // Create actual GPU vertex buffer
        bufferCreate(pState, bufferSize,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                     pOutBuffer, pOutMemory);

        // Copy from staging to GPU
        bufferCopy(pState, stagingBuffer, *pOutBuffer, bufferSize);
#ifdef VERTEX_BUFFER_DEBUG
        logs_log(LOG_DEBUG, "Created vertex buffer (%" PRIu32 " vertices, %" PRIu32 " bytes).", vertexCount, (uint32_t)bufferSize);
#endif
    } while (0);

    if (stagingBuffer != VK_NULL_HANDLE)
        vkDestroyBuffer(pState->context.device, stagingBuffer, pState->context.pAllocator);
    if (stagingMemory != VK_NULL_HANDLE)
        vkFreeMemory(pState->context.device, stagingMemory, pState->context.pAllocator);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without vertex buffers.");
}
#pragma endregion
#pragma region Destroy
void vertexBuffer_destroy(State_t *pState)
{
    vkDestroyBuffer(pState->context.device, pState->renderer.vertexBuffer, pState->context.pAllocator);
    vkFreeMemory(pState->context.device, pState->renderer.vertexBufferMemory, pState->context.pAllocator);
}
#pragma endregion
#pragma region
#undef VERTEX_BUFFER_DEBUG
#pragma endregion