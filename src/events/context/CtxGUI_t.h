#pragma once

#include <stdint.h>
#include "gui/gui_t.h"

typedef struct
{
    uint32_t menuDepth;
    GUI_t *gui;
} CtxGUI_t;