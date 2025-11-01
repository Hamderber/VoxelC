#pragma region Includes
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include "core/logs.h"
#include "cmath/cmath.h"
#include "rendering/atlas_texture.h"
#include "rendering/image.h"
#include "core/types/context_t.h"
#include "core/types/renderer_t.h"
#include "core/types/state_t.h"
#include "stb_image.h"
#include "main.h"
#include "rendering/texture.h"
#include "rendering/buffers/buffers.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Image Create
/// @brief Gets a pointer to pixel (PX, PY) for the given image
static stbi_uc *pixel_ptr(stbi_uc *pImage, const int W, const int BPP, const int PX, const int PY)
{
    if (!pImage)
        return NULL;

    size_t indexBytes = ((PY * W) + PX) * BPP;
    return pImage + indexBytes;
}

/// @brief Create the padded atlas texture image
static void image_create(State_t *pState)
{
    const char *pIMAGE_PATH = RESOURCE_TEXTURE_PATH TEXTURE_ATLAS;
    const uint32_t TILE_PX = pState->config.subtextureSize;
    const uint32_t PAD_PX = pState->config.atlasPaddingPx;

    stbi_uc *pSRC = NULL;
    stbi_uc *pDST = NULL;

    int crashLine = 0;
    do
    {
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        pSRC = stbi_load(pIMAGE_PATH, &width, &height, &channels, STBI_rgb_alpha);
        if (!pSRC)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to load texture %s!", pIMAGE_PATH);
            break;
        }

        if ((width % (int)TILE_PX) != 0 || (height % (int)TILE_PX) != 0)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Atlas dimensions (%dx%d) not divisible by subtexture size (%" PRIu32 ")!", width, height, TILE_PX);
            break;
        }

        const uint32_t TILES_X = (uint32_t)width / TILE_PX;
        const uint32_t TILES_Y = (uint32_t)height / TILE_PX;
        const uint32_t STRIDE = TILE_PX + 2 * PAD_PX;
        const uint32_t WIDTH_PAD = TILES_X * STRIDE;
        const uint32_t HEIGHT_PAD = TILES_Y * STRIDE;
        // 4 bytes per pixel (RGBA)
        const int BPP = 4;

        pDST = calloc((size_t)WIDTH_PAD * HEIGHT_PAD, BPP);
        if (!pDST)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for padded atlas buffer (%ux%u)", WIDTH_PAD, HEIGHT_PAD);
            break;
        }

#pragma region Image Padding
        for (uint32_t ty = 0; ty < TILES_Y; ty++)
            for (uint32_t tx = 0; tx < TILES_X; tx++)
            {
                const uint32_t SX = tx * TILE_PX;
                const uint32_t SY = ty * TILE_PX;
                const uint32_t DX = tx * STRIDE + PAD_PX;
                const uint32_t DY = ty * STRIDE + PAD_PX;

                // Copy inner tile
                for (uint32_t y = 0; y < TILE_PX; y++)
                    memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX, DY + y),
                           pixel_ptr(pSRC, width, BPP, SX, SY + y), (size_t)TILE_PX * BPP);

                // Duplicate padding
                // Horizontal rings (top & bottom), for each ring row r in [1..padding px]
                for (uint32_t r = 1; r <= PAD_PX; r++)
                {
                    // Top ring row dy - r copies original first row (sy + 0)
                    for (uint32_t x = 0; x < TILE_PX; x++)
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX + x, DY - r),
                               pixel_ptr(pSRC, width, BPP, SX + x, SY), BPP);

                    // Bottom ring row dy + tile px - 1 + r copies original last row (sy + tile px - 1)
                    for (uint32_t x = 0; x < TILE_PX; x++)
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX + x, DY + TILE_PX - 1 + r),
                               pixel_ptr(pSRC, width, BPP, SX + x, SY + TILE_PX - 1), BPP);
                }

                // Vertical rings (left & right), for each ring column r in [1..padding px]
                for (uint32_t r = 1; r <= PAD_PX; r++)
                {
                    // Left ring column dx - r copies original first col (sx + 0)
                    for (uint32_t y = 0; y < TILE_PX; y++)
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX - r, DY + y),
                               pixel_ptr(pSRC, width, BPP, SX, SY + y), BPP);

                    // Right ring column dx + tile px - 1 + r copies original last col (sx + tile px - 1)
                    for (uint32_t y = 0; y < TILE_PX; y++)
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX + TILE_PX - 1 + r, DY + y),
                               pixel_ptr(pSRC, width, BPP, SX + TILE_PX - 1, SY + y), BPP);
                }

                // Corners fill all r×c corner blocks
                for (uint32_t r = 1; r <= PAD_PX; r++)
                    for (uint32_t c = 1; c <= PAD_PX; c++)
                    {
                        // top left corner
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX - c, DY - r),
                               pixel_ptr(pSRC, width, BPP, SX, SY), BPP);
                        // top right corner
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX + TILE_PX - 1 + c, DY - r),
                               pixel_ptr(pSRC, width, BPP, SX + TILE_PX - 1, SY), BPP);
                        // bottom left corner
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX - c, DY + TILE_PX - 1 + r),
                               pixel_ptr(pSRC, width, BPP, SX, SY + TILE_PX - 1), BPP);
                        // bottom right corner
                        memcpy(pixel_ptr(pDST, WIDTH_PAD, BPP, DX + TILE_PX - 1 + c, DY + TILE_PX - 1 + r),
                               pixel_ptr(pSRC, width, BPP, SX + TILE_PX - 1, SY + TILE_PX - 1), BPP);
                    }
            }
#pragma endregion

