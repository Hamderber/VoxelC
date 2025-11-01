#pragma region Includes
#include <stdint.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <inttypes.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Destroy
void syncObjects_destroy(State_t *pState)
{
    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        if (pState->renderer.pInFlightFences != NULL)
        {
            vkDestroyFence(pState->context.device, pState->renderer.pInFlightFences[i], pState->context.pAllocator);
            pState->renderer.pInFlightFences[i] = VK_NULL_HANDLE;
        }

    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        if (pState->renderer.pRenderFinishedSemaphores != NULL)
        {
            vkDestroySemaphore(pState->context.device, pState->renderer.pRenderFinishedSemaphores[i], pState->context.pAllocator);
            pState->renderer.pRenderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }

    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        if (pState->renderer.pImageAcquiredSemaphores != NULL)
        {
            vkDestroySemaphore(pState->context.device, pState->renderer.pImageAcquiredSemaphores[i], pState->context.pAllocator);
            pState->renderer.pImageAcquiredSemaphores[i] = VK_NULL_HANDLE;
        }

    free(pState->renderer.pInFlightFences);
    pState->renderer.pInFlightFences = NULL;

    free(pState->renderer.pImageAcquiredSemaphores);
    pState->renderer.pImageAcquiredSemaphores = NULL;

    free(pState->renderer.pRenderFinishedSemaphores);
    pState->renderer.pRenderFinishedSemaphores = NULL;
}
#pragma endregion
#pragma region Create
void syncObjects_create(State_t *pState)
{
    int crashLine = 0;
    do
    {
        // Destroy the original sync objects if they existed first
        syncObjects_destroy(pState);

        pState->renderer.pImageAcquiredSemaphores = malloc(sizeof(VkSemaphore) * pState->config.maxFramesInFlight);
        if (!pState->renderer.pImageAcquiredSemaphores)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allcoate memory for image acquired semaphors!");
            break;
        }

        pState->renderer.pRenderFinishedSemaphores = malloc(sizeof(VkSemaphore) * pState->config.maxFramesInFlight);
        if (!pState->renderer.pRenderFinishedSemaphores)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allcoate memory for render finished semaphors!");
            break;
        }

        pState->renderer.pInFlightFences = malloc(sizeof(VkFence) * pState->config.maxFramesInFlight);
        if (!pState->renderer.pInFlightFences)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allcoate memory for in-flight fences!");
            break;
        }

        // GPU operations are async so sync is required to aid in parallel execution
        // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
        // Fence: same above but for CPU
        // Binary is just signaled/not signaled. Timeline is more states than 2 (0/1) basically.
        VkSemaphoreCreateInfo SEMAPHORE_CREATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        // Must start with the fences signaled so something actually renders initially
        const VkFenceCreateInfo FENCE_CREATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

        for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        {
            if (vkCreateSemaphore(pState->context.device, &SEMAPHORE_CREATE_INFO, pState->context.pAllocator,
                                  &pState->renderer.pImageAcquiredSemaphores[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to create image acquired semaphore!");
                break;
            }

            if (vkCreateSemaphore(pState->context.device, &SEMAPHORE_CREATE_INFO, pState->context.pAllocator,
                                  &pState->renderer.pRenderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to create render finished semaphore!");
                break;
            }

            if (vkCreateFence(pState->context.device, &FENCE_CREATE_INFO, pState->context.pAllocator,
                              &pState->renderer.pInFlightFences[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to create in-flight fence!");
                break;
            }
        }

        pState->renderer.currentFrame = 0;
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without CPU/GPU sync objects.");
}
#pragma endregion