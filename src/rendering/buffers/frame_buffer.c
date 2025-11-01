#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include <vulkan/vulkan.h>
#include "rendering/buffers/buffers.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Create
void framebuffers_create(State_t *pState)
{
    int crashLine = 0;
    do
    {
        pState->renderer.framebufferCount = pState->window.swapchain.imageCount;
        pState->renderer.pFramebuffers = malloc(sizeof(VkFramebuffer) * pState->renderer.framebufferCount);

        if (!pState->renderer.pFramebuffers)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for frame buffers!");
            break;
        }

        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            // Only one image layer for the current swapchain configuration
            .layers = 1,
            .renderPass = pState->renderer.pRenderPass,
            .width = pState->window.swapchain.imageExtent.width,
            .height = pState->window.swapchain.imageExtent.height,
            // The attachment count must be the same as the amount of attachment descriptions in the renderpass
            .attachmentCount = pState->renderer.renderpassAttachmentCount,
        };

        for (uint32_t i = 0; i < pState->renderer.framebufferCount; i++)
        {
            // Building the image view here by just attaching the depth image view to the original swapchain image view
            const VkImageView ATTACHMENTS[] = {
                pState->window.swapchain.pImageViews[i],
                pState->renderer.depthImageView,
            };

            createInfo.pAttachments = ATTACHMENTS;

            if (vkCreateFramebuffer(pState->context.device, &createInfo, pState->context.pAllocator,
                                    &pState->renderer.pFramebuffers[i]) != VK_SUCCESS)
            {
                crashLine = __LINE__;
                logs_log(LOG_ERROR, "Failed to create frame buffer %" PRIu32 "!", i);
                break;
            }
        }
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without frame buffers!");
}
#pragma endregion
#pragma region Destroy
void framebuffers_destroy(State_t *pState)
{
    for (uint32_t i = 0; i < pState->renderer.framebufferCount; i++)
        vkDestroyFramebuffer(pState->context.device, pState->renderer.pFramebuffers[i], pState->context.pAllocator);

    free(pState->renderer.pFramebuffers);

    pState->renderer.pFramebuffers = NULL;
    pState->renderer.framebufferCount = 0;
}
#pragma endregion