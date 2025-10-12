#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "rendering/buffers/command_buffer.h"

void bufferCreate(State_t *state, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                  VkBuffer *buffer, VkDeviceMemory *bufferMemory);

void bufferCopy(State_t *state, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size);

void bufferCopyToImage(State_t *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
