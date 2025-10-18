#include <stdint.h>
#include <string.h>
#include "core/logs.h"
#include "cmath/cmath.h"
#include "rendering/atlas_texture.h"
#include "rendering/image.h"
#include "core/types/context_t.h"
#include "core/types/renderer_t.h"
#include "core/types/state_t.h"
#include "stb_image.h"
#include "main.h"
#include "rendering/buffers/buffers.h"

void atlasTextureImageCreate(State_t *state)
{
    int width, height, channels;
    const char *imagePath = RESOURCE_TEXTURE_PATH TEXTURE_ATLAS;
    // Flip the uv vertically to match face implementation
    stbi_set_flip_vertically_on_load(true);

    // Force the image to load with an alpha channel
    stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    // 4 bytes per pixel (RGBA)
    VkDeviceSize imageSize = width * height * 4;

    logs_log(LOG_DEBUG, "Atlas PNG: %dx%d px, subtextureSize=%u px", width, height, state->config.subtextureSize);
    state->renderer.atlasWidthInTiles = width / state->config.subtextureSize;
    state->renderer.atlasHeightInTiles = height / state->config.subtextureSize;
    state->renderer.atlasRegionCount = state->renderer.atlasWidthInTiles * state->renderer.atlasHeightInTiles;
    logs_log(LOG_DEBUG, "The atlas texture has %u regions.", state->renderer.atlasRegionCount);

    logs_logIfError(pixels == NULL,
                    "Failed to load texture %s!", imagePath);

    logs_log(LOG_DEBUG, "Loaded texture %s", imagePath);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    bufferCreate(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    // Map and copy the data into the staging buffer
    void *data;
    logs_logIfError(vkMapMemory(state->context.device, stagingBufferMemory, 0, imageSize, 0, &data),
                    "Failed to map texture staging buffer memory.");
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(state->context.device, stagingBufferMemory);
    // free the image array that was loaded
    stbi_image_free(pixels);

    imageCreate(state, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.atlasTextureImage, &state->renderer.atlasTextureImageMemory);

    // Transition the image for copy
    // Undefined because don't care about original contents of the image before the copy operation
    imageLayoutTransition(state, state->renderer.atlasTextureImage, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    bufferCopyToImage(state, stagingBuffer, state->renderer.atlasTextureImage, (uint32_t)width, (uint32_t)height);

    // Transition the image for sampling
    imageLayoutTransition(state, state->renderer.atlasTextureImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingBufferMemory, state->context.pAllocator);
}

void atlasTextureImageDestroy(State_t *state)
{
    vkDestroyImage(state->context.device, state->renderer.atlasTextureImage, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.atlasTextureImageMemory, state->context.pAllocator);
}

void atlasTextureViewImageCreate(State_t *state)
{
    // Written this way to support looping in the future
    state->renderer.atlasTextureImageView = imageViewCreate(state, state->renderer.atlasTextureImage,
                                                            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void atlasTextureImageViewDestroy(Context_t *context, Renderer_t *renderer)
{
    vkDestroyImageView(context->device, renderer->atlasTextureImageView, context->pAllocator);
}

void atlasDestroy(AtlasRegion_t *pAtlasRegions)
{
    free(pAtlasRegions);
    pAtlasRegions = NULL;
}

AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles)
{
    logs_log(LOG_DEBUG, "Creating texture atlas regions...");

    atlasDestroy(pAtlasRegions);

    pAtlasRegions = malloc(sizeof(AtlasRegion_t) * atlasRegionCount);
    logs_logIfError(pAtlasRegions == NULL,
                    "Failed to allocate memory for the atlas texture regions!");

    const float dU = 1.0f / (float)atlasWidthInTiles;
    const float dV = 1.0f / (float)atlasHeightInTiles;

    uint32_t regionIndex = 0U;
    for (uint32_t y = 0; y < atlasHeightInTiles; y++)
    {
        for (uint32_t x = 0; x < atlasWidthInTiles; x++)
        {
            float u0 = (float)x * dU;
            float v0 = (float)y * dV;
            float u1 = u0 + dU;
            float v1 = v0 + dV;

            pAtlasRegions[regionIndex] = (AtlasRegion_t){
                .uvMin = {u0, v0},
                .uvMax = {u1, v1},
            };

            regionIndex++;
        }
    }

    logs_log(LOG_DEBUG, "Generated %u atlas UV regions (%dx%d).",
             atlasRegionCount, atlasWidthInTiles, atlasHeightInTiles);

    logs_log(LOG_DEBUG, "tilesX=%u tilesY=%u  |  dU=%.5f dV=%.5f",
             atlasWidthInTiles, atlasHeightInTiles,
             dU,
             dV);

    AtlasRegion_t r = pAtlasRegions[0];
    logs_log(LOG_DEBUG, "Region[0]: uvMin=(%.5f,%.5f) uvMax=(%.5f,%.5f) span=(%.5f,%.5f)",
             r.uvMin.x, r.uvMin.y, r.uvMax.x, r.uvMax.y,
             r.uvMax.x - r.uvMin.x, r.uvMax.y - r.uvMin.y);

    return pAtlasRegions;
}
