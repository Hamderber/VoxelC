#pragma once

#include "core/core.h"
#include "rendering/depth.h"

/// @brief The render pass is basically the blueprint for the graphics operation in the graphics pipeline
/// @param state
static void rp_create(State_t *state)
{
    VkAttachmentReference colorAttachmentReference = {
        // This 0 is the output location of the outColor vec4 in the fragment shader. If other outputs are needed, the attachment
        // number would be the same as the output location
        .attachment = 0U,
        // Render target for color output
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentReference = {
        // Depth
        .attachment = 1U,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpassDescriptions[] = {
        (VkSubpassDescription){
            // Use for graphics pipeline instead of a compute pipeline
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentReference,
            .pDepthStencilAttachment = &depthAttachmentReference,
        },
    };

    VkAttachmentDescription attachmentDescriptions[] = {
        // Present
        {
            .format = state->window.swapchain.format,
            // No MSAA at this time
            .samples = VK_SAMPLE_COUNT_1_BIT,
            // We don't know what the original layout will be
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            // Tells Vulkan to transition the image layout to presentation source for presenting to the screen
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            // The load operation will be clear (clear image initially)
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            // We want to store the results of this render
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        },
        // Depth
        {
            .format = depth_formatGet(state),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            // The depth data won't be stored because its not used after drawing
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };

    state->renderer.renderpassAttachmentCount = sizeof(attachmentDescriptions) / sizeof(*attachmentDescriptions);

    VkSubpassDependency dependencies[] = {
        (VkSubpassDependency){
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            // Destination is the first subpass
            .dstSubpass = 0U,
            // Wait in the pipeline for the previous external operations to finish before color attachment output
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        },
    };

    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = sizeof(subpassDescriptions) / sizeof(*subpassDescriptions),
        .pSubpasses = subpassDescriptions,
        .attachmentCount = state->renderer.renderpassAttachmentCount,
        .pAttachments = attachmentDescriptions,
        .dependencyCount = sizeof(dependencies) / sizeof(*dependencies),
        .pDependencies = dependencies,
    };

    logs_logIfError(vkCreateRenderPass(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.pRenderPass),
                    "Failed to create the render pass.");
};

/// @brief Destroys the render pass
/// @param state
static void rp_destroy(State_t *state)
{
    vkDestroyRenderPass(state->context.device, state->renderer.pRenderPass, state->context.pAllocator);

    state->renderer.renderpassAttachmentCount = 0U;
}