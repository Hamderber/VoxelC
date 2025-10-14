#pragma once

#include "core/types/state_t.h"

void app_init(State_t *state);

void app_renderLoop(State_t *state);

void app_loop(State_t *state);

void app_cleanup(State_t *state);