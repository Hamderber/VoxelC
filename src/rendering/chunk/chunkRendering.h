#pragma once

#include "cmath/cmath.h"
#include "rendering/types/renderChunk_t.h"

void chunk_placeRenderInWorld(RenderChunk_t *chunk, Vec3f_t *position)
{
    chunk->modelMatrix = cmath_mat_rotate(MAT4_IDENTITY, 0.0F, VEC3_Y_AXIS);
    chunk->modelMatrix = cmath_mat_setTranslation(MAT4_IDENTITY, *position);
}