#include <vulkan/vulkan.h>
#include "core/logs.h"
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
                    "Failed to create buffer!");

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(state->context.device, *buffer, &memoryRequirements);

    uint32_t memoryType = vulkan_device_physicalMemoryType_get(state, memoryRequirements.memoryTypeBits, propertyFlags);

    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryType,
    };

    logs_logIfError(vkAllocateMemory(state->context.device, &allocateInfo, state->context.pAllocator, bufferMemory),
                    "Failed to allocate buffer memory!");

    // No offset required. If there was an offset, it would have to be divisible by memoryRequirements.alignment
    logs_logIfError(vkBindBufferMemory(state->context.device, *buffer, *bufferMemory, 0),
                    "Failed to bind buffer memory!");
}

void bufferCopy(State_t *state, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = commandBuffer_singleTime_start(state);

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    // Only copying one region
    vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);

    commandBuffer_singleTime_end(state, commandBuffer);
}

void bufferCopyToImage(State_t *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = commandBuffer_singleTime_start(state);

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

    commandBuffer_singleTime_end(state, commandBuffer);
}
