#pragma region Includes
#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <inttypes.h>
#include "core/glfw_instance.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"
#include "gui/swapchain.h"
#include "gui/mouse.h"
#include "events/eventBus.h"
#include "input/input.h"
#include "gui/guiController.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Window Ops.
void window_fullscreen_toggle(State_t *pState, bool toggle)
{
    pState->window.fullscreen = toggle;

    GLFWwindow *pWindow = pState->window.pWindow;
    GLFWmonitor *pMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *pMODE = glfwGetVideoMode(pMonitor);

    if (pState->window.fullscreen)
    {
        // Preserve aspect before fullscreen
        glfwGetWindowPos(pWindow, &pState->window.prevX, &pState->window.prevY);
        glfwGetWindowSize(pWindow, &pState->window.widthPrev, &pState->window.heightPrev);

        // Set fullscreen (exclusive)
        glfwSetWindowMonitor(pWindow, pMonitor, 0, 0, pMODE->width, pMODE->height, GLFW_DONT_CARE);

        logs_log(LOG_DEBUG, "Entered fullscreen (%dx%d@%dHz)", pMODE->width, pMODE->height, pMODE->refreshRate);

        pState->window.fullscreen = true;
    }
    else
    {
        // Restore previous windowed mode
        glfwSetWindowMonitor(pWindow, NULL, pState->window.prevX, pState->window.prevY,
                             pState->window.widthPrev, pState->window.heightPrev, GLFW_DONT_CARE);

        logs_log(LOG_DEBUG, "Exited fullscreen, returning to %dx%d", pState->window.widthPrev, pState->window.heightPrev);
    }
}

void window_waitForValidFramebuffer(Window_t *pWindow)
{
    // Vulkan errors if the window is minimized and it tries to create a 0x0 size swapchain
    int width = 0, height = 0;

    // Block until the window is restored (not minimized)
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(pWindow->pWindow, &width, &height);
        // sleep until resize/restored event occurs
        glfwWaitEvents();
    }

    pWindow->frameBufferWidth = width;
    pWindow->frameBufferHeight = height;
}
#pragma region endregion
#pragma region Surface Ops.
VkSurfaceFormatKHR window_surfaceFormats_select(const Context_t *pCONTEXT, const Window_t *pWINDOW)
{
    VkSurfaceFormatKHR format = {0};
    VkSurfaceFormatKHR *pFormats = NULL;
    uint32_t count;
    int crashLine = 0;
    do
    {
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(pCONTEXT->physicalDevice, pWINDOW->surface, &count, NULL) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to query Vulkan physical device surface format count!");
            break;
        }

        if (count == 0)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Found no supported Vulkan physical device surface formats!");
            break;
        }

        pFormats = malloc(sizeof(VkSurfaceFormatKHR) * count);
        if (!pFormats)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for Vulkan physical device surface formats!");
            break;
        }

        if (vkGetPhysicalDeviceSurfaceFormatsKHR(pCONTEXT->physicalDevice, pWINDOW->surface, &count, pFormats) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to query Vulkan physical device surface formats!");
            break;
        }

        // Default to the first format ...
        format = pFormats[0];
        for (uint32_t i = 0; i < count; i++)
            // SRGB is the most commonly supported (and best) so we want that one if available
            // B8G8R8A8 = 8-bit blue/green/red/alpha components (so a 32-bit color depth)
            if (pFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR && pFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
            {
                // ... Unless the preferred format is found.
                format = pFormats[i];
                break;
            }

        logs_log(LOG_DEBUG, "Format %s %" PRIu32 " selected for Vukan physical device presentation.",
                 VkFormatToString(format.format), (uint32_t)format.format);
    } while (0);

    free(pFormats);
    if (crashLine != 0)
        crashHandler_crash_graceful(
            CRASH_LOCATION_LINE(crashLine),
            "The program cannot continue without a format to present to the Vulkan physical device with.");
    return format;
}

VkPresentModeKHR window_surfacePresentModes_select(const AppConfig_t *pCONFIG, const Context_t *pCONTEXT, const Window_t *pWINDOW)
{
    // Default to FIFO if immediate/mailbox conditon's aren't met
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VkPresentModeKHR *pPresentModes = NULL;
    // See https://www.youtube.com/watch?v=nSzQcyQTtRY for the different present modes (visual examples)
    uint32_t count = 0;
    int crashLine = 0;
    do
    {
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(pCONTEXT->physicalDevice, pWINDOW->surface, &count, NULL) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to query physical device surface presentation modes!");
            break;
        }

        pPresentModes = malloc(sizeof(VkPresentModeKHR) * count);
        if (!pPresentModes)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for presentation modes!");
            break;
        }

        if (vkGetPhysicalDeviceSurfacePresentModesKHR(pCONTEXT->physicalDevice, pWINDOW->surface, &count, pPresentModes) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to query physical device surface presentation modes!");
            break;
        }

        if (count == 0)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "No surface presentation modes found for the Vulkan physical device!");
            break;
        }

        bool stop = false;
        for (uint32_t i = 0; i < count && !stop; i++)
            // If vsync is disabled, IMMEDIATE if supported (frames as fast as possible, WILL cause screen tearing)
            if (!pCONFIG->vsync && pPresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
            {
                presentMode = pPresentModes[i];
                stop = true;
            }
            // If vsync enabled, prefer mailbox if available
            else if (pCONFIG->vsync && pPresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode = pPresentModes[i];
                stop = true;
            }
    } while (0);

    free(pPresentModes);
    logs_log(LOG_DEBUG, "Vulkan presentation mode: %s", VkPresentModeKHRToString(presentMode));
    if (crashLine != 0)
        crashHandler_crash_graceful(
            CRASH_LOCATION_LINE(crashLine),
            "The program cannot continue without a Vulkan surface presentation mode.");
    return presentMode;
}

