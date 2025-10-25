#pragma once

#include <stdbool.h>
#include "core/types/state_t.h"
#include <vulkan/vulkan.h>

static const char *const s_pREQUIRED_LAYERS[] = {
    "VK_LAYER_KHRONOS_validation",
};
static const size_t s_REQUIRED_LAYERS_COUNT = (sizeof(s_pREQUIRED_LAYERS) / sizeof(s_pREQUIRED_LAYERS[0]));

static const char *s_pREQUIRED_EXTENSIONS[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};
static const size_t s_REQUIRED_EXTENSIONS_COUNT = sizeof(s_pREQUIRED_EXTENSIONS) / sizeof(s_pREQUIRED_EXTENSIONS[0]);

/// @brief Checks if the format has VK_FORMAT_D32_SFLOAT_S8_UINT or VK_FORMAT_D24_UNORM_S8_UINT
bool vulkan_format_hasStencilComponent(VkFormat format);

/// @brief Gets the first memory type (best) that matches the property flags for the state's physical device
uint32_t vulkan_device_physicalMemoryType_get(const State_t *pSTATE, uint32_t memoryRequirements, VkMemoryPropertyFlags propertyFlags);

/// @brief Logs the physical device's features and capabilities
void vulkan_deviceCapabilities_log(const VkPhysicalDeviceFeatures PHYSICAL_DEVICE_FEATURES, const VkSurfaceCapabilitiesKHR capabilities);

/// @brief Finds the first (best) format for the physical device that matches all criteria
VkFormat vulkan_instance_formatSupportedFind(State_t *state, VkFormat *candidates, size_t candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features);

/// @brief Creates the state's Vulkan instance and associated contexts
void vulkan_init(State_t *state);

/// @brief Destroys the state's Vulkan instance
void vulkan_instance_destroy(State_t *state);
