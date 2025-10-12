#pragma once

#include <vulkan/vulkan.h>
#include "core/state.h"

VkCommandBuffer commandBufferSingleTimeBegin(State_t *state)
{
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->renderer.commandPool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    logs_logIfError(vkAllocateCommandBuffers(state->context.device, &allocateInfo, &commandBuffer),
                    "Failed to allocate command buffer!")

        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

    logs_logIfError(vkBeginCommandBuffer(commandBuffer, &beginInfo),
                    "Failed to begin command buffer!")

        return commandBuffer;
}

void commandBufferSingleTimeEnd(State_t *state, VkCommandBuffer commandBuffer)
{
    logs_logIfError(vkEndCommandBuffer(commandBuffer),
                    "Failed to end command buffer!")

        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
        };

    // A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete,
    // instead of executing one at a time. That may give the driver more opportunities to optimize but is not
    // implemented at this time. Fence is passed as null and we just wait for the transfer queue to be idle right now.
    logs_logIfError(vkQueueSubmit(state->context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),
                    "Failed to submit graphicsQueue!")
        logs_logIfError(vkQueueWaitIdle(state->context.graphicsQueue),
                        "Failed to wait for graphics queue to idle!")

            vkFreeCommandBuffers(state->context.device, state->renderer.commandPool, 1, &commandBuffer);
}