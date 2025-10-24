#pragma once

#include "core/logs.h"
#include "core/glfw_instance.h"
#include "core/vk_instance.h"
#include "rendering/shaders.h"
#include "rendering/rendering.h"
#include "gui/window.h"
#include "physics/physics.h"
#include "main.h"
#include "core/config.h"
#include "input/input.h"
#include "events/eventBus.h"
#include "entity/entityManager.h"
#include "gui/guiController.h"
#include "world/world.h"
#include "rendering/types/renderModel_t.h"
#include "rendering/model_3d.h"
#include "scene/scene.h"

void app_init(State_t *state)
{
    logs_log(LOG_INFO, "Starting %s...", PROGRAM_NAME);

    glfwi_init();

    vki_create(state);

    win_create(state);

    // MUST be called AFTER win_create because the Window_t is assigned there
    input_init(state);

    rend_create(state);

    time_init(&state->time);

    events_init(&state->eventBus);

    em_init(state);

    gui_init(state);

    world_load(state);

    RenderModel_t *mdl = m3d_load(state,
                                  MODEL_PATH "complex_test.glb",
                                  RESOURCE_TEXTURE_PATH "complex_test.png");

    mdl->modelMatrix = cmath_mat_setTranslation(MAT4_IDENTITY, (Vec3f_t){-1.0F, 0.0F, 0.0F});

    scene_modelCreate(&state->scene, mdl);

    // wordl offsed
}

void app_renderLoop(State_t *state)
{
    // Handle the window events, including actually closing the window with the X
    win_pollEvents();

    // Handle all inputs since the last frame displayed (GLFW)
    input_update(state);

    // Must call this after the window poll events (glfwPollEvents(); specifically) because resizing the window and the
    // associated callback would be generated from that function. This will only hit AFTER the user has LET GO of the
    // side of the window during resize. This means that each time the window changes, the swapchain will only be recreated
    // once the user STOPS the resize process.
    if (state->window.swapchain.recreate)
    {
        rend_recreate(state);
    }

    rend_present(state);

    time_update(&state->time);
}

void app_loop(State_t *state)
{
    while (!win_shouldClose(&state->window))
    {
        phys_loop(state);
        app_renderLoop(state);
        // logs_log(LOG_DEBUG, "FPS: %lf Frame: %d", state->time.framesPerSecond, state->renderer.currentFrame);
    }
}

void app_cleanup(State_t *state)
{
    scene_destroy(state);
    world_destroy(state);

    // Order matters here (including order inside of destroy functions)because of potential physical device and interdependency.
    // vki_instanceCreate() is called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out.
    // The window doesn't need to be destroyed because GLFW handles it on its own. Stated explicitly for legibility.
    rend_destroy(state);
    win_destroy(state);
    vki_destroy(state);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    state->window.swapchain.handle = NULL;
    state->context.instance = NULL;
    state->context.pAllocator = NULL;

    em_destroy(state);

    logs_log(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}
