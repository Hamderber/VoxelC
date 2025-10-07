#pragma once

uint32_t clamp_uint32_t(uint32_t num, uint32_t min, uint32_t max)
{
    if (num < min)
    {
        return min;
    }
    else if (num > max)
    {
        return max;
    }
    else
    {
        return num;
    }
}

void glfwErrorCallback(int errorCode, const char *description)
{
    LOG_IF_ERROR(errorCode,
                 "GLFW: %s", description)
}

void exitCallback(void)
{
    // While the OS will handle the garbage collection/etc of all code at program termination anyway, a safe-exit
    // isn't necessary. But because glfw is a windowing library, it could've potentially altered system settings
    // for rendering/etc. It is very important to safely terminate glfw before fully exiting the program.
    glfwTerminate();
}

void errorHandlingSetup()
{
    // Set function that is called when a glfw error is caught
    glfwSetErrorCallback(glfwErrorCallback);
    // Set function to be called at program termination
    atexit(exitCallback);
}

/// @brief This is requried even if window-is-resizable is hard-coded to false! This is because the user can
/// always minimize the window, which also causes a frame buffer size change.
/// @param window
/// @param frameBufferWidth
/// @param frameBufferHeight
void glfwFramebufferSizeCallback(GLFWwindow *window, int frameBufferWidth, int frameBufferHeight)
{
    logger(LOG_INFO, "Frame buffer size changed to %d by %d (WxH)", frameBufferWidth, frameBufferHeight);
    // The user pointer is saved as a void pointer, but we are effectively explicitly casting that void pointer back
    // to the State_t type.
    State_t *state = glfwGetWindowUserPointer(window);

    state->window.swapchain.recreate = true;
    state->window.frameBufferWidth = frameBufferWidth;
    state->window.frameBufferHeight = frameBufferHeight;
}
