#pragma once

#include "cmath/cmath.h"
#include "core/types/state_t.h"
#include "rendering/shaders.h"
#include "rendering/atlas_texture.h"
#include "rendering/texture.h"

static const Vec2f_t faceUVs[4] = {
    {0.0F, 1.0F}, // top-left
    {0.0F, 0.0F}, // bottom-left
    {1.0F, 1.0F}, // top-right
    {1.0F, 0.0F}, // bottom-right
};

static inline void uvs_voxel_assignFaceUVs(ShaderVertexVoxel_t *restrict pVerts, const size_t START,
                                           const AtlasRegion_t *restrict REGION, const TextureRotation_e ROTATION)
{
    Vec2f_t rotatedUVs[4];
    applyTextureRotation(rotatedUVs, faceUVs, ROTATION);

    for (int i = 0; i < 4; ++i)
    {
        pVerts[START + i].texCoord.x = cmath_clampF(REGION->uvMin.x + rotatedUVs[i].x * (REGION->uvMax.x - REGION->uvMin.x), 0.0F, 1.0F);
        pVerts[START + i].texCoord.y = cmath_clampF(REGION->uvMin.y + rotatedUVs[i].y * (REGION->uvMax.y - REGION->uvMin.y), 0.0F, 1.0F);
    }
}