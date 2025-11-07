#pragma once

struct State_t;
/// @brief Initializes the actual application
void app_init(struct State_t *pState);

/// @brief Window/GUI/rendering-based per-frame application loop
void app_loop_render(struct State_t *pState);

/// @brief Logic-based primary application loop
void app_loop_main(struct State_t *pState);

/// @brief Destroy/deallocate/etc (cleanup) the actual application
void app_cleanup(struct State_t *pState);