#pragma once

#include <stdint.h>
#include "cmath/cmath.h"

typedef struct
{
    Vec3f_t pos;
    Vec3f_t color;
    Vec2f_t texCoord;
    uint32_t atlasIndex;
} ShaderVertexModel_t;