#pragma once

#include "input/types/inputActionMapping_t.h"

// Keybindings configurable via json (todo)
typedef struct
{
    int key;
    bool keyUp;
    bool keyDown;
    bool pressedThisFrame;
    bool pressedLastFrame;
    InputActionMapping_t inputMapping;
} InputKey_t;