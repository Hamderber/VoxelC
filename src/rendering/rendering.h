#pragma once

#include "core/types/state_t.h"
#include "events/eventTypes.h"

/// @brief Event subscriber for toggling the wireframe pipeline
EventResult_t rendering_wireframe_onTogglePress(State_t *pState, Event_t *pEvent, void *pCtx);

/// @brief Recreate the rendering system
void rendering_recreate(State_t *pState);

/// @brief The primary function for consolidating data for the GPU to present something to the window
void rendering_present(State_t *pState);

/// @brief Create the rendering system
void rendering_create(State_t *pState);

/// @brief Destroy the rendering system
void rendering_destroy(State_t *pState);
