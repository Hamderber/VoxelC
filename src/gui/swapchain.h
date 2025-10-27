#pragma once

#include "core/types/state_t.h"

/// @brief Gets the next image in the swapchain and assigns it in the state's renderer
void swapchain_image_acquireNext(State_t *pState);

/// @brief Presents the current swapchain image to the window (adds to queue) and increments the current frame
void swapchain_image_present(State_t *pState);

/// @brief Creates the actual swapchain
void swapchain_create(State_t *pState);

/// @brief Destroys the swapchain
void swapchain_destroy(State_t *pState);