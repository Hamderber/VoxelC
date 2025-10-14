#pragma once

#include <GLFW/glfw3.h>
#include "input/types/inputKey_t.h"

typedef struct
{
    // Iterate through this with: int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; key++
    InputKey_t pInputKeys[GLFW_KEY_LAST + 1];
    bool pMouseButtons[GLFW_MOUSE_BUTTON_LAST + 1];
    double mouseX, mouseY;
    double mouseDeltaX, mouseDeltaY;
    double scrollDelta;
} Input_t;