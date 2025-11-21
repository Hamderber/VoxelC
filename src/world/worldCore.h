#pragma once
#include "world/worldType_e.h"
#include "core/types/state_t.h"

void worldCore_tick(State_t *pState, double deltaTime);

WorldState_t *worldCore_create(State_t *pState, const WorldType_e TYPE);

void worldCore_destroy(State_t *restrict pState, WorldState_t *restrict pWorldState);