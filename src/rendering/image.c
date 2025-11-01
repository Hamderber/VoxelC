
#include <stdint.h>
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "rendering/buffers/command_buffer.h"
#include "core/vk_instance.h"

VkImageView imageViewCreate(State_t *state, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{

    VkImageViewCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = aspectFlags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .components = state->config.swapchainComponentMapping,
    };

    VkImageView imageView;
    logs_logIfError(vkCreateImageView(state->context.device, &createInfo, state->context.pAllocator, &imageView),
                    "Failed to create image view!");

    return imageView;
}

void imageLayoutTransition(State_t *state, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = commandBuffer_singleTime_start(state);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        // Can use VK_IMAGE_LAYOUT_UNDEFINED if don't care about existing contents of the image
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        // Used if transferring queue family ownership
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        // Specifies the image that is affected and the specific part of the image. Currently just a non array/mipped image
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .srcAccessMask = 0, // TODO
        .dstAccessMask = 0, // TODO
    };

    VkPipelineStageFlags sourceStage = {0};
    VkPipelineStageFlags destinationStage = {0};

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        logs_logIfError(true,
                        "Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer,
                         // Which pipeline stage the operations occur that should happen before the barrier
                         sourceStage,
                         // What pipeline stage in which operations will wait on the barrier
                         destinationStage,
                         0,
                         0, VK_NULL_HANDLE,
                         0, VK_NULL_HANDLE,
                         1, &barrier);

    commandBuffer_singleTime_end(state, commandBuffer);
}

void imageCreate(State_t *state, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory)
{
    VkImageCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        // What type of coordinate system the texels will be accessed by
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        // These formats must match or the copy will fail
        .format = format,
        // Must use the same format for the texels as the pixels in the buffer or the copy will fail
        .tiling = tiling,
        // Not usable by the GPU and the very first transition will discard the texels. New copy so safe to discard.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // Will be used to copy (transfer) but also accessed by the shader (sampled)
        .usage = usage,
        // Only used by 1 queue family
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // Look into https://vulkan-tutorial.com/Texture_mapping/Images discussion on sparse images (voxel usage)
        .flags = 0,
    };

    logs_logIfError(vkCreateImage(state->context.device, &createInfo, state->context.pAllocator, image),
                    "Failed to create image!");

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(state->context.device, *image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = vulkan_device_physicalMemoryType_get(state, memoryRequirements.memoryTypeBits, properties),
    };

    logs_logIfError(vkAllocateMemory(state->context.device, &allocateInfo, state->context.pAllocator, imageMemory),
                    "Failed to allocate memory for texture images!");

    vkBindImageMemory(state->context.device, *image, *imageMemory, 0);
}
