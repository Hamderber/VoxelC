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

// void atlasTextureImageCreate(State_t *state)
// {
//     int width, height, channels;
//     const char *imagePath = RESOURCE_TEXTURE_PATH TEXTURE_ATLAS;
//     // Flip the uv vertically to match face implementation
//     stbi_set_flip_vertically_on_load(true);

//     // Force the image to load with an alpha channel
//     stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
//     // 4 bytes per pixel (RGBA)
//     VkDeviceSize imageSize = width * height * 4;

//     logs_log(LOG_DEBUG, "Atlas PNG: %dx%d px, subtextureSize=%u px", width, height, state->config.subtextureSize);
//     state->renderer.atlasWidthInTiles = width / state->config.subtextureSize;
//     state->renderer.atlasHeightInTiles = height / state->config.subtextureSize;
//     state->renderer.atlasRegionCount = state->renderer.atlasWidthInTiles * state->renderer.atlasHeightInTiles;
//     logs_log(LOG_DEBUG, "The atlas texture has %u regions.", state->renderer.atlasRegionCount);

//     logs_logIfError(pixels == NULL,
//                     "Failed to load texture %s!", imagePath);

//     logs_log(LOG_DEBUG, "Loaded texture %s", imagePath);

//     VkBuffer stagingBuffer;
//     VkDeviceMemory stagingBufferMemory;

//     bufferCreate(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                  &stagingBuffer, &stagingBufferMemory);

//     // Map and copy the data into the staging buffer
//     void *data;
//     logs_logIfError(vkMapMemory(state->context.device, stagingBufferMemory, 0, imageSize, 0, &data),
//                     "Failed to map texture staging buffer memory.");
//     memcpy(data, pixels, (size_t)imageSize);
//     vkUnmapMemory(state->context.device, stagingBufferMemory);
//     // free the image array that was loaded
//     stbi_image_free(pixels);

//     imageCreate(state, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
//                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                 &state->renderer.atlasTextureImage, &state->renderer.atlasTextureImageMemory);

//     // Transition the image for copy
//     // Undefined because don't care about original contents of the image before the copy operation
//     imageLayoutTransition(state, state->renderer.atlasTextureImage, VK_IMAGE_LAYOUT_UNDEFINED,
//                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

//     bufferCopyToImage(state, stagingBuffer, state->renderer.atlasTextureImage, (uint32_t)width, (uint32_t)height);

//     // Transition the image for sampling
//     imageLayoutTransition(state, state->renderer.atlasTextureImage,
//                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//     vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
//     vkFreeMemory(state->context.device, stagingBufferMemory, state->context.pAllocator);
// }

// void atlasTextureImageCreate(State_t *state)
// {
//     int width, height, channels;
//     const char *imagePath = RESOURCE_TEXTURE_PATH TEXTURE_ATLAS;

//     // Keep consistent with your face-UV convention
//     stbi_set_flip_vertically_on_load(true);

//     // Force RGBA
//     stbi_uc *src = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
//     logs_logIfError(src == NULL, "Failed to load texture %s!", imagePath);
//     logs_log(LOG_DEBUG, "Loaded texture %s", imagePath);

//     const uint32_t tilePx = state->config.subtextureSize;
//     logs_logIfError((width % (int)tilePx) != 0 || (height % (int)tilePx) != 0,
//                     "Atlas dimensions (%dx%d) not divisible by subtextureSize (%u)", width, height, tilePx);

//     // Original grid dimensions (tiles)
//     const uint32_t tilesX = (uint32_t)width / tilePx;
//     const uint32_t tilesY = (uint32_t)height / tilePx;

//     // 1-pixel gutter around each tile -> 2px between tiles in the output
//     const uint32_t gutter = 1;
//     const uint32_t newTile = tilePx + 2 * gutter;
//     const uint32_t newWidth = tilesX * newTile;
//     const uint32_t newHeight = tilesY * newTile;

