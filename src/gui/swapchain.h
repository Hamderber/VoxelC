#pragma once

#include "core/types/state_t.h"

void sc_imageAcquireNext(State_t *state);

void sc_imagePresent(State_t *state);

/// @brief Creates the actual swapchain
void swapchain_create(State_t *state);

/// @brief Destroys the swapchain
void swapchain_destroy(State_t *state);