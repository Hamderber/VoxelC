#pragma once

#include "core/types/state_t.h"

/// @brief Destroy the GPU/CPU sync objects
void syncObjects_destroy(State_t *pState);

/// @brief Create the GPU/CPU sync objects
void syncObjects_create(State_t *pState);