//     // Build padded atlas in CPU memory
//     const int C = 4; // RGBA8
//     stbi_uc *dst = (stbi_uc *)malloc((size_t)newWidth * newHeight * C);
//     if (!dst)
//     {
//         logs_log(LOG_ERROR, "Failed to allocate padded atlas buffer (%ux%u)", newWidth, newHeight);
//         stbi_image_free(src);
//         return;
//     }

// #define SRC(px, py) (&src[((py) * width + (px)) * C])
// #define DST(px, py) (&dst[((py) * newWidth + (px)) * C])

//     // For each tile, copy center and write gutters from duplicated edges
//     for (uint32_t ty = 0; ty < tilesY; ++ty)
//     {
//         for (uint32_t tx = 0; tx < tilesX; ++tx)
//         {
//             const uint32_t srcX = tx * tilePx;
//             const uint32_t srcY = ty * tilePx;

//             const uint32_t dstX = tx * newTile + gutter;
//             const uint32_t dstY = ty * newTile + gutter;

//             // Copy inner content
//             for (uint32_t y = 0; y < tilePx; ++y)
//             {
//                 memcpy(DST(dstX, dstY + y), SRC(srcX, srcY + y), (size_t)tilePx * C);
//             }

//             // Top & bottom gutters (duplicate first/last source rows)
//             for (uint32_t x = 0; x < tilePx; ++x)
//             {
//                 memcpy(DST(dstX + x, dstY - 1), SRC(srcX + x, srcY + 0), C);               // top
//                 memcpy(DST(dstX + x, dstY + tilePx), SRC(srcX + x, srcY + tilePx - 1), C); // bottom
//             }

//             // Left & right gutters (duplicate first/last source cols)
//             for (uint32_t y = 0; y < tilePx; ++y)
//             {
//                 memcpy(DST(dstX - 1, dstY + y), SRC(srcX + 0, srcY + y), C);               // left
//                 memcpy(DST(dstX + tilePx, dstY + y), SRC(srcX + tilePx - 1, srcY + y), C); // right
//             }

//             // Corners
//             memcpy(DST(dstX - 1, dstY - 1), SRC(srcX + 0, srcY + 0), C);
//             memcpy(DST(dstX + tilePx, dstY - 1), SRC(srcX + tilePx - 1, srcY + 0), C);
//             memcpy(DST(dstX - 1, dstY + tilePx), SRC(srcX + 0, srcY + tilePx - 1), C);
//             memcpy(DST(dstX + tilePx, dstY + tilePx), SRC(srcX + tilePx - 1, srcY + tilePx - 1), C);
//         }
//     }

//     stbi_image_free(src);

//     logs_log(LOG_DEBUG, "Atlas PNG: %dx%d px -> padded %ux%u px, tile=%u, gutter=%u",
//              width, height, newWidth, newHeight, tilePx, gutter);

//     // Update renderer bookkeeping for the tile grid
//     state->renderer.atlasWidthInTiles = tilesX;
//     state->renderer.atlasHeightInTiles = tilesY;
//     state->renderer.atlasRegionCount = tilesX * tilesY;

//     // Upload the padded atlas to GPU
//     VkDeviceSize imageSize = (VkDeviceSize)newWidth * newHeight * C;

//     VkBuffer stagingBuffer;
//     VkDeviceMemory stagingBufferMemory;
//     bufferCreate(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                  &stagingBuffer, &stagingBufferMemory);

//     void *mapped = NULL;
//     logs_logIfError(vkMapMemory(state->context.device, stagingBufferMemory, 0, imageSize, 0, &mapped),
//                     "Failed to map texture staging buffer memory.");
//     memcpy(mapped, dst, (size_t)imageSize);
//     vkUnmapMemory(state->context.device, stagingBufferMemory);
//     free(dst);

//     imageCreate(state, newWidth, newHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
//                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                 &state->renderer.atlasTextureImage, &state->renderer.atlasTextureImageMemory);

//     imageLayoutTransition(state, state->renderer.atlasTextureImage,
//                           VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

//     bufferCopyToImage(state, stagingBuffer, state->renderer.atlasTextureImage, newWidth, newHeight);

