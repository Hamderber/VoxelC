#pragma once

#include "core/types/state_t.h"
#include "world/chunk.h"
#include "api/chunk/chunkAPI.h"

void world_chunk_addToCollection(State_t *pState, Chunk_t *pChunk);

/// @brief Loads chunks in the cubic volume centered at chunkPos
void world_chunks_load(State_t *pState, Entity_t *pLoadingEntity, const Vec3i_t CHUNK_POS, const uint32_t RADIUS);

void world_loop(State_t *pState);

void world_load(State_t *pState);

void world_destroy(State_t *pState);