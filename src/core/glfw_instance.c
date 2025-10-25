#include <stdio.h>
#include <stdlib.h>
#include "core/logs.h"
#include <GLFW/glfw3.h>
#include "core/types/state_t.h"
#include "threading/threading.h"

/// @brief Logs the GLFW version
static inline void instance_logInfo(void) { logs_log(LOG_DEBUG, "GLFW %s", glfwGetVersionString()); }

/// @brief Callback for when there is a GLFW error
static void instance_error_callback(int errorCode, const char *description)
{
#if defined(DEBUG)
    logs_logIfError(errorCode, "GLFW: Error [%d] %s", errorCode, description);
#else
    logs_log(LOG_ERROR, "GLFW: Error [%d] %s", errorCode, description);
#endif
}

/// @brief Terminates GLFW instance.
static void instance_exit_callback(void)
{
    // Only terminate GLFW if the thread that has exited is the main thread
    if (!threading_thread_isMain())
        return;

    // While the OS will handle the garbage collection/etc of all code at program termination anyway, a safe-exit
    // isn't necessary. But because glfw is a windowing library, it could've potentially altered system settings
    // for rendering/etc. It is very important to safely terminate glfw before fully exiting the program.
    glfwTerminate();
    logs_log(LOG_DEBUG, "Terminated the GLFW instance.");
}

/// @brief Assign GLFW callbacks
static void instance_error_handlingSetup(void)
{
    glfwSetErrorCallback(instance_error_callback);
    // Program exit
    atexit(instance_exit_callback);
}

void glfwInstance_framebuffer_sizeCallback(GLFWwindow *pWindow, int frameBufferWidth, int frameBufferHeight)
{
    logs_log(LOG_DEBUG, "Frame buffer size changed to %d by %d (WxH)", frameBufferWidth, frameBufferHeight);

    State_t *pState = glfwGetWindowUserPointer(pWindow);
    if (!pState)
    {
        logs_log(LOG_ERROR, "Failed to get state user pointer from GLFW!");
        return;
    }

    pState->window.swapchain.recreate = true;
    pState->window.frameBufferWidth = frameBufferWidth;
    pState->window.frameBufferHeight = frameBufferHeight;
}

void glfwInstance_init(void)
{
    instance_error_handlingSetup();

    if (glfwInit() != GLFW_TRUE)
    {
        logs_logIfError(true, "Failed to initialize GLFW!");
        abort();
    }

    instance_logInfo();
}
