#pragma once

#include "cmath/cmath.h"
#include "rendering/types/renderChunk_t.h"

void chunkRendering_drawChunks(State_t *pState, VkCommandBuffer *pCmd, VkPipelineLayout *pPipelineLayout);

void chunk_placeRenderInWorld(RenderChunk_t *chunk, Vec3f_t *position);

/// @brief Destroys the chunk's render chunk (frees vulkan-related arrays/buffers)
void chunk_renderDestroy(State_t *state, RenderChunk_t *chunk);

/// @brief Creates the rendering chunk for the associated chunk. This is includes mesh creation and such
bool chunk_mesh_create(State_t *restrict pState, const Vec3u8_t *restrict pPOINTS, const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                       const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK, Chunk_t *restrict pChunk);