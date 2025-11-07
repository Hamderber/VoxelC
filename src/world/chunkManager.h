#pragma once

#include <stdbool.h>
#include "cmath/cmath.h"
#include "core/types/state_t.h"
#include "world/chunk.h"

// Written to be accessed using CubeFace enum
static const Vec3i_t spNEIGHBOR_OFFSETS[6] = {{-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

/// @brief Gets the chunk at the chunk position. Returns null if not found.
Chunk_t *chunkManager_getChunk(const State_t *pSTATE, const Vec3i_t CHUNK_POS);

/// @brief Create chunks at the passed positions and add them directly to the world
bool chunkManager_chunk_createBatch(State_t *pState, const Vec3i_t *pCHUNK_POS, size_t count, Entity_t *pLoadingEntity);

/// @brief Gets the the block in the chunk's local coord system
const inline BlockVoxel_t chunkManager_getBlock(const Chunk_t *pCHUNK, const Vec3u8_t LOCAL_POS)
{
    return pCHUNK->pBlockVoxels[xyz_to_chunkBlockIndex(LOCAL_POS.x, LOCAL_POS.y, LOCAL_POS.z)];
}

/// @brief Checks if the chunk is loaded (in the chunks linked list)
bool chunk_isLoaded(State_t *pState, const Vec3i_t CHUNK_POS);

/// @brief Destroys the chunk and frees internals
void chunk_destroy(State_t *pState, Chunk_t *pChunk);

/// @brief Augment of linkedList_free_all that frees all chunks and their internals
void chunkManager_linkedList_destroy(State_t *pState, LinkedList_t **ppChunksLL);

/// @brief Subscribes to events in the chunk event channel
void chunkManager_create(State_t *pState);

/// @brief Unsubscribes to events in the chunk event channel
void chunkManager_destroy(State_t *pState);