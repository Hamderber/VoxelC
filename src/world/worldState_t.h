#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "api/chunk/chunkAPI.h"
#include "world/world_t.h"
#include "entity/entity_t.h"
#include "world/chunk.h"
#include "collection/linkedList_t.h"
#include "rendering/types/chunkRenderer_t.h"

typedef struct WorldState_t
{
    ChunkSource_t *pChunkSource;
    bool isLoaded;
    World_t world;
    Entity_t *pPlayerEntity;
    // Assign chunks that will be permanently loaded to this instead of depending on a specific player
    Entity_t *pChunkLoadingEntity;
    LinkedList_t *pChunksLL;
    ChunkRenderer_t chunkRenderer;
} WorldState_t;