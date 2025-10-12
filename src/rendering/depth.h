#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"

/// @brief Gets the best supported depth format
/// @param state
/// @return VkFormat
static VkFormat depth_formatGet(State_t *state)
{
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    return vki_formatSupportedFind(state, formats, sizeof(formats) / sizeof(*formats), VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}