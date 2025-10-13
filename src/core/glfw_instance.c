#include <stdio.h>
#include <stdlib.h>
#include "core/logs.h"
#include <GLFW/glfw3.h>
#include "core/types/state_t.h"

/// @brief Logs the GLFW version
/// @param  void
void glfwi_logInfo(void)
{
    logs_log(LOG_DEBUG, "GLFW %s", glfwGetVersionString());
}

/// @brief Logs the error code. This function is called when there is a GLFW error
/// @param errorCode
/// @param description
static void glfwi_errorCallback(int errorCode, const char *description)
{
    logs_logIfError(errorCode,
                    "GLFW: %s", description);
}

/// @brief Terminates GLFW (very important). Function is called on program exit.
/// @param  void
static void glfwi_exitCallback(void)
{
    // While the OS will handle the garbage collection/etc of all code at program termination anyway, a safe-exit
    // isn't necessary. But because glfw is a windowing library, it could've potentially altered system settings
    // for rendering/etc. It is very important to safely terminate glfw before fully exiting the program.
    glfwTerminate();
}

/// @brief Assigns functions to the GLFW callbacks (error, exit)
/// @param  void
static void glfwi_errorHandlingSetup(void)
{
    // Set function that is called when a glfw error is caught
    glfwSetErrorCallback(glfwi_errorCallback);
    // Set function to be called at program termination
    atexit(glfwi_exitCallback);
}

/// @brief This is requried even if window-is-resizable is hard-coded to false! This is because the user can
/// always minimize the window, which also causes a frame buffer size change.
/// @param window
/// @param frameBufferWidth
/// @param frameBufferHeight
void glfwi_framebufferSizeCallback(GLFWwindow *window, int frameBufferWidth, int frameBufferHeight)
{
    logs_log(LOG_DEBUG, "Frame buffer size changed to %d by %d (WxH)", frameBufferWidth, frameBufferHeight);
    // The user pointer is saved as a void pointer, but we are effectively explicitly casting that void pointer back
    // to the State_t type. GLFW lets us define a pointer, which is why state wasnt passed here
    State_t *state = glfwGetWindowUserPointer(window);

    state->window.swapchain.recreate = true;
    state->window.frameBufferWidth = frameBufferWidth;
    state->window.frameBufferHeight = frameBufferHeight;
}

/// @brief Initalizes GLFW. Logs info (debug) and establishes callbacks.
/// @param  void
void glfwi_init(void)
{
    // Must init glfw first so that we can actually assign its error handler
    glfwInit();
    glfwi_logInfo();
    glfwi_errorHandlingSetup();
}
