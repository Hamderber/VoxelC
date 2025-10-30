#pragma region Includes
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "rendering/types/uniformBufferObject_t.h"
#include "rendering/buffers/buffers.h"
#include "core/crash_handler.h"
#include "character/character.h"
#include "rendering/camera/cameraController.h"
#pragma endregion
#pragma region Update
void uniformBuffer_update(State_t *pState)
{
    do
    {
        if (!pState->renderer.pUniformBuffersMapped)
        {
            logs_log(LOG_ERROR, "State has an invalid uniform buffer mapping pointer!");
            break;
        }

        const UniformBufferObject_t CAMERA_UBO = {
            .view = camera_viewMatrix_get(pState),
            .projection = camera_projectionMatrix_get(pState)};

        memcpy(pState->renderer.pUniformBuffersMapped[pState->renderer.currentFrame], &CAMERA_UBO, sizeof(CAMERA_UBO));

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without mapped memory for updating uniform buffers.");
}
#pragma endregion
#pragma region Create
void uniformBuffers_create(State_t *pState)
{
    const VkDeviceSize BUFFER_SIZE = sizeof(UniformBufferObject_t);

    int crashLine = 0;
    do
    {
        pState->renderer.pUniformBuffers = malloc(sizeof(VkBuffer) * pState->config.maxFramesInFlight);
        pState->renderer.pUniformBufferMemories = malloc(sizeof(VkDeviceMemory) * pState->config.maxFramesInFlight);
        pState->renderer.pUniformBuffersMapped = malloc(sizeof(void *) * pState->config.maxFramesInFlight);
        if (!pState->renderer.pUniformBuffers || !pState->renderer.pUniformBufferMemories || !pState->renderer.pUniformBuffersMapped)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for the uniform buffers!");
            break;
        }

        for (size_t i = 0; i < pState->config.maxFramesInFlight; i++)
        {
            bufferCreate(pState, BUFFER_SIZE, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                         &pState->renderer.pUniformBuffers[i], &pState->renderer.pUniformBufferMemories[i]);

            const uint32_t FLAGS = 0;
            const uint32_t OFFSET = 0;
            if (vkMapMemory(pState->context.device, pState->renderer.pUniformBufferMemories[i], OFFSET, BUFFER_SIZE, FLAGS,
                            &pState->renderer.pUniformBuffersMapped[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to map memory for uniform buffer %" PRIu32 "!");
                break;
            }
        }
    } while (0);

    if (crashLine != 0)
    {
        // Don't bother with freeing the partially created UBOs because the program is crashing anyway.
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without uniform buffers to update for rendering.");
    }
}
#pragma endregion
#pragma region Destroy
void uniformBuffers_destroy(State_t *pState)
{
    for (size_t i = 0; i < pState->config.maxFramesInFlight; i++)
    {
        vkDestroyBuffer(pState->context.device, pState->renderer.pUniformBuffers[i], pState->context.pAllocator);
        vkUnmapMemory(pState->context.device, pState->renderer.pUniformBufferMemories[i]);
        vkFreeMemory(pState->context.device, pState->renderer.pUniformBufferMemories[i], pState->context.pAllocator);
        pState->renderer.pUniformBuffersMapped[i] = NULL;
    }

    free(pState->renderer.pUniformBuffers);
    pState->renderer.pUniformBuffers = NULL;
    free(pState->renderer.pUniformBufferMemories);
    pState->renderer.pUniformBufferMemories = NULL;
    free(pState->renderer.pUniformBuffersMapped);
    pState->renderer.pUniformBuffersMapped = NULL;
}
#pragma endregion