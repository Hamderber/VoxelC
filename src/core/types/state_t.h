#pragma once

#include "core/config.h"
#include "core/types/window_t.h"
#include "core/types/context_t.h"
#include "core/app_time.h"
#include "core/types/renderer_t.h"

typedef struct
{
    AppConfig_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
} State_t;
