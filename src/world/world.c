#include <string.h>
#include <stdlib.h>
#include "core/types/state_t.h"
#include "world/world_t.h"
#include "character/characterType_t.h"
#include "character/character.h"

void world_init(State_t *state)
{
    state->worldState = calloc(1, sizeof(WorldState_t));

    state->worldState->world.pPlayer = character_create(state, CHARACTER_TYPE_PLAYER);
}

void world_load(State_t *state)
{
    world_init(state);
}