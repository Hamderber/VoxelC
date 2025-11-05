#pragma once

#include "core/types/state_t.h"
#include "character/characterType_t.h"

static const float DEFAULT_UNIFORM_SPEED = 48.0F; // 12.0F;
static const float SPRINT_SPEED_MULTIPLIER = 1.5F;

/// @brief Updates the player's physics intent via character controller's input
void player_physicsIntentUpdate(State_t *pState);

/// @brief Initializes the given character's controller
void character_init(State_t *pState, Character_t *pCharacter);