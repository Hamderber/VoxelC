#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>
#include "rendering/camera/camera_t.h"
#include "entity/entity_t.h"

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkQueue graphicsQueue;
    // This is always null right now so that Vulkan uses its own allocator
    VkAllocationCallbacks *pAllocator;
    /// @brief UINT32_MAX means no family assigned (set to max during creation)
    uint32_t queueFamily;
    Entity_t *pCameraEntity;
} Context_t;