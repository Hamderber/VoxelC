#pragma once

#include "inputActionMapping_t.h"
#include "events/context/CtxInputMapped_t.h"

#define INPUT_ACTION_QUERY_COUNT_MAX 64

typedef struct
{
    InputActionMapping_t mapping;
    CtxDescriptor_t actionCtx;
} InputActionQuery_t;