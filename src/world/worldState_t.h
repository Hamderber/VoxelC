#pragma once

#include <stdbool.h>
#include "world/world_t.h"
#include "entity/entity_t.h"

typedef struct
{
    bool isLoaded;
    World_t world;
    Entity_t *pPlayerEntity;
} WorldState_t;