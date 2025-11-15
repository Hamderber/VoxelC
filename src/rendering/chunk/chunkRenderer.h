#pragma once
#include "world/worldState_t.h"
#include "rendering/types/chunkRemeshCtx_t.h"

bool chunkRenderer_enqueueRemesh(WorldState_t *restrict pWorldState, ChunkRemeshCtx_t *restrict pCtx);

void chunkRenderer_remeshChunks(State_t *pState);

bool chunkRenderer_create(WorldState_t *pWorldState);