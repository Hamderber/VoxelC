#pragma once

#include "events/context/CtxDescriptor_t.h"

typedef struct
{
    int keyCode;
    CtxDescriptor_e direction;
} Keystroke_t;

typedef struct
{
    Keystroke_t *keystrokes;
    size_t keyCount;
} CtxInputRaw_t;