#pragma once

#include "cmath/cmath.h"
#include "rendering/types/renderChunk_t.h"

void chunk_drawChunks(State_t *pState, VkCommandBuffer *pCmd, VkPipelineLayout *pPipelineLayout);

void chunk_placeRenderInWorld(RenderChunk_t *chunk, Vec3f_t *position);

/// @brief Destroys the chunk's render chunk (frees vulkan-related arrays/buffers)
void chunk_renderDestroy(State_t *state, RenderChunk_t *chunk);