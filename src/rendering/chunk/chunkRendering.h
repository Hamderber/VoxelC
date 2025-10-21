#pragma once

#include "cmath/cmath.h"
#include "rendering/types/renderChunk_t.h"

void chunk_placeRenderInWorld(RenderChunk_t *chunk, Vec3f_t *position);

void chunk_renderDestroy(State_t *state, RenderChunk_t *chunk);