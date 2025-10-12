#pragma once

#include "core/types/state_t.h"
#include <vulkan/vulkan.h>

static const char *pVALIDATION_LAYERS[] = {"VK_LAYER_KHRONOS_validation"};
static const uint32_t VALIDATION_LAYER_COUNT = 1;

void vki_logCapabilities(VkPhysicalDeviceFeatures physicalDeviceFeatures, const VkSurfaceCapabilitiesKHR capabilities);

uint32_t vki_physicalMemoryTypeGet(State_t *state, uint32_t memoryRequirements, VkMemoryPropertyFlags propertyFlags);

VkFormat vki_formatSupportedFind(State_t *state, VkFormat *candidates, size_t candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features);

void vki_logAPI(void);

void vki_create(State_t *state);

void vki_destroy(State_t *state);
