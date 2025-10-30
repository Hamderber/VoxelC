#include <vulkan/vulkan.h>
#include "rendering/image.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"

VkFormat depthFormat_get(State_t *pState)
{
    const VkFormat pFORMATS[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    return vulkan_instance_formatSupportedFind(pState, pFORMATS, sizeof(pFORMATS) / sizeof(*pFORMATS), VK_IMAGE_TILING_OPTIMAL,
                                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void depthResources_create(State_t *pState)
{
    VkFormat depthFormat = depthFormat_get(pState);

    imageCreate(pState, pState->window.swapchain.imageExtent.width, pState->window.swapchain.imageExtent.height,
                depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &pState->renderer.depthImage, &pState->renderer.depthImageMemory);

    pState->renderer.depthImageView = imageViewCreate(pState, pState->renderer.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    // Don't need to explicitly transition the layout of the image to a depth attachment because taken care of in the render pass
}

void depthResources_destroy(State_t *pState)
{
    vkDestroyImageView(pState->context.device, pState->renderer.depthImageView, pState->context.pAllocator);
    vkDestroyImage(pState->context.device, pState->renderer.depthImage, pState->context.pAllocator);
    vkFreeMemory(pState->context.device, pState->renderer.depthImageMemory, pState->context.pAllocator);
}
