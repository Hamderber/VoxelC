#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/types/state_t.h"

/// @brief Checks if the config's AF is available in the device options. Returns 0 if not.
/// @param state
/// @return float
static float tex_AFGet(State_t *state)
{
    for (size_t i = 0; i < state->renderer.anisotropicFilteringOptionsCount; i++)
    {
        if ((int)state->config.anisotropy == (int)state->renderer.anisotropicFilteringOptions[i])
        {
            logs_log(LOG_DEBUG, "Anisotropic filtering is set to %d x", (int)state->config.anisotropy);
            return (float)state->renderer.anisotropicFilteringOptions[i];
        }
    }

    logs_log(LOG_WARN, "Configured anisotropic filtering (%d) is not supported. Anisotropic filtering will be set to minimum (1 x)!",
             (int)state->config.anisotropy);
    return 1.0F;
}

void tex_samplerCreate(State_t *state)
{
    VkSamplerCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        // Filters specify how to interpolate texels that are magnified or minified
        // Mag = oversampling and Min = undersampling
        // See https://vulkan-tutorial.com/Texture_mapping/Image_view_and_sampler
        // Linear for larger textures (interpolation)
        // .magFilter = VK_FILTER_LINEAR,
        // .minFilter = VK_FILTER_LINEAR,
        // Nearest for small textures (pixel art)
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        // Addressing can be defined per-axis and for some reason its UVW instead of XYZ (texture space convention)
        // Repeat is the most commonly used (floors, etc.) for tiling
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = tex_AFGet(state),
        // Which color is returned when sampling beyond the image with clamp to border addressing mode. It is possible to
        // return black, white or transparent in either float or int formats
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        // Which coordinate system you want to use to address texels in an image. If this field is VK_TRUE, then you can simply
        // use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed
        // using the [0, 1) range on all axes. Real-world applications almost always use normalized coordinates, because then
        // it's possible to use textures of varying resolutions with the exact same coordinates
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        // Mipmapping not implemneted at this time
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    logs_logIfError(vkCreateSampler(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.textureSampler),
                    "Failed to create texture sampler!");
}

void tex_samplerDestroy(State_t *state)
{
    vkDestroySampler(state->context.device, state->renderer.textureSampler, state->context.pAllocator);
}
