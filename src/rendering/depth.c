#pragma once

#include <vulkan/vulkan.h>
#include "rendering/image.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"

/// @brief Gets the best supported depth format
/// @param state
/// @return VkFormat
VkFormat depth_formatGet(State_t *state)
{
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    return vki_formatSupportedFind(state, formats, sizeof(formats) / sizeof(*formats), VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void depthResourcesCreate(State_t *state)
{
    VkFormat depthFormat = depth_formatGet(state);

    imageCreate(state, state->window.swapchain.imageExtent.width, state->window.swapchain.imageExtent.height,
                depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.depthImage, &state->renderer.depthImageMemory);

    state->renderer.depthImageView = imageViewCreate(state, state->renderer.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    // Don't need to explicitly transition the layout of the image to a depth attachment because taken care of in the render pass
}

void depthResourcesDestroy(State_t *state)
{
    vkDestroyImageView(state->context.device, state->renderer.depthImageView, state->context.pAllocator);
    vkDestroyImage(state->context.device, state->renderer.depthImage, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.depthImageMemory, state->context.pAllocator);
}
