#pragma once

#include "core/state/state.h"

void sc_imageAcquireNext(State_t *state);

void sc_imagePresent(State_t *state);

void sc_create(State_t *state);

void sc_destroy(State_t *state);