#pragma once

#include "core/core.h"
#include "rendering/shaders.h"
#include "Renderer.h"
#include "gui/window.h"
#include "physics/physics.h"
#include "main.h"
#include "gui/swapchain.h"

void app_init(State_t *state)
{
    logs_log(LOG_INFO, "Starting %s...", PROGRAM_NAME);

    glfwi_init();

    vki_create(state);
    win_create(state);
    rendererCreate(state);

    time_init(state);
}

void app_renderLoop(State_t *state)
{
    // Handle the window events, including actually closing the window with the X
    win_pollEvents(state);

    // Must call this after the window poll events (glfwPollEvents(); specifically) because resizing the window and the
    // associated callback would be generated from that function. This will only hit AFTER the user has LET GO of the
    // side of the window during resize. This means that each time the window changes, the swapchain will only be recreated
    // once the user STOPS the resize process.
    if (state->window.swapchain.recreate)
    {
        // Make sure the GPU is idle. This could be a queue wait plus fence if more wait accuracy is needed
        vkDeviceWaitIdle(state->context.device);
        win_waitForValidFramebuffer(state);

        depthResourcesDestroy(state);
        framebuffersDestroy(state);

        sc_create(state);
        depthResourcesCreate(state);
        framebuffersCreate(state);

        logs_log(LOG_INFO, "Re-created the swapchain.");
        state->window.swapchain.recreate = false;
    }

    updateUniformBuffer(state);
    sc_imageAcquireNext(state);
    commandBufferRecord(state);
    commandBufferSubmit(state);
    sc_imagePresent(state);

    time_update(state);
}

void app_loop(State_t *state)
{
    while (!win_shouldClose(state))
    {
        physicsLoop(state);
        app_renderLoop(state);
        // logs_log(LOG_INFO, "FPS: %lf Frame: %d", state->time.framesPerSecond, state->renderer.currentFrame);
    }
}

void app_cleanup(State_t *state)
{
    // Order matters here (including order inside of destroy functions)because of potential physical device and interdependency.
    // vki_instanceCreate() is called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out.
    // The window doesn't need to be destroyed because GLFW handles it on its own. Stated explicitly for legibility.
    rendererDestroy(state);
    win_destroy(state);
    vki_destroy(state);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    state->window.swapchain.handle = NULL;
    state->context.instance = NULL;
    state->context.pAllocator = NULL;

    logs_log(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}
