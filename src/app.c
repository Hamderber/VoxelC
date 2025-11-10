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
#include "core/random.h"
#include "gui/swapchain.h"
#include "rendering/types/renderModel_t.h"
#include "scene/scene.h"
#include "rendering/model_3d.h"
#include "core/randomNoise.h"
#include "cmath/weightedMaps.h"

void app_init(State_t *restrict pState)
{
    logs_log(LOG_INFO, "Starting %s...", PROGRAM_NAME);

    config_init(pState);

    const uint32_t PRNG_SEED = 0; // 8675309U;
    random_init(PRNG_SEED);
    randomNoise_init(random_seedGet());
    weightedMaps_init(pState);

    cmath_instantiate();

    glfwInstance_init();

    vulkan_init(pState);

    window_init(pState);

    swapchain_create(pState);

    // MUST be called AFTER window_init because the Window_t is assigned there
    input_init(pState);

    rendering_create(pState);

    time_init(&pState->time);

    events_init(&pState->eventBus);

    em_init(pState);

    gui_init(pState);

    world_load(pState);

    scene_model_createAll(pState);
}

void app_loop_render(State_t *restrict pState)
{
    // Handle the window events, including actually closing the window with the X
    window_events_poll();

    // Handle all inputs since the last frame displayed (GLFW)
    input_poll(pState);

    // Must call this after the window poll events (glfwPollEvents(); specifically) because resizing the window and the
    // associated callback would be generated from that function. This will only hit AFTER the user has LET GO of the
    // side of the window during resize. This means that each time the window changes, the swapchain will only be recreated
    // once the user STOPS the resize process.
    if (pState->window.swapchain.recreate)
    {
        rendering_recreate(pState);
    }

    rendering_present(pState);

    time_update(&pState->time);
}

void app_loop_main(State_t *restrict pState)
{
    while (!win_shouldClose(&pState->window))
    {
        phys_loop(pState);
        app_loop_render(pState);
        // logs_log(LOG_DEBUG, "FPS: %lf Frame: %d", state->time.framesPerSecond, state->renderer.currentFrame);
    }
}

void app_cleanup(State_t *restrict pState)
{
    scene_destroy(pState);
    world_destroy(pState);

    // Order matters here (including order inside of destroy functions)because of potential physical device and interdependency.
    // vulkan_init() is called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out.
    // The window doesn't need to be destroyed because GLFW handles it on its own. Stated explicitly for legibility.
    rendering_destroy(pState);
    window_destroy(pState);
    vulkan_instance_destroy(pState);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    pState->window.swapchain.handle = NULL;
    pState->context.instance = NULL;
    pState->context.pAllocator = NULL;

    em_destroy(pState);

    weightedMaps_destroy(pState);
    cmath_destroy();

    logs_log(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}
