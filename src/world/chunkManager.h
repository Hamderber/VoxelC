#pragma once

#include <stdbool.h>
#include "cmath/cmath.h"
#include "core/types/state_t.h"
#include "api/chunk/chunkAPI.h"
#include "chunk/chunkManager_t.h"
#include "world/voxel/block_t.h"

typedef enum ChunkQuery_e
{
    QUERY_CHUNK_LOADED,
    QUERY_CHUNK_UNLOADED,
    QUERY_CHUNK_LOADED_AND_UNLOADED,
} ChunkQuery_e;

/// @brief Gets the chunk at the chunk position. Returns null if not found.
Chunk_t *chunkManager_getChunk(const State_t *pSTATE, const Vec3i_t CHUNK_POS);

/// @brief Iterates the provided chunk positions and returns a heap array of chunk positions matching query from that collection.
/// Places the length of that collection into pCount. If there are duplicate chunkpos in the query then there will be duplicate
/// results. ORDER IS NON_DETERMINISTIC
Chunk_t **chunkManager_getChunks(const State_t *restrict pSTATE, const Vec3i_t *restrict pCHUNK_POS, const size_t numChunkPos,
                                 size_t *restrict pCount, const bool RESIZE);

/// @brief Returns an array (length = CUBE_FACE_COUNT) of pointers to each loaded chunk or null if it is not loaded around
/// neighbor chunk at CHUNK_POS. (Heap) indexed by CubeFace_e
Chunk_t **chunkManager_getChunkNeighbors(const State_t *pSTATE, const Vec3i_t CHUNK_POS);

/// @brief Adds the passed entity to each chunk in the passed collection's entity loading linked list.
bool chunkManager_chunk_addLoadingEntity(Chunk_t **ppChunks, size_t numChunks, Entity_t *pEntity);

/// @brief Gets the the block in the chunk's local coord system
const inline BlockVoxel_t chunkManager_getBlock(const Chunk_t *pCHUNK, const Vec3u8_t LOCAL_POS)
{
    return pCHUNK->pBlockVoxels[xyz_to_chunkBlockIndex(LOCAL_POS.x, LOCAL_POS.y, LOCAL_POS.z)];
}

/// @brief Gets the the block's render type in the chunk's local coord system
const inline BlockRenderType_e chunkManager_getBlockRenderType(const Chunk_t *pCHUNK, const Vec3u8_t LOCAL_POS)
{
    return pCHUNK->pBlockVoxels[xyz_to_chunkBlockIndex(LOCAL_POS.x, LOCAL_POS.y, LOCAL_POS.z)].pBLOCK_DEFINITION->BLOCK_RENDER_TYPE;
}

/// @brief Checks if the chunk is loaded (in the chunks linked list)
bool chunk_isLoaded(const State_t *pSTATE, const Vec3i_t CHUNK_POS);