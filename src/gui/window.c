#include <stdbool.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "core/glfw_instance.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"
#include "gui/swapchain.h"
#include "gui/mouse.h"
#include "events/eventBus.h"
#include "input/input.h"
#include "gui/guiController.h"

void win_fullscreenToggle(State_t *state, bool toggle)
{
    state->window.fullscreen = toggle;

    GLFWwindow *window = state->window.pWindow;
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    if (state->window.fullscreen)
    {
        // Preserve aspect before fullscreen
        glfwGetWindowPos(window, &state->window.prevX, &state->window.prevY);
        glfwGetWindowSize(window, &state->window.widthPrev, &state->window.heightPrev);

        // Set fullscreen (exclusive)
        glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, GLFW_DONT_CARE);

        logs_log(LOG_INFO, "Entered fullscreen (%dx%d@%dHz)", mode->width, mode->height, mode->refreshRate);

        state->window.fullscreen = true;
    }
    else
    {
        // Restore previous windowed mode
        glfwSetWindowMonitor(window, NULL, state->window.prevX, state->window.prevY,
                             state->window.widthPrev, state->window.heightPrev, GLFW_DONT_CARE);

        logs_log(LOG_INFO, "Exited fullscreen, returning to %dx%d", state->window.widthPrev, state->window.heightPrev);
    }
}

/// @brief Wrapper for polling GLFW events
/// @param void
void win_pollEvents(void)
{
    glfwPollEvents();
}

/// @brief Gets the should-window-close flag for the state's GLFW window (thread unsafe)
/// @param state
/// @return
bool win_shouldClose(Window_t *window)
{
    return glfwWindowShouldClose(window->pWindow);
}

/// @brief Creates the GLFW window surface
/// @param state
static void win_surfaceCreate(State_t *state)
{
    // surface is just a cross-platform abstraction of the window (helps with vulkan)
    logs_logIfError(glfwCreateWindowSurface(state->context.instance, state->window.pWindow, state->context.pAllocator,
                                            &state->window.surface),
                    "Unable to create Vulkan window surface");
}

/// @brief Destroys the Vulkan surface associated with the GLFW window
/// @param state
static void win_surfaceDestroy(State_t *state)
{
    vkDestroySurfaceKHR(state->context.instance, state->window.surface, state->context.pAllocator);
}

/// @brief Gets Vulkan window surface capabilities
/// @param context
/// @param window
/// @return VkSurfaceCapabilitiesKHR
VkSurfaceCapabilitiesKHR win_surfaceCapabilitiesGet(const Context_t *context, const Window_t *window)
{
    VkSurfaceCapabilitiesKHR capabilities;
    logs_logIfError(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, window->surface, &capabilities),
                    "Failed to query physical device surface capabilities.");

    return capabilities;
}

/// @brief Selects the best surface format from the available options
/// @param context
/// @param window
/// @return VkSurfaceFormatKHR
VkSurfaceFormatKHR win_surfaceFormatsSelect(const Context_t *context, const Window_t *window)
{
    uint32_t formatCount;
    // null so that we just get the number of formats
    logs_logIfError(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, window->surface, &formatCount, NULL),
                    "Failed to query physical device surface format count.");
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    logs_logIfError(formats == NULL,
                    "Unable to allocate memory for Vulkan surface formats");
    logs_logIfError(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, window->surface, &formatCount, formats),
                    "Failed to query physical device surface formats.");

    VkSurfaceFormatKHR format = formats[0]; // Default to the first format ...
    for (uint32_t i = 0; i < formatCount; i++)
    {
        // SRGB is the most commonly supported (and best) so we want that one if available
        // B8G8R8A8 = 8-bit blue/green/red/alpha components (so a 32-bit color depth)
        if (formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            format = formats[i]; // ... Unless the preferred format is found.
            break;
        }
    }

    free(formats);

    return format;
}

/// @brief Selects the preferred surface present mode
/// @param context
/// @param window
/// @return VkPresentModeKHR
VkPresentModeKHR win_surfacePresentModesSelect(const AppConfig_t *config, const Context_t *context, const Window_t *window)
{
    // If vsync is disabled, don't bother enumerating the available options. Immediate is always supported and will
    // throw as many frames as possible despite monitor refresh rate. This WILL cause screen tearing.
    if (!config->vsync)
    {
        return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    // See https://www.youtube.com/watch?v=nSzQcyQTtRY for the different present modes (visual examples)
    // Immedaite causes screen tearing (don't use) and I think Mailbox sounds the best. Unfortunately,
    // Mailbox isn't universally supported and is much more power-intensive (constant frame generation/discarding)
    // FIFO is required to exist on all platforms. All others are potentially not :(
    uint32_t presentModeCount;
    // null so that we just get the number of present modes
    logs_logIfError(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice, window->surface, &presentModeCount, NULL),
                    "Failed to query physical device surface presentation modes.");
    VkPresentModeKHR *presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    logs_logIfError(presentModes == NULL,
                    "Unable to allocate memory for Vulkan present modes.");
    logs_logIfError(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice, window->surface, &presentModeCount, presentModes),
                    "Failed to query physical device surface presentation modes.");

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Default to FIFO ...
    for (uint32_t i = 0; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = presentModes[i]; // ... Unless Mailbox is available
            break;
        }
    }

    free(presentModes);

    return presentMode;
}

/// @brief Intentionally hangs while the window is minimized
/// @param state
void win_waitForValidFramebuffer(Window_t *window)
{
    // Vulkan errors if the window is minimized and it tries to create a 0x0 size swapchain
    int width = 0, height = 0;

    // Block until the window is restored (not minimized)
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window->pWindow, &width, &height);
        // sleep until resize/restored event occurs
        glfwWaitEvents();
    }

    window->frameBufferWidth = width;
    window->frameBufferHeight = height;
}

/// @brief Callback function for when the window gains or loses focus
void win_focusToggleCallback(GLFWwindow *window, int focused)
{
    State_t *state = glfwGetWindowUserPointer(window);

    if (focused == GLFW_TRUE)
    {
        logs_log(LOG_INFO, "Window gained focus");
        if (state->gui.menuDepth == 0)
            gui_toggleCursorCapture(state, true);
    }
    else
    {
        logs_log(LOG_INFO, "Window lost focus");
        if (state->gui.menuDepth == 0)
        {
            input_inputActionSimulate(state, INPUT_ACTION_MENU_TOGGLE, CTX_INPUT_ACTION_START);
            // Freeing cursor immediately is better UX design
            gui_toggleCursorCapture(state, false);
        }
    }
}

/// @brief Creates the GLFW window
/// @param state
void win_create(State_t *state)
{
    // Vulkan => no api. OpenGL would require the OpenGL api
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, state->config.windowResizable);

    int width = state->config.windowWidth;
    int height = state->config.windowHeight;
    // There is no need to store the actual monitor reference. Just the window.
    GLFWmonitor *monitor = NULL;

    // Copy fullscreen from config to window
    state->window.fullscreen = state->config.windowFullscreen;
    if (state->window.fullscreen)
    {
        monitor = glfwGetPrimaryMonitor();
        // If the window is fullscreen, set the window's resolution to the monitor's
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
    }

    // If not fullscreen, set window resolution to the default state values (set in main())
    state->window.pWindow = glfwCreateWindow(width, height, state->config.pWINDOW_TITLE, monitor, NULL);

    int frameBufferWidth;
    int frameBufferHeight;
    glfwGetFramebufferSize(state->window.pWindow, &frameBufferWidth, &frameBufferHeight);
    state->window.frameBufferWidth = frameBufferWidth;
    state->window.frameBufferHeight = frameBufferHeight;

    // This allows for the glfw window to keep a reference to the state. Thus, we don't have to make state a global variable.
    // This is necessary for things such as callback functions (see below in this method) where the callback function
    // otherwise wouldn't have access to the state.
    glfwSetWindowUserPointer(state->window.pWindow, state);

    // Must be after the window user pointer is set so that the mouse callback can access state
    mouse_init(state);

    // Must be after mouse so that mouse capture can be adequately set during program launch. This is because the program could be launched
    // and then immediately lose focus causing the mouse to be "stolen" from whatever actually does have focus
    glfwSetWindowFocusCallback(state->window.pWindow, win_focusToggleCallback);

    // If the window changes size, call this function. There is a window-specific one, but the frame buffer one is better.
    // This allows for supporting retina displays and other screens that use subpixels (Vulkan sees subpixels as normal pixels).
    // For those types of displays, the window width/height and the frame buffer size would be different numbers. Also consider
    // that if the user has two monitors with only one being a retina display, they could drag the window from one screen to another
    // which would change the frame buffer size but NOT the actual window dimensions.
    glfwSetFramebufferSizeCallback(state->window.pWindow, glfwInstance_framebuffer_sizeCallback);

    win_surfaceCreate(state);
    sc_create(state);
}

/// @brief Destroys the window, surface, and swapchain
/// @param state
void win_destroy(State_t *state)
{
    sc_destroy(state);
    win_surfaceDestroy(state);
    glfwDestroyWindow(state->window.pWindow);
}