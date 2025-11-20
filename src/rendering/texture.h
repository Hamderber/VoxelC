#pragma once

#include <string.h>
#include <vulkan/vulkan.h>
#include "cmath/cmath.h"
#include "rendering/types/textureRotation_t.h"
#include "core/types/state_t.h"

static inline void applyTextureRotation(Vec2f_t outUVs[4], const Vec2f_t inUVs[4], TextureRotation_e rotation)
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

void tex_samplerCreate(State_t *state);

void tex_samplerDestroy(State_t *state);

bool texture2DCreateFromFile(State_t *state,
                             const char *imagePath,
                             VkImage *outImage,
                             VkDeviceMemory *outMemory,
                             VkImageView *outView,
                             VkSampler *outSampler);