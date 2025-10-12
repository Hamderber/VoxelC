#pragma once

#include "core/types/state_t.h"

void sc_imageAcquireNext(State_t *state);

void sc_imagePresent(State_t *state);

void sc_create(State_t *state);

void sc_destroy(State_t *state);