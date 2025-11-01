#pragma once

#include <stdint.h>
#include "core/types/state_t.h"
#include "rendering/types/shaderVertexModel_t.h"
#include "rendering/types/shaderVertexVoxel_t.h"

void vertexBuffer_createFromData_Model(State_t *state, ShaderVertexModel_t *vertices, uint32_t vertexCount);
void vertexBuffer_createFromData_Voxel(State_t *pState, ShaderVertexVoxel_t *pVertices, uint32_t vertexCount);

void vertexBuffer_destroy(State_t *state);