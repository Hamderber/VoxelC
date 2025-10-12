#pragma once
#include "rendering/types/textureRotation_t.h"
#include "core/types/atlasFace_t.h"

typedef struct
{
    // which tile in the atlas
    AtlasFace_t atlasIndex;
    // rotation/flip to apply
    TextureRotation_t rotation;
} FaceTexture_t;