#pragma once

#include <GLFW/glfw3.h>
#include "input/types/inputActionMapping_t.h"

typedef struct
{
    InputActionMapping_t action;
    int defaultKey;
} DefaultKeyMapping_t;

static const DefaultKeyMapping_t DEFAULT_KEY_MAPPINGS[] = {
    {INPUT_ACTION_TOGGLE_FULLSCREEN, GLFW_KEY_F11},
    {INPUT_ACTION_MOVE_FORWARD, GLFW_KEY_W},
    {INPUT_ACTION_MOVE_BACKWARD, GLFW_KEY_S},
    {INPUT_ACTION_MOVE_LEFT, GLFW_KEY_A},
    {INPUT_ACTION_MOVE_RIGHT, GLFW_KEY_D},
    {INPUT_ACTION_JUMP, GLFW_KEY_SPACE},
    {INPUT_ACTION_CROUCH, GLFW_KEY_LEFT_SHIFT},
    {INPUT_ACTION_MENU_TOGGLE, GLFW_KEY_ESCAPE},
};

static const size_t DEFAULT_KEY_MAPPINGS_COUNT =
    sizeof(DEFAULT_KEY_MAPPINGS) / sizeof(DEFAULT_KEY_MAPPINGS[0]);