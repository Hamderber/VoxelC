#pragma once

#include "core/types/state_t.h"

/// @brief Gets the best supported depth format
VkFormat depthFormat_get(State_t *pState);

/// @brief Creates the depth resource
void depthResources_create(State_t *pState);

/// @brief Destroys the depth resource
void depthResources_destroy(State_t *pState);