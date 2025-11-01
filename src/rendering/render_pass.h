#pragma once

#include "core/types/state_t.h"

/// @brief The render pass is basically the blueprint for the graphics operation in the graphics pipeline
void renderpass_create(State_t *pState);

/// @brief Destroys the render pass
void renderpass_destroy(State_t *pState);