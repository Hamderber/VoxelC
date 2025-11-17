#pragma once

#include "core/types/state_t.h"
#include "api/chunk/chunkAPI.h"

void world_chunk_addToCollection(State_t *restrict pState, Chunk_t *restrict pChunk);

void world_loop(State_t *pState);

void world_chunks_load(State_t *restrict pState, Entity_t *restrict pLoadingEntity, const Vec3i_t CHUNK_POS, const uint32_t RADIUS);

void world_load(State_t *pState);

void world_destroy(State_t *pState);