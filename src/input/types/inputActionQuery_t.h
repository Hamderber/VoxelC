#pragma once

#include "inputActionMapping_t.h"
#include "events/context/CtxInputMapped_t.h"

#define INPUT_ACTION_QUERY_COUNT_MAX 64

typedef struct
{
    InputActionMapping_e mapping;
    CtxDescriptor_e actionCtx;
} InputActionQuery_t;