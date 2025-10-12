#pragma once

#include <stdint.h>
#include "c_math/c_math.h"

typedef struct
{
    Vec3f_t pos;
    Vec3f_t color;
    Vec2f_t texCoord;
    uint32_t atlasIndex;
} ShaderVertex_t;