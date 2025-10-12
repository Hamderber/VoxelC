#pragma once

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/logs.h"
#include "core/state.h"
#include <vulkan/vulkan.h>
#include "rendering/buffers/buffers.h"

void framebuffersCreate(State_t *state)
{

    state->renderer.framebufferCount = state->window.swapchain.imageCount;
    state->renderer.pFramebuffers = malloc(sizeof(VkFramebuffer) * state->renderer.framebufferCount);

    logs_logIfError(!state->renderer.pFramebuffers,
                    "Failed to allocate memory for the framebuffers.")

        for (uint32_t framebufferIndex = 0U; framebufferIndex < state->renderer.framebufferCount; framebufferIndex++)
    {
        // Building the image view here by just attaching the depth image view to the original swapchain image view
        VkImageView attachments[2] = {
            state->window.swapchain.pImageViews[framebufferIndex],
            state->renderer.depthImageView,
        };

        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            // Only one image layer for the current swapchain configuration
            .layers = 1U,
            .renderPass = state->renderer.pRenderPass,
            .width = state->window.swapchain.imageExtent.width,
            .height = state->window.swapchain.imageExtent.height,
            // The attachment count must be the same as the amount of attachment descriptions in the renderpass
            .attachmentCount = state->renderer.renderpassAttachmentCount,
            .pAttachments = attachments,
        };

        logs_logIfError(vkCreateFramebuffer(state->context.device, &createInfo, state->context.pAllocator,
                                            &state->renderer.pFramebuffers[framebufferIndex]),
                        "Failed to create frame buffer %d.", framebufferIndex)
    }
}

void framebuffersDestroy(State_t *state)
{
    // There is a hypothtical situation where the cause of detroying framebuffers is due to changing the amount of swapchain
    // images. In such a case, directly relying on state->window.swapchain.imageCount would be insufficient and cause a memory leak.
    for (uint32_t i = 0; i < state->renderer.framebufferCount; i++)
    {
        vkDestroyFramebuffer(state->context.device, state->renderer.pFramebuffers[i], state->context.pAllocator);
    }

    free(state->renderer.pFramebuffers);

    state->renderer.pFramebuffers = NULL;
    state->renderer.framebufferCount = 0U;
}
