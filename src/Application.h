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

void loop(State_t *state)
{
    while (!windowShouldClose(state))
    {
        // Handle the window events, including actually closing the window with the X
        windowPollEvents(state);

        // Must call this after the window poll events (glfwPollEvents(); specifically) because resizing the window and the
        // associated callback would be generated from that function. This will only hit AFTER the user has LET GO of the
        // side of the window during resize. This means that each time the window changes, the swapchain will only be recreated
        // once the user STOPS the resize process.
        if (state->window.swapchain.recreate)
        {
            state->window.swapchain.recreate = false;
            swapchainCreate(state);
            logger(LOG_INFO, "Re-created the swapchain.");
        }

        // uint64_t imageTimeout = UINT64_MAX;
        // // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
        // // Fence: same above but for CPU
        // VkSemaphore semaphore = NULL;
        // VkFence fence = NULL;
        // uint32_t imageIndex;
        // LOG_IF_ERROR(vkAcquireNextImageKHR(state->device, state->swapchain, imageTimeout, semaphore, fence, &imageIndex),
        //              "Failed to aquire the next image from the swapchain. Index: %d", imageIndex)

        // VkPresentInfoKHR presentInfo = {
        //     .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        //     // Don't just pass the address to imageIndex because it wouldn't be a const. Declare it inline
        //     .pImageIndices = &(uint32_t){imageIndex},
        //     .swapchainCount = 1,
        //     .pSwapchains = &state->swapchain,
        // };

        // // Can't just catch this result with the error logger. Actually have to handle it.
        // VkResult result = vkQueuePresentKHR(state->queue, &presentInfo);

        // // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
        // // MUST be recreated immediately and presentation will just be attempted on the next frame.
        // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        // {
        //     logger(LOG_WARN, "The swapchain is out of date and must be recreated! VkResult = %d", result);
        //     state->recreateSwapchain = true;
        // }
        // else
        // {
        //     LOG_IF_ERROR(result,
        //                  "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.")
        // }
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
    state->context.allocator = NULL;

    logger(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}
