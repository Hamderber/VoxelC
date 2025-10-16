#pragma once

#include "core/config.h"
#include "core/types/window_t.h"
#include "core/types/context_t.h"
#include "core/app_time.h"
#include "core/types/renderer_t.h"
#include "events/eventTypes.h"
#include "rendering/camera/camera_t.h"
#include "entity/entityManager.h"

typedef struct State_t
{
    AppConfig_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
    EventBus_t eventBus;
    EntityManger_t entityManager;
} State_t;
