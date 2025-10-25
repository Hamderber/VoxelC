#pragma once

#include "core/types/state_t.h"

/// @brief Initializes the actual application
void app_init(State_t *state);

/// @brief Window/GUI/rendering-based per-frame application loop
void app_loop_render(State_t *state);

/// @brief Logic-based primary application loop
void app_loop_main(State_t *state);

/// @brief Destroy/deallocate/etc (cleanup) the actual application
void app_cleanup(State_t *state);