#pragma once

#include <stdint.h>
#include "cmath/cmath.h"
#include "world/voxel/cubeFace_t.h"

typedef struct
{
    Vec3f_t pos;
    Vec3f_t color;
    Vec2f_t texCoord;
    uint32_t atlasIndex;
    int faceID;
} ShaderVertexVoxel_t;