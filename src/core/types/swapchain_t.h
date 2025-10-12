#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

typedef struct
{
    // Swapchain
    // https://www.youtube.com/watch?v=nSzQcyQTtRY
    VkSwapchainKHR handle;
    uint32_t imageCount;
    uint32_t imageAcquiredIndex;
    bool recreate;
    VkImage *pImages;
    VkImageView *pImageViews;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkExtent2D imageExtent;
} Swapchain_t;