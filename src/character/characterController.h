#pragma once

#include "core/types/state_t.h"
#include "character/characterType_t.h"

static const float DEFAULT_UNIFORM_SPEED = 12.0F;
static const float SPRINT_SPEED_MULTIPLIER = 1.5F;

void player_physicsIntentUpdate(State_t *state);

void character_init(State_t *state, Character_t *character);