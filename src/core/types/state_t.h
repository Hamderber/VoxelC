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
#include "world/worldState_t.h"
#include "core/types/scene_t.h"
#include "cmath/weightedMap_t.h"
#include "events/eventTypes.h"
#include "world/worldConfig_t.h"

typedef struct State_t
{
    struct AppConfig_t config;
    struct Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
    EventBus_t eventBus;
    struct EntityManger_t entityManager;
    Input_t input;
    GUI_State_t gui;

    // Contains data on non-voxel models for rendering
    Scene_t scene;
    struct WorldState_t *pWorldState;
    struct WorldConfig_t worldConfig;

    // Math
    WeightMaps_t weightedMaps;
} State_t;
