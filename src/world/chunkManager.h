#pragma once

#include <stdbool.h>
#include "cmath/cmath.h"
#include "core/types/state_t.h"
#include "world/chunk.h"

/// @brief Gets the chunk at the chunk position. Returns null if not found.
Chunk_t *chunkManager_getChunk(State_t *pState, ChunkPos_t chunkPos);

/// @brief Create chunks at the passed positions and add them directly to the world
bool chunkManager_chunk_createBatch(State_t *pState, const ChunkPos_t *pCHUNK_POS, const size_t COUNT);

/// @brief Gets the the block in the chunk's local coord system
const inline BlockVoxel_t chunkManager_getBlock(const Chunk_t *pCHUNK, const Vec3i_t LOCAL_POS)
{
    return pCHUNK->pBlockVoxels[xyz_to_chunkBlockIndex(LOCAL_POS.x, LOCAL_POS.y, LOCAL_POS.z)];
}