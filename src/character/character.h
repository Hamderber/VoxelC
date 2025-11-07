#pragma once

#include "cmath/cmath.h"

struct State_t;
struct Entity_t;
union EntityComponentData_t;
enum CharacterType_t;

/// @brief Gets the player character's position. It is lerped between current position and the previous physics frame position
/// to allow for frame-based motion or calculations without causing artifacting/jitter
Vec3f_t character_player_positionLerped_get(const struct State_t *pSTATE);

/// @brief Publishes a CHUNK_EVENT with the character and current chunk that the character is in
void entity_player_chunkPos_update_publish(struct State_t *pState, struct Entity_t *pEntity, union EntityComponentData_t *pComponentData);

/// @brief Creates a character of the given type
struct Character_t *character_create(struct State_t *pState, enum CharacterType_t characterType);