#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "gui/gui_t.h"
#include "gui/mouse_t.h"

typedef struct
{
    bool menuOpen;
    // Even with a depth of 0, the OVERLAY GUI will be "open"
    uint32_t menuDepth;
    GUI_t GUIs[GUI_ID_COUNT];
    // Tracks the order of opened menus. Because the same menu can't be opened twice, the array length is the same
    // as the maximum amount of menus possible
    GUIMenuID_e guiStack[GUI_ID_COUNT];
    Mouse_t mouse;
} GUI_State_t;