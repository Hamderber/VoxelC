#pragma once

#include "core/types/state_t.h"

/// @brief Creates and binds the descriptor set layouts. Must be done before creating the descriptor pool and graphics pipelines.
void descriptorSet_layout_create(State_t *pState);

/// @brief Creates the descriptor pool
void descriptorPool_create(State_t *pState);

/// @brief Destroys the descriptor pool
void descriptorPool_destroy(State_t *pState);
