#pragma once

#include "core/types/state_t.h"
#include "character/characterType_t.h"
#include "world/worldState_t.h"

/// @brief Gets the player character's position. It is lerped between current position and the previous physics frame position
/// to allow for frame-based motion or calculations without causing artifacting/jitter
Vec3f_t character_player_positionLerped_get(const State_t *pSTATE);

Character_t *character_create(State_t *state, CharacterType_t characterType);