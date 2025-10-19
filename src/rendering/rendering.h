#pragma once

#include "core/types/state_t.h"
#include "events/eventTypes.h"

EventResult_t rend_onWireframeTogglePress(struct State_t *state, Event_t *event, void *ctx);

void rend_recreate(State_t *state);

void rend_present(State_t *state);

void rend_create(State_t *state);

void rend_destroy(State_t *state);
