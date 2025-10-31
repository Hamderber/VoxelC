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
        if (pState->renderer.inFlightFences != NULL)
        {
            vkDestroyFence(pState->context.device, pState->renderer.inFlightFences[i], pState->context.pAllocator);
            pState->renderer.inFlightFences[i] = VK_NULL_HANDLE;
        }

    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        if (pState->renderer.renderFinishedSemaphores != NULL)
        {
            vkDestroySemaphore(pState->context.device, pState->renderer.renderFinishedSemaphores[i], pState->context.pAllocator);
            pState->renderer.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }

    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
        if (pState->renderer.imageAcquiredSemaphores != NULL)
        {
            vkDestroySemaphore(pState->context.device, pState->renderer.imageAcquiredSemaphores[i], pState->context.pAllocator);
            pState->renderer.imageAcquiredSemaphores[i] = VK_NULL_HANDLE;
        }

    free(pState->renderer.inFlightFences);
    pState->renderer.inFlightFences = NULL;

    free(pState->renderer.imageAcquiredSemaphores);
    pState->renderer.imageAcquiredSemaphores = NULL;

    free(pState->renderer.renderFinishedSemaphores);
    pState->renderer.renderFinishedSemaphores = NULL;
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

        pState->renderer.imageAcquiredSemaphores = malloc(sizeof(VkSemaphore) * pState->config.maxFramesInFlight);
        if (!pState->renderer.imageAcquiredSemaphores)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allcoate memory for image acquired semaphors!");
            break;
        }

        pState->renderer.renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * pState->config.maxFramesInFlight);
        if (!pState->renderer.renderFinishedSemaphores)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allcoate memory for render finished semaphors!");
            break;
        }

        pState->renderer.inFlightFences = malloc(sizeof(VkFence) * pState->config.maxFramesInFlight);
        if (!pState->renderer.inFlightFences)
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
                                  &pState->renderer.imageAcquiredSemaphores[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to create image acquired semaphore!");
                break;
            }

            if (vkCreateSemaphore(pState->context.device, &SEMAPHORE_CREATE_INFO, pState->context.pAllocator,
                                  &pState->renderer.renderFinishedSemaphores[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to create render finished semaphore!");
                break;
            }

            if (vkCreateFence(pState->context.device, &FENCE_CREATE_INFO, pState->context.pAllocator,
                              &pState->renderer.inFlightFences[i]) != VK_SUCCESS)
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