//     imageLayoutTransition(state, state->renderer.atlasTextureImage,
//                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

//     vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
//     vkFreeMemory(state->context.device, stagingBufferMemory, state->context.pAllocator);

//     // Also create the image view here (you can keep your separate function if you prefer)
//     state->renderer.atlasTextureImageView = imageViewCreate(
//         state, state->renderer.atlasTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
// }

void atlasTextureImageCreate(State_t *state)
{
    int width, height, channels;
    const char *imagePath = RESOURCE_TEXTURE_PATH TEXTURE_ATLAS;
    stbi_set_flip_vertically_on_load(true);

    stbi_uc *src = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    logs_logIfError(src == NULL, "Failed to load texture %s!", imagePath);

    const uint32_t tilePx = state->config.subtextureSize;
    const uint32_t gutterPx = state->config.atlasGutterPx; // <<< add this to your config (e.g., 4 or 8)

    logs_logIfError((width % (int)tilePx) != 0 || (height % (int)tilePx) != 0,
                    "Atlas dimensions (%dx%d) not divisible by subtextureSize (%u)", width, height, tilePx);

    const uint32_t tilesX = (uint32_t)width / tilePx;
    const uint32_t tilesY = (uint32_t)height / tilePx;
    const uint32_t stride = tilePx + 2 * gutterPx;
    const uint32_t newWidth = tilesX * stride;
    const uint32_t newHeight = tilesY * stride;

    const int C = 4;
    stbi_uc *dst = (stbi_uc *)calloc((size_t)newWidth * newHeight, C);
    logs_logIfError(dst == NULL, "Failed to allocate padded atlas buffer (%ux%u)", newWidth, newHeight);

#define SRC(px, py) (&src[((py) * width + (px)) * C])
#define DST(px, py) (&dst[((py) * newWidth + (px)) * C])

    for (uint32_t ty = 0; ty < tilesY; ++ty)
    {
        for (uint32_t tx = 0; tx < tilesX; ++tx)
        {
            const uint32_t sx = tx * tilePx;
            const uint32_t sy = ty * tilePx;
            const uint32_t dx = tx * stride + gutterPx;
            const uint32_t dy = ty * stride + gutterPx;

            // Copy inner tile
            for (uint32_t y = 0; y < tilePx; ++y)
                memcpy(DST(dx, dy + y), SRC(sx, sy + y), (size_t)tilePx * C);

            // Duplicate gutter rings of thickness = gutterPx
            // Horizontal rings (top & bottom), for each ring row r in [1..gutterPx]
            for (uint32_t r = 1; r <= gutterPx; ++r)
            {
                // Top ring row dy - r copies original first row (sy + 0)
                for (uint32_t x = 0; x < tilePx; ++x)
                    memcpy(DST(dx + x, dy - r), SRC(sx + x, sy + 0), C);

                // Bottom ring row dy + tilePx - 1 + r copies original last row (sy + tilePx - 1)
                for (uint32_t x = 0; x < tilePx; ++x)
                    memcpy(DST(dx + x, dy + tilePx - 1 + r), SRC(sx + x, sy + tilePx - 1), C);
            }

            // Vertical rings (left & right), for each ring column r in [1..gutterPx]
            for (uint32_t r = 1; r <= gutterPx; ++r)
            {
                // Left ring column dx - r copies original first col (sx + 0)
                for (uint32_t y = 0; y < tilePx; ++y)
                    memcpy(DST(dx - r, dy + y), SRC(sx + 0, sy + y), C);

                // Right ring column dx + tilePx - 1 + r copies original last col (sx + tilePx - 1)
                for (uint32_t y = 0; y < tilePx; ++y)
                    memcpy(DST(dx + tilePx - 1 + r, dy + y), SRC(sx + tilePx - 1, sy + y), C);
            }

            // Corners: fill all r×c corner blocks
            for (uint32_t r = 1; r <= gutterPx; ++r)
            {
                for (uint32_t c = 1; c <= gutterPx; ++c)
                {
                    // top-left corner
                    memcpy(DST(dx - c, dy - r), SRC(sx + 0, sy + 0), C);
                    // top-right corner
                    memcpy(DST(dx + tilePx - 1 + c, dy - r), SRC(sx + tilePx - 1, sy + 0), C);
                    // bottom-left corner
                    memcpy(DST(dx - c, dy + tilePx - 1 + r), SRC(sx + 0, sy + tilePx - 1), C);
                    // bottom-right corner
                    memcpy(DST(dx + tilePx - 1 + c, dy + tilePx - 1 + r), SRC(sx + tilePx - 1, sy + tilePx - 1), C);
                }
            }
        }
    }

    stbi_image_free(src);

    // Upload to GPU
    VkDeviceSize size = (VkDeviceSize)newWidth * newHeight * C;
    VkBuffer staging;
    VkDeviceMemory stagingMem;
    bufferCreate(state, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &staging, &stagingMem);
    void *mapped = NULL;
    logs_logIfError(vkMapMemory(state->context.device, stagingMem, 0, size, 0, &mapped),
                    "Failed to map texture staging buffer memory.");
    memcpy(mapped, dst, (size_t)size);
    vkUnmapMemory(state->context.device, stagingMem);
    free(dst);

    imageCreate(state, newWidth, newHeight, VK_FORMAT_R8G8B8A8_SRGB,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.atlasTextureImage, &state->renderer.atlasTextureImageMemory);

    imageLayoutTransition(state, state->renderer.atlasTextureImage,
                          VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    bufferCopyToImage(state, staging, state->renderer.atlasTextureImage, newWidth, newHeight);
    imageLayoutTransition(state, state->renderer.atlasTextureImage,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(state->context.device, staging, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingMem, state->context.pAllocator);

    // Store tile grid; regions will be built using padded size + same gutter
    state->renderer.atlasWidthInTiles = tilesX;
    state->renderer.atlasHeightInTiles = tilesY;
    state->renderer.atlasRegionCount = tilesX * tilesY;

    // Optional safety asserts:
    logs_log(LOG_DEBUG, "Padded atlas: %ux%u (tile=%u, gutter=%u, stride=%u, tiles=%ux%u)",
             newWidth, newHeight, tilePx, gutterPx, stride, tilesX, tilesY);
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

// AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles)
// {
//     logs_log(LOG_DEBUG, "Creating texture atlas regions...");

//     atlasDestroy(pAtlasRegions);

//     pAtlasRegions = malloc(sizeof(AtlasRegion_t) * atlasRegionCount);
//     logs_logIfError(pAtlasRegions == NULL,
//                     "Failed to allocate memory for the atlas texture regions!");

//     const float dU = 1.0f / (float)atlasWidthInTiles;
//     const float dV = 1.0f / (float)atlasHeightInTiles;

//     uint32_t regionIndex = 0U;
//     for (uint32_t y = 0; y < atlasHeightInTiles; y++)
//     {
//         for (uint32_t x = 0; x < atlasWidthInTiles; x++)
//         {
//             float u0 = (float)x * dU;
//             float v0 = (float)y * dV;
//             float u1 = u0 + dU;
//             float v1 = v0 + dV;

//             pAtlasRegions[regionIndex] = (AtlasRegion_t){
//                 .uvMin = {u0, v0},
//                 .uvMax = {u1, v1},
//             };

//             regionIndex++;
//         }
//     }

//     logs_log(LOG_DEBUG, "Generated %u atlas UV regions (%dx%d).",
//              atlasRegionCount, atlasWidthInTiles, atlasHeightInTiles);

//     logs_log(LOG_DEBUG, "tilesX=%u tilesY=%u  |  dU=%.5f dV=%.5f",
//              atlasWidthInTiles, atlasHeightInTiles,
//              dU,
//              dV);

//     AtlasRegion_t r = pAtlasRegions[0];
//     logs_log(LOG_DEBUG, "Region[0]: uvMin=(%.5f,%.5f) uvMax=(%.5f,%.5f) span=(%.5f,%.5f)",
//              r.uvMin.x, r.uvMin.y, r.uvMax.x, r.uvMax.y,
//              r.uvMax.x - r.uvMin.x, r.uvMax.y - r.uvMin.y);

//     return pAtlasRegions;
// }

// AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions,
//                            uint32_t atlasRegionCount,
//                            uint32_t atlasWidthInTiles,
//                            uint32_t atlasHeightInTiles,
//                            uint32_t atlasWidthPx,  // padded atlas width (px)
//                            uint32_t atlasHeightPx, // padded atlas height (px)
//                            uint32_t tilePx,        // inner tile size (px)
//                            uint32_t gutterPx)      // 1 px
// {
//     logs_log(LOG_DEBUG,
//              "Creating texture atlas regions... (tiles=%ux%u, atlasPx=%ux%u, tilePx=%u, gutter=%u)",
//              atlasWidthInTiles, atlasHeightInTiles, atlasWidthPx, atlasHeightPx, tilePx, gutterPx);

//     if (pAtlasRegions)
//     {
//         free(pAtlasRegions);
//         pAtlasRegions = NULL;
//     }

//     pAtlasRegions = malloc(sizeof(AtlasRegion_t) * atlasRegionCount);
//     logs_logIfError(!pAtlasRegions, "Failed to allocate atlas regions");

//     const float invW = 1.0f / (float)atlasWidthPx;
//     const float invH = 1.0f / (float)atlasHeightPx;

//     const uint32_t stride = tilePx + 2 * gutterPx; // distance from tile to tile (px)

//     uint32_t idx = 0;
//     for (uint32_t ty = 0; ty < atlasHeightInTiles; ++ty)
//     {
//         for (uint32_t tx = 0; tx < atlasWidthInTiles; ++tx)
//         {
//             // inner content rect IN PIXELS (edge-aligned, NOT center-aligned)
//             const uint32_t x0 = tx * stride + gutterPx;
//             const uint32_t y0 = ty * stride + gutterPx;
//             const uint32_t x1 = x0 + tilePx; // NOTE: right/top are *edge* coordinates
//             const uint32_t y1 = y0 + tilePx;

//             pAtlasRegions[idx].uvMin.x = (float)x0 * invW;
//             pAtlasRegions[idx].uvMin.y = (float)y0 * invH;
//             pAtlasRegions[idx].uvMax.x = (float)x1 * invW;
//             pAtlasRegions[idx].uvMax.y = (float)y1 * invH;
//             ++idx;
//         }
//     }

//     logs_log(LOG_DEBUG, "Generated %u atlas UV regions (edge-aligned, gutter-aware).", atlasRegionCount);
//     return pAtlasRegions;
// }

AtlasRegion_t *atlasCreate(AtlasRegion_t *regions,
                           uint32_t count,
                           uint32_t tilesX, uint32_t tilesY,
                           uint32_t atlasWpx, uint32_t atlasHpx,
                           uint32_t tilePx, uint32_t gutterPx)
{
    if (regions)
        free(regions);
    regions = (AtlasRegion_t *)malloc(sizeof(AtlasRegion_t) * count);
    logs_logIfError(!regions, "Failed to allocate atlas regions");

    const float invW = 1.0f / (float)atlasWpx;
    const float invH = 1.0f / (float)atlasHpx;
    const uint32_t stride = tilePx + 2 * gutterPx;

    uint32_t i = 0;
    for (uint32_t ty = 0; ty < tilesY; ++ty)
    {
        for (uint32_t tx = 0; tx < tilesX; ++tx)
        {
            const uint32_t x0 = tx * stride + gutterPx;
            const uint32_t y0 = ty * stride + gutterPx;
            const uint32_t x1 = x0 + tilePx; // edge-aligned inner rect
            const uint32_t y1 = y0 + tilePx;

            regions[i].uvMin.x = (float)x0 * invW;
            regions[i].uvMin.y = (float)y0 * invH;
            regions[i].uvMax.x = (float)x1 * invW;
            regions[i].uvMax.y = (float)y1 * invH;
            ++i;
        }
    }
    return regions;
}
