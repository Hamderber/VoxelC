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

static inline void assignFaceUVs(ShaderVertex_t *verts, size_t start, const AtlasRegion_t *region, TextureRotation_t rotation)
{
    Vec2f_t rotatedUVs[4];
    applyTextureRotation(rotatedUVs, faceUVs, rotation);

    for (int i = 0; i < 4; ++i)
    {
        // verts[start + i].texCoord.x = region->uvMin.x + rotatedUVs[i].x * (region->uvMax.x - region->uvMin.x);
        // verts[start + i].texCoord.y = region->uvMin.y + rotatedUVs[i].y * (region->uvMax.y - region->uvMin.y);

        verts[start + i].texCoord.x = cmath_clampF(region->uvMin.x + rotatedUVs[i].x * (region->uvMax.x - region->uvMin.x), 0.0F, 1.0F);
        verts[start + i].texCoord.y = cmath_clampF(region->uvMin.y + rotatedUVs[i].y * (region->uvMax.y - region->uvMin.y), 0.0F, 1.0F);
    }
}