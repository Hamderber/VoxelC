#pragma region Includes
#pragma once
#include "api/chunk/chunkAPI.h"
#pragma endregion
#pragma region Operations
/// @brief Destroy the world data of the chunk
void chunk_world_destroy(Chunk_t *pChunk);

/// @brief Destroy the chunk, which involves freeing its members and the chunk allocation itself. pCtx = State_t *pState
void chunk_destroy(void *pCtx, Chunk_t *pChunk);

/// @brief Instantiate a chunk with allocations for storing world data
Chunk_t *chunk_world_create(const Vec3i_t CHUNK_POS);
#pragma endregion