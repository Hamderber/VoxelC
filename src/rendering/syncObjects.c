#include <stdint.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/types/state_t.h"

void syncObjectsDestroy(State_t *state)
{
    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.inFlightFences != NULL)
        {
            vkDestroyFence(state->context.device, state->renderer.inFlightFences[i], state->context.pAllocator);
            state->renderer.inFlightFences[i] = VK_NULL_HANDLE;
        }
    }

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.renderFinishedSemaphores != NULL)
        {
            vkDestroySemaphore(state->context.device, state->renderer.renderFinishedSemaphores[i], state->context.pAllocator);
            state->renderer.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.imageAcquiredSemaphores != NULL)
        {
            vkDestroySemaphore(state->context.device, state->renderer.imageAcquiredSemaphores[i], state->context.pAllocator);
            state->renderer.imageAcquiredSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    free(state->renderer.inFlightFences);
    state->renderer.inFlightFences = NULL;
    free(state->renderer.imageAcquiredSemaphores);
    state->renderer.imageAcquiredSemaphores = NULL;
    free(state->renderer.renderFinishedSemaphores);
    state->renderer.renderFinishedSemaphores = NULL;
}

void syncObjectsCreate(State_t *state)
{
    // Destroy the original sync objects if they existed first
    syncObjectsDestroy(state);

    state->renderer.imageAcquiredSemaphores = malloc(sizeof(VkSemaphore) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.imageAcquiredSemaphores == NULL,
                    "Failed to allcoate memory for image acquired semaphors!");

    state->renderer.renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.renderFinishedSemaphores == NULL,
                    "Failed to allcoate memory for render finished semaphors!");

    state->renderer.inFlightFences = malloc(sizeof(VkFence) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.inFlightFences == NULL,
                    "Failed to allcoate memory for in-flight fences!");

    // GPU operations are async so sync is required to aid in parallel execution
    // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
    // Fence: same above but for CPU
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        // Binary is just signaled/not signaled. Timeline is more states than 2 (0/1) basically.
    };

    // Must start with the fences signaled so something actually renders initially
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        logs_logIfError(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                          &state->renderer.imageAcquiredSemaphores[i]),
                        "Failed to create image acquired semaphore");

        logs_logIfError(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                          &state->renderer.renderFinishedSemaphores[i]),
                        "Failed to create render finished semaphore");

        logs_logIfError(vkCreateFence(state->context.device, &fenceCreateInfo, state->context.pAllocator,
                                      &state->renderer.inFlightFences[i]),
                        "Failed to create in-flight fence");
    }

    state->renderer.currentFrame = 0;
}
