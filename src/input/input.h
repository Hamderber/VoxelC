#pragma once

#include <stdbool.h>
#include "core/types/state_t.h"
#include "input/types/inputActionMapping_t.h"
#include "input/types/inputKey_t.h"
#include "events/eventBus.h"

/// @brief Gets the InputActionMapping_t assocaited with the keycode.
static inline InputActionMapping_t input_key_InputActionMapping_get(const State_t *pSTATE, const int KEYCODE)
{
    return pSTATE->input.pInputKeys[KEYCODE].inputMapping;
}

/// @brief Checks if the keycode is mapped to INPUT_ACTION_UNMAPPED within the state
static inline bool input_key_HasInputActionMapping(const State_t *pSTATE, const int KEYCODE)
{
    return pSTATE->input.pInputKeys[KEYCODE].inputMapping != INPUT_ACTION_UNMAPPED;
}

/// @brief Publishes an input event as if user input caused it
void input_inputAction_simulate(State_t *state, InputActionMapping_t inputActionMapped, CtxDescriptor_t actionState);

void input_poll(State_t *pState);

void input_init(const State_t *pSTATE);