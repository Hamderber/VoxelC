#pragma once

#include "input/types/inputActionMapping_t.h"
#include "events/context/CtxDescriptor_t.h"

typedef struct
{
    InputActionMapping_t action;
    CtxDescriptor_t actionState;
} InputAction_t;

typedef struct
{
    InputAction_t *inputActions;
    size_t actionCount;
} CtxInputMapped_t;