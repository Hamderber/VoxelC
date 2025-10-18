#pragma once

#include <stdbool.h>
#include "core/types/state_t.h"
#include "input/types/inputActionMapping_t.h"
#include "input/types/inputKey_t.h"
#include "events/eventBus.h"

InputActionMapping_t input_keyActionGet(State_t *state, int key);

void input_inputActionSimulate(State_t *state, InputActionMapping_t inputActionMapped, CtxDescriptor_t actionState);

bool input_keyHasInputAction(State_t *state, int key);

void input_update(State_t *state);

void input_init(State_t *state);