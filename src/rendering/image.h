#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>
#include "core/types/state_t.h"

VkImageView imageViewCreate(State_t *state, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

void imageLayoutTransition(State_t *state, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

void imageCreate(State_t *state, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory);
