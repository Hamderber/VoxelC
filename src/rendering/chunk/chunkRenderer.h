#pragma once
#include "world/worldState_t.h"

bool chunkRenderer_enqueueRemesh(WorldState_t *restrict pWorldState, Chunk_t *restrict pChunk);

void chunkRenderer_remeshChunks(State_t *pState);

bool chunkRenderer_create(WorldState_t *pWorldState);