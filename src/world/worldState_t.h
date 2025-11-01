#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "world/world_t.h"
#include "entity/entity_t.h"
#include "world/chunk.h"

typedef struct
{
    bool isLoaded;
    World_t world;
    Entity_t *pPlayerEntity;
    Chunk_t **ppChunks;
    uint32_t chunkCount;
} WorldState_t;