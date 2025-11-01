#pragma once

#include "core/types/state_t.h"
#include "world/chunk.h"

void world_chunk_addToCollection(State_t *pState, Chunk_t *pChunk);

void world_load(State_t *pState);

void world_destroy(State_t *pState);