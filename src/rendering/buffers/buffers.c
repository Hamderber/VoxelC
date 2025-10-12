#include <vulkan/vulkan.h>
#include "core/vk_instance.h"
#include "rendering/buffers/command_buffer.h"

void bufferCreate(State_t *state, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                  VkBuffer *buffer, VkDeviceMemory *bufferMemory)
{
    VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = usageFlags,
        // Don't need to share this buffer between queue families right now
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    logs_logIfError(vkCreateBuffer(state->context.device, &createInfo, state->context.pAllocator, buffer),
                    "Failed to create buffer!")

        VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(state->context.device, *buffer, &memoryRequirements);

    uint32_t memoryType = vki_physicalMemoryTypeGet(state, memoryRequirements.memoryTypeBits, propertyFlags);

    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryType,
    };

    logs_logIfError(vkAllocateMemory(state->context.device, &allocateInfo, state->context.pAllocator, bufferMemory),
                    "Failed to allocate buffer memory!")

        // No offset required. If there was an offset, it would have to be divisible by memoryRequirements.alignment
        logs_logIfError(vkBindBufferMemory(state->context.device, *buffer, *bufferMemory, 0),
                        "Failed to bind buffer memory!")
}

void bufferCopy(State_t *state, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->renderer.commandPool,
        .commandBufferCount = 1U,
    };

    VkCommandBuffer commandBuffer = commandBufferSingleTimeBegin(state);

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    // Only copying one region
    vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);

    commandBufferSingleTimeEnd(state, commandBuffer);
}

void bufferCopyToImage(State_t *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = commandBufferSingleTimeBegin(state);

    VkBufferImageCopy region = {
        // Byte offset
        .bufferOffset = 0,
        // Length and height of how buffer is laid out in memory
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
    };

    // Assuming that the image has already been transitioned to a copy-optimal layout
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    commandBufferSingleTimeEnd(state, commandBuffer);
}
