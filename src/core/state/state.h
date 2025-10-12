#pragma once

#include "core/config.h"
#include "window_t.h"
#include "context_t.h"
#include "core/app_time.h"
#include "renderer_t.h"

typedef struct
{
    Config_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
} State_t;
