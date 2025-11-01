#pragma once

#include <stdint.h>
#include "core/types/state_t.h"
#include "rendering/types/shaderVertex_t.h"

void vertexBuffer_createFromData(State_t *state, ShaderVertex_t *vertices, uint32_t vertexCount);

void vertexBuffer_destroy(State_t *state);