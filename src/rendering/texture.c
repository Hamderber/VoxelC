#include <vulkan/vulkan.h>
#include <string.h>
#include <stdlib.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "stb_image.h"
#include "image.h"
#include "buffers/buffers.h"

/// @brief Checks if the config's AF is available in the device options. Returns 0 if not.
static float tex_AFGet(State_t *pState)
{
    for (size_t i = 0; i < pState->renderer.anisotropicFilteringOptionsCount; i++)
        if ((int)pState->config.anisotropy == (int)pState->renderer.pAnisotropicFilteringOptions[i])
        {
            logs_log(LOG_DEBUG, "Anisotropic filtering is set to %d x", (int)pState->config.anisotropy);
            return (float)pState->renderer.pAnisotropicFilteringOptions[i];
        }

    logs_log(LOG_WARN, "Configured anisotropic filtering (%d) is not supported. Anisotropic filtering will be set to minimum (1x)!",
             (int)pState->config.anisotropy);

    return 1.0F;
}

void tex_samplerCreate(State_t *pState)
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
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = tex_AFGet(pState),
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
        .mipLodBias = 0.0F,
        .minLod = 0.0F,
        .maxLod = 0.0F,
    };

    logs_logIfError(vkCreateSampler(pState->context.device, &createInfo, pState->context.pAllocator, &pState->renderer.textureSampler),
                    "Failed to create texture sampler!");
}

void tex_samplerDestroy(State_t *state)
{
    vkDestroySampler(state->context.device, state->renderer.textureSampler, state->context.pAllocator);
}

// Create a single 2D texture (no atlas, no mipmaps). sRGB format.
// Returns true on success. Out params are valid on success.
bool texture2DCreateFromFile(State_t *state,
                             const char *imagePath,
                             VkImage *outImage,
                             VkDeviceMemory *outMemory,
                             VkImageView *outView,
                             VkSampler *outSampler)
{
    int width = 0, height = 0, channels = 0;

    stbi_set_flip_vertically_on_load(true);

    // Force 4 channels (RGBA) for a predictable VkFormat
    stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels)
    {
        logs_log(LOG_ERROR, "Failed to load model texture: %s", imagePath);
        return false;
    }

    const VkDeviceSize imageSize = (VkDeviceSize)width * (VkDeviceSize)height * 4;
    logs_log(LOG_DEBUG, "Model texture: %s (%dx%d, RGBA8, %zu bytes)",
             imagePath, width, height, (size_t)imageSize);

    // --- Staging buffer ---
    VkBuffer stagingBuffer = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
    bufferCreate(state, imageSize,
                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingMemory);

    void *mapped = NULL;
    if (vkMapMemory(state->context.device, stagingMemory, 0, imageSize, 0, &mapped) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to map model texture staging memory.");
        stbi_image_free(pixels);
        vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
        vkFreeMemory(state->context.device, stagingMemory, state->context.pAllocator);
        return false;
    }
    memcpy(mapped, pixels, (size_t)imageSize);
    vkUnmapMemory(state->context.device, stagingMemory);
    stbi_image_free(pixels);

    // --- GPU image ---
    imageCreate(state, width, height,
                VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                outImage, outMemory);

    // Layout: UNDEFINED -> TRANSFER_DST_OPTIMAL
    imageLayoutTransition(state, *outImage,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy staging -> image
    bufferCopyToImage(state, stagingBuffer, *outImage, (uint32_t)width, (uint32_t)height);

    // Layout: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    imageLayoutTransition(state, *outImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Free staging
    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingMemory, state->context.pAllocator);

    // --- Image view ---
    *outView = imageViewCreate(state, *outImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
    if (*outView == VK_NULL_HANDLE)
    {
        logs_log(LOG_ERROR, "Failed to create image view for model texture.");
        vkDestroyImage(state->context.device, *outImage, state->context.pAllocator);
        vkFreeMemory(state->context.device, *outMemory, state->context.pAllocator);
        *outImage = VK_NULL_HANDLE;
        *outMemory = VK_NULL_HANDLE;
        return false;
    }

    VkSamplerCreateInfo sci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST, // sharper pixels
        .minFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    if (vkCreateSampler(state->context.device, &sci, state->context.pAllocator, outSampler) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to create sampler for model texture.");
        vkDestroyImageView(state->context.device, *outView, state->context.pAllocator);
        vkDestroyImage(state->context.device, *outImage, state->context.pAllocator);
        vkFreeMemory(state->context.device, *outMemory, state->context.pAllocator);
        *outView = VK_NULL_HANDLE;
        *outImage = VK_NULL_HANDLE;
        *outMemory = VK_NULL_HANDLE;
        return false;
    }

    logs_log(LOG_DEBUG, "Created model texture: %s", imagePath);
    return true;
}
