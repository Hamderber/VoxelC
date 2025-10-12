#pragma once

#include <string.h>
#include "rendering/atlas_texture.h"
#include "c_math/c_math.h"

typedef enum
{
    // default
    TEX_ROT_0 = 0,
    // clockwise
    TEX_ROT_90 = 1,
    TEX_ROT_180 = 2,
    TEX_ROT_270 = 3,
    TEX_FLIP_X = 4,
    TEX_FLIP_Y = 5,
} TextureRotation_t;

typedef struct
{
    // which tile in the atlas
    AtlasFace_t atlasIndex;
    // rotation/flip to apply
    TextureRotation_t rotation;
} FaceTexture_t;

static inline void applyTextureRotation(Vec2f_t outUVs[4], const Vec2f_t inUVs[4], TextureRotation_t rotation)
{
    switch (rotation)
    {
    case TEX_ROT_0:
        // copy as-is
        memcpy(outUVs, inUVs, sizeof(Vec2f_t) * 4);
        break;

    case TEX_ROT_90:
        outUVs[0] = inUVs[1]; // top-left <- bottom-left
        outUVs[1] = inUVs[3]; // bottom-left <- bottom-right
        outUVs[2] = inUVs[0]; // top-right <- top-left
        outUVs[3] = inUVs[2]; // bottom-right <- top-right
        break;

    case TEX_ROT_180:
        outUVs[0] = inUVs[3];
        outUVs[1] = inUVs[2];
        outUVs[2] = inUVs[1];
        outUVs[3] = inUVs[0];
        break;

    case TEX_ROT_270:
        outUVs[0] = inUVs[2];
        outUVs[1] = inUVs[0];
        outUVs[2] = inUVs[3];
        outUVs[3] = inUVs[1];
        break;

    case TEX_FLIP_X:
        outUVs[0] = inUVs[2]; // swap left/right
        outUVs[1] = inUVs[3];
        outUVs[2] = inUVs[0];
        outUVs[3] = inUVs[1];
        break;

    case TEX_FLIP_Y:
        outUVs[0] = inUVs[1]; // swap top/bottom
        outUVs[1] = inUVs[0];
        outUVs[2] = inUVs[3];
        outUVs[3] = inUVs[2];
        break;
    }
}
