#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "world/world_t.h"
#include "entity/entity_t.h"
#include "rendering/types/renderChunk_t.h"

typedef struct
{
    bool isLoaded;
    World_t world;
    Entity_t *pPlayerEntity;
    RenderChunk_t **ppRenderChunks;
    uint32_t renderChunkCount;
} WorldState_t;