#pragma region Image Save
        // Upload to GPU
        VkDeviceSize size = (VkDeviceSize)WIDTH_PAD * HEIGHT_PAD * BPP;
        VkBuffer staging;
        VkDeviceMemory stagingMem;
        bufferCreate(pState, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &staging, &stagingMem);

        const uint32_t OFFSET = 0;
        const uint32_t FLAGS = 0;
        void *pMapped = NULL;
        if (vkMapMemory(pState->context.device, stagingMem, OFFSET, size, FLAGS, &pMapped) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to map atlas texture staging buffer memory!");
            break;
        }

        memcpy(pMapped, pDST, (size_t)size);
        vkUnmapMemory(pState->context.device, stagingMem);

        imageCreate(pState, WIDTH_PAD, HEIGHT_PAD, VK_FORMAT_R8G8B8A8_SRGB,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    &pState->renderer.atlasTextureImage, &pState->renderer.atlasTextureImageMemory);

        imageLayoutTransition(pState, pState->renderer.atlasTextureImage,
                              VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        bufferCopyToImage(pState, staging, pState->renderer.atlasTextureImage, WIDTH_PAD, HEIGHT_PAD);

        imageLayoutTransition(pState, pState->renderer.atlasTextureImage,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        vkDestroyBuffer(pState->context.device, staging, pState->context.pAllocator);
        vkFreeMemory(pState->context.device, stagingMem, pState->context.pAllocator);

        pState->renderer.atlasWidthInTiles = TILES_X;
        pState->renderer.atlasHeightInTiles = TILES_Y;
        pState->renderer.atlasRegionCount = TILES_X * TILES_Y;

        logs_log(LOG_DEBUG, "Padded atlas: %ux%u (tile=%u, padding=%u, stride=%u, tiles=%ux%u)",
                 WIDTH_PAD, HEIGHT_PAD, TILE_PX, PAD_PX, STRIDE, TILES_X, TILES_Y);
    } while (0);

    stbi_image_free(pSRC);
    free(pDST);

    if (crashLine != 0)
    {
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without an atlas texture image.");
    }
}
#pragma endregion
#pragma region Image Destroy
static void image_destroy(State_t *pState)
{
    vkDestroyImage(pState->context.device, pState->renderer.atlasTextureImage, pState->context.pAllocator);
    vkFreeMemory(pState->context.device, pState->renderer.atlasTextureImageMemory, pState->context.pAllocator);
}
#pragma endregion
#pragma region Image View
static inline void imageView_create(State_t *pState)
{
    pState->renderer.atlasTextureImageView = imageViewCreate(pState, pState->renderer.atlasTextureImage,
                                                             VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

static inline void imageView_destroy(Context_t *pContext, Renderer_t *pRenderer)
{
    vkDestroyImageView(pContext->device, pRenderer->atlasTextureImageView, pContext->pAllocator);
}
#pragma endregion
#pragma region Atlas
static inline void atlas_destroy(AtlasRegion_t *pAtlasRegions)
{
    free(pAtlasRegions);
    pAtlasRegions = NULL;
}

static void atlas_create(State_t *pState)
{
    do
    {
        // Build a black gutter around each tile of the atlas. Gutter prevents oversampling during aniosotropic filtering
        const uint32_t TILE_PX = pState->config.subtextureSize;
        const uint32_t PAD_PX = pState->config.atlasPaddingPx;
        const uint32_t TILES_X = pState->renderer.atlasWidthInTiles;
        const uint32_t TILES_Y = pState->renderer.atlasHeightInTiles;
        const uint32_t STRIDE = TILE_PX + 2 * PAD_PX;
        const float ATLAS_W_PX = (float)TILES_X * STRIDE;
        const float ATLAS_H_PX = (float)TILES_Y * STRIDE;
        const float INV_W = 1.0F / ATLAS_W_PX;
        const float INV_H = 1.0F / ATLAS_H_PX;

        if (pState->renderer.pAtlasRegions)
            free(pState->renderer.pAtlasRegions);

        pState->renderer.pAtlasRegions = (AtlasRegion_t *)malloc(sizeof(AtlasRegion_t) * pState->renderer.atlasRegionCount);

        if (!pState->renderer.pAtlasRegions)
        {
            logs_log(LOG_ERROR, "Failed to allocate memory for the texture atlas regions!");
            break;
        }

        uint32_t i = 0;
        for (uint32_t dY = 0; dY < TILES_Y; dY++)
            for (uint32_t dX = 0; dX < TILES_X; dX++)
            {
                const uint32_t x0 = dX * STRIDE + PAD_PX;
                const uint32_t y0 = dY * STRIDE + PAD_PX;
                // edge-aligned inner rect
                const uint32_t x1 = x0 + TILE_PX;
                const uint32_t y1 = y0 + TILE_PX;

                pState->renderer.pAtlasRegions[i].uvMin.x = (float)x0 * INV_W;
                pState->renderer.pAtlasRegions[i].uvMin.y = (float)y0 * INV_H;
                pState->renderer.pAtlasRegions[i].uvMax.x = (float)x1 * INV_W;
                pState->renderer.pAtlasRegions[i].uvMax.y = (float)y1 * INV_H;
                i++;
            }

        return;
    } while (0);

    free(pState->renderer.pAtlasRegions);
    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a texture atlas.");
}
#pragma endregion
#pragma region Create / Destroy
void atlasTexture_create(State_t *pState)
{
    // Atlas resources FIRST (image -> view -> sampler -> regions)
    image_create(pState);
    imageView_create(pState);
    tex_samplerCreate(pState);

    atlas_create(pState);
}

void atlasTexture_destroy(State_t *pState)
{
    tex_samplerDestroy(pState);
    imageView_destroy(&pState->context, &pState->renderer);
    image_destroy(pState);
    atlas_destroy(pState->renderer.pAtlasRegions);
}
#pragma endregion