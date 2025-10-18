#pragma once

#include "core/config.h"
#include "core/types/window_t.h"
#include "core/types/context_t.h"
#include "core/app_time.h"
#include "core/types/renderer_t.h"
#include "events/eventTypes.h"
#include "rendering/camera/camera_t.h"
#include "entity/entityManager.h"
#include "input/types/input_t.h"
#include "gui/guiState_t.h"

typedef struct State_t
{
    AppConfig_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
    EventBus_t eventBus;
    EntityManger_t entityManager;
    Input_t input;
    GUI_State_t gui;
} State_t;
