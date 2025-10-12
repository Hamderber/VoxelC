#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"

void commandBufferAllocate(State_t *state)
{
    // free previous commmand buffers if present
    if (state->renderer.pCommandBuffers != NULL)
    {
        vkFreeCommandBuffers(state->context.device, state->renderer.commandPool,
                             state->config.maxFramesInFlight, state->renderer.pCommandBuffers);
        free(state->renderer.pCommandBuffers);
        state->renderer.pCommandBuffers = NULL;
    }

    state->renderer.pCommandBuffers = malloc(sizeof(VkCommandBuffer) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.pCommandBuffers == NULL,
                    "Failed to allocate memory for command buffers")

        VkCommandBufferAllocateInfo allocateInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = state->renderer.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = state->config.maxFramesInFlight,
        };

    logs_logIfError(vkAllocateCommandBuffers(state->context.device, &allocateInfo, state->renderer.pCommandBuffers),
                    "Failed to allocate command buffers")
}

void commandBufferRecord(State_t *state)
{
    // Skip this recording frame if the swapchain will be/is being recreated (avoids null pointers)
    if (state->window.swapchain.recreate)
        return;

    VkRect2D renderArea = {
        .extent = state->window.swapchain.imageExtent,
    };

    // Which color values to clear when using the clear operation defined in the attachments of the render pass.
    // Order of clear values must be equal to order of attachments
    VkClearValue clearValues[] = {
        // Clears image but leaves a background color
        {
            // Black
            .color.float32 = {0.0F, 0.0F, 0.0F, 0.0F},
        },
        // Resets the depth stencil
        {
            .depthStencil = {1.0f, 0},
        },
    };

    // Avoid access violations
    if (!state->renderer.pFramebuffers || state->window.swapchain.imageAcquiredIndex >= state->renderer.framebufferCount)
    {
        logs_log(LOG_WARN, "Skipped an access violation during frame buffer access during commandBufferRecord!");
        return;
    }

    uint32_t frameIndex = state->renderer.currentFrame;
    VkCommandBuffer cmd = state->renderer.pCommandBuffers[frameIndex];

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };

    VkResult r = vkResetCommandBuffer(cmd, 0);
    if (r != VK_SUCCESS)
    {
        // Don't just do logs_logIfError because we want to early exit here
        logs_log(LOG_ERROR, "vkResetCommandBuffer failed (%d) for frame %u", r, frameIndex);
        return;
    }

    logs_logIfError(vkBeginCommandBuffer(cmd, &commandBufferBeginInfo),
                    "Failed to begin command buffer (frame %u)", frameIndex);

    // ALL vkCmd functions (commands) MUST go between the begin and end command buffer functions (obviously)
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->renderer.pRenderPass,
        .framebuffer = state->renderer.pFramebuffers[state->window.swapchain.imageAcquiredIndex],
        .renderArea = renderArea,
        .clearValueCount = sizeof(clearValues) / sizeof(*clearValues),
        .pClearValues = clearValues,
    };

    // If secondary command buffers are used, use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Required because the viewport is dynamic (resizeable)
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)state->window.swapchain.imageExtent.width,
        .height = (float)state->window.swapchain.imageExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = state->window.swapchain.imageExtent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind the render pipeline to graphics (instead of compute)
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphicsPipeline);

    VkBuffer vertexBuffers[] = {state->renderer.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    // 16 limits verticies to 65535 (consider once making own models and having a check?)
    vkCmdBindIndexBuffer(cmd, state->renderer.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // No offset and 1 descriptor set bound for this frame
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pipelineLayout,
                            0, 1, &state->renderer.pDescriptorSets[state->renderer.currentFrame], 0, VK_NULL_HANDLE);

    // DRAW ! ! ! ! !
    // Not using instanced rendering so just 1 instance with nothing for the offset
    vkCmdDrawIndexed(cmd, state->renderer.modelIndexCount, 1, 0, 0, 0);

    // Must end the render pass if has begun (obviously)
    vkCmdEndRenderPass(cmd);

    // All errors generated from vkCmd functions will populate here. The vkCmd functions themselves are all void.
    logs_logIfError(vkEndCommandBuffer(cmd),
                    "Failed to end the command buffer for frame %d.", state->window.swapchain.imageAcquiredIndex)
}

void commandBufferSubmit(State_t *state)
{
    uint32_t frame = state->renderer.currentFrame;
    VkCommandBuffer cmd = state->renderer.pCommandBuffers[frame];

    VkPipelineStageFlags stageFlags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &state->renderer.imageAcquiredSemaphores[frame],
        .pWaitDstStageMask = stageFlags,
        .commandBufferCount = 1U,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1U,
        .pSignalSemaphores = &state->renderer.renderFinishedSemaphores[frame],
    };

    // Reset the fence *right before* submit (this is the only reset for this frame)
    logs_logIfError(vkResetFences(state->context.device, 1U, &state->renderer.inFlightFences[frame]),
                    "Failed to reset in-flight fence before submit (frame %u)", frame);

    VkResult r = vkQueueSubmit(state->context.graphicsQueue, 1U, &submitInfo, state->renderer.inFlightFences[frame]);

    logs_logIfError(r,
                    "Failed to submit graphicsQueue to the command buffer.");
}

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