VkSurfaceCapabilitiesKHR window_surfaceCapabilities_get(const Context_t *pCONTEXT, const Window_t *pWINDOW)
{
    VkSurfaceCapabilitiesKHR capabilities = {0};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pCONTEXT->physicalDevice, pWINDOW->surface, &capabilities) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to query Vulkan physical device's surface capabilities!");
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without having the Vulkan device's surface capabilities for presentation");
        return capabilities;
    }

    return capabilities;
}

/// @brief Destroys the Vulkan surface associated with the GLFW window
static void surface_destroy(State_t *pState)
{
    vkDestroySurfaceKHR(pState->context.instance, pState->window.surface, pState->context.pAllocator);
}
#pragma endregion
#pragma region Create Surface
/// @brief Creates the GLFW window surface
static bool surface_create(State_t *pState)
{
    // surface is just a cross-platform abstraction of the window (helps with vulkan)
    if (glfwCreateWindowSurface(pState->context.instance, pState->window.pWindow, pState->context.pAllocator,
                                &pState->window.surface) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to create the Vulkan window surface!");
        return false;
    }

    return true;
}
#pragma endregion
#pragma region Create Window
static void dimensions_set(State_t *pState)
{
    // Vulkan => no api. OpenGL would require the OpenGL api
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, pState->config.windowResizable);

    int width = pState->config.windowWidth;
    int height = pState->config.windowHeight;
    GLFWmonitor *pMonitor = NULL;

    pState->window.fullscreen = pState->config.windowFullscreen;
    if (pState->window.fullscreen)
    {
        // If the window is fullscreen, overwrite the window's resolution to the monitor's
        pMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *pMODE = glfwGetVideoMode(pMonitor);
        width = pMODE->width;
        height = pMODE->height;
    }

    pState->window.pWindow = glfwCreateWindow(width, height, pState->config.pWINDOW_TITLE, pMonitor, NULL);

    int frameBufferWidth;
    int frameBufferHeight;
    glfwGetFramebufferSize(pState->window.pWindow, &frameBufferWidth, &frameBufferHeight);
    pState->window.frameBufferWidth = frameBufferWidth;
    pState->window.frameBufferHeight = frameBufferHeight;
}

/// @brief Callback function for when the window gains or loses focus
static void focusToggleCallback(GLFWwindow *pWindow, int focused)
{
    State_t *pState = glfwGetWindowUserPointer(pWindow);

    if (focused == GLFW_TRUE)
    {
        logs_log(LOG_DEBUG, "Window gained focus");
        if (pState->gui.menuDepth == 0)
            gui_toggleCursorCapture(pState, true);
    }
    else
    {
        logs_log(LOG_DEBUG, "Window lost focus");
        if (pState->gui.menuDepth == 0)
        {
            input_inputAction_simulate(pState, INPUT_ACTION_MENU_TOGGLE, CTX_INPUT_ACTION_START);
            // Freeing cursor immediately is better UX design
            gui_toggleCursorCapture(pState, false);
        }
    }
}

static void callbacks_set(State_t *pState)
{
    // Must be after mouse so that mouse capture can be adequately set during program launch. This is because the program could be launched
    // and then immediately lose focus causing the mouse to be "stolen" from whatever actually does have focus
    glfwSetWindowFocusCallback(pState->window.pWindow, focusToggleCallback);

    // If the window changes size, call this function. There is a window-specific one, but the frame buffer one is better.
    // This allows for supporting retina displays and other screens that use subpixels (Vulkan sees subpixels as normal pixels).
    // For those types of displays, the window width/height and the frame buffer size would be different numbers. Also consider
    // that if the user has two monitors with only one being a retina display, they could drag the window from one screen to another
    // which would change the frame buffer size but NOT the actual window dimensions.
    glfwSetFramebufferSizeCallback(pState->window.pWindow, glfwInstance_framebuffer_sizeCallback);
}
#pragma endregion
#pragma region Const/Dest-ructor
void window_init(State_t *pState)
{
    dimensions_set(pState);

    // This allows for the glfw window to keep a reference to the state. Thus, we don't have to make state a global variable.
    // This is necessary for things such as callback functions (see below in this method) where the callback function
    // otherwise wouldn't have access to the state.
    glfwSetWindowUserPointer(pState->window.pWindow, pState);

    // Must be after the window user pointer is set so that the mouse callback can access state
    mouse_init(pState);

    callbacks_set(pState);

    if (!surface_create(pState))
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a surface to display to.");
}

void window_destroy(State_t *pState)
{
    swapchain_destroy(pState);
    surface_destroy(pState);
    glfwDestroyWindow(pState->window.pWindow);
}
#pragma endregion