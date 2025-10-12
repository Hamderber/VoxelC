#include <stdint.h>
#include <string.h>
#include "core/logs.h"
#include "c_math/c_math.h"
#include "rendering/atlas_texture.h"
#include "rendering/image.h"

// void atlasTextureViewImageCreate(State_t *state)
// {
//     // Written this way to support looping in the future
//     state->renderer.atlasTextureImageView = imageViewCreate(state, state->renderer.atlasTextureImage,
//                                                             VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
// }

// void atlasTextureImageViewDestroy(State_t *state)
// {
//     vkDestroyImageView(state->context.device, state->renderer.atlasTextureImageView, state->context.pAllocator);
// }

void atlasDestroy(AtlasRegion_t *pAtlasRegions)
{
    free(pAtlasRegions);
    pAtlasRegions = NULL;
}

AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles)
{
    logs_log(LOG_INFO, "Creating texture atlas regions...");

    atlasDestroy(pAtlasRegions);

    pAtlasRegions = malloc(sizeof(AtlasRegion_t) * atlasRegionCount);
    logs_logIfError(pAtlasRegions == NULL,
                    "Failed to allocate memory for the atlas texture regions!")

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

    logs_log(LOG_INFO, "Generated %u atlas UV regions (%dx%d).",
             atlasRegionCount, atlasWidthInTiles, atlasHeightInTiles);

    logs_log(LOG_INFO, "tilesX=%u tilesY=%u  |  dU=%.5f dV=%.5f",
             atlasWidthInTiles, atlasHeightInTiles,
             dU,
             dV);

    AtlasRegion_t r = pAtlasRegions[0];
    logs_log(LOG_INFO, "Region[0]: uvMin=(%.5f,%.5f) uvMax=(%.5f,%.5f) span=(%.5f,%.5f)",
             r.uvMin.x, r.uvMin.y, r.uvMax.x, r.uvMax.y,
             r.uvMax.x - r.uvMin.x, r.uvMax.y - r.uvMin.y);

    return pAtlasRegions;
}
