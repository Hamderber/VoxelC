#include <stdint.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/state.h"
#include "c_math/c_math.h"
#include "rendering/atlas_texture.h"

// void atlasRegionUVApply(State_t *state, uint32_t index, ShaderVertex_t *verts, size_t start, size_t count, AtlasRegion_t region)
// {
//     for (size_t i = 0; i < count; i++)
//     {
//         Vec2f_t uv = verts[start + i].texCoord;
//         verts[start + i].texCoord.x = region.uvMin.x + uv.x * (region.uvMax.x - region.uvMin.x);
//         verts[start + i].texCoord.y = region.uvMin.y + uv.y * (region.uvMax.y - region.uvMin.y);
//     }
// }

// void atlasDestroy(State_t *state)
// {
//     free(state->renderer.pAtlasRegions);
//     state->renderer.pAtlasRegions = NULL;
// }

// /*
//     The atlas is index bottom left to top right sequentially
//     3 4 5
//     0 1 2
// */
// void atlasCreate(State_t *state)
// {
//     logs_log(LOG_INFO, "Creating texture atlas regions...");

//     atlasDestroy(state);

//     state->renderer.pAtlasRegions = malloc(sizeof(AtlasRegion_t) * state->renderer.atlasRegionCount);
//     logs_logIfError(state->renderer.pAtlasRegions == NULL,
//                     "Failed to allocate memory for the atlas texture regions!")

//         const float dU = 1.0f / (float)state->renderer.atlasWidthInTiles;
//     const float dV = 1.0f / (float)state->renderer.atlasHeightInTiles;

//     uint32_t regionIndex = 0U;
//     for (uint32_t y = 0; y < state->renderer.atlasHeightInTiles; y++)
//     {
//         for (uint32_t x = 0; x < state->renderer.atlasWidthInTiles; x++)
//         {
//             float u0 = (float)x * dU;
//             float v0 = (float)y * dV;
//             float u1 = u0 + dU;
//             float v1 = v0 + dV;

//             state->renderer.pAtlasRegions[regionIndex] = (AtlasRegion_t){
//                 .uvMin = {u0, v0},
//                 .uvMax = {u1, v1},
//             };

//             regionIndex++;
//         }
//     }

//     logs_log(LOG_INFO, "Generated %u atlas UV regions (%dx%d).",
//              state->renderer.atlasRegionCount, state->renderer.atlasWidthInTiles, state->renderer.atlasHeightInTiles);

//     logs_log(LOG_INFO, "tilesX=%u tilesY=%u  |  dU=%.5f dV=%.5f",
//              state->renderer.atlasWidthInTiles, state->renderer.atlasHeightInTiles,
//              dU,
//              dV);

//     AtlasRegion_t r = state->renderer.pAtlasRegions[0];
//     logs_log(LOG_INFO, "Region[0]: uvMin=(%.5f,%.5f) uvMax=(%.5f,%.5f) span=(%.5f,%.5f)",
//              r.uvMin.x, r.uvMin.y, r.uvMax.x, r.uvMax.y,
//              r.uvMax.x - r.uvMin.x, r.uvMax.y - r.uvMin.y);
// }