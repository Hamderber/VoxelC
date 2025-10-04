#pragma once

void init(State_t *state)
{
    logger(LOG_INFO, "Starting %s...", PROGRAM_NAME);
    logVulkanInfo();

    // Must init glfw first so that we can actually assign its error handler
    glfwInit();
    errorHandlingSetup();

    contextCreate(state);
    windowCreate(state);
    rendererCreate(state);
}

double renderLoop(State_t *state)
{
    double frameStartTime = glfwGetTime();

    // Handle the window events, including actually closing the window with the X
    windowPollEvents(state);

    // Must call this after the window poll events (glfwPollEvents(); specifically) because resizing the window and the
    // associated callback would be generated from that function. This will only hit AFTER the user has LET GO of the
    // side of the window during resize. This means that each time the window changes, the swapchain will only be recreated
    // once the user STOPS the resize process.
    if (state->window.swapchain.recreate)
    {
        // Make sure the GPU is idle. This could be a queue wait plus fence if more wait accuracy is needed
        vkDeviceWaitIdle(state->context.device);
        framebuffersDestroy(state);
        swapchainCreate(state);
        framebuffersCreate(state);
        logger(LOG_INFO, "Re-created the swapchain.");
        state->window.swapchain.recreate = false;
    }

    swapchainImageAcquireNext(state);
    commandBufferRecord(state);
    commandBufferSubmit(state);
    swapchainImagePresent(state);

    return glfwGetTime() - frameStartTime;
}

void loop(State_t *state)
{
    while (!windowShouldClose(state))
    {
        double frameTime = renderLoop(state);
        double frameFrequency = 1 / frameTime;
        // logger(LOG_INFO, "FPS: %lf", frameFrequency);
    }
}

void cleanup(State_t *state)
{
    // Order matters here (including order inside of destroy functions)because of potential physical device and interdependency.
    // instanceCreate() is called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out.
    // The window doesn't need to be destroyed because GLFW handles it on its own. Stated explicitly for legibility.
    rendererDestroy(state);
    windowDestroy(state);
    contextDestroy(state);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    state->window.swapchain.handle = NULL;
    state->context.instance = NULL;
    state->context.pAllocator = NULL;

    logger(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}
