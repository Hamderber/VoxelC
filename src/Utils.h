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

static const char *LogLevelStrs[] = {
    "Info",
    "Warn"};

/// @brief Logs the passed formatted str to the console with the given level.
/// @param level
/// @param format
/// @param
void logger(LogLevel_t level, const char *format, ...) // log() is built-in don't use it
{
    time_t t = time(NULL);
    struct tm currentTime = *localtime(&t);

    // enum can be directly used as array index. Need to add the time of the epoc (Jan 1 1900) to the current time
    // ISO 8601 Timestamp minus time zone and miliseconds
    // https://stackoverflow.com/questions/1442116/how-can-i-get-the-date-and-time-values-in-a-c-program
    // https://en.wikipedia.org/wiki/ISO_8601
    printf("[%s][%d-%02d-%02dT%02d:%02d:%02d] ", LogLevelStrs[level], currentTime.tm_year + 1900, currentTime.tm_mon + 1, currentTime.tm_mday, currentTime.tm_hour, currentTime.tm_min, currentTime.tm_sec);

    va_list args;
    va_start(args, format);

    vprintf(format, args); // prints formatted string

    va_end(args);

    // By including the trailing newline, the output stream is flushed which allows the printed line to show up immediately.
    // Otherwise, the reader would have to wait until program exit or something else is printed.
    printf("\n");
}

void logVulkanInfo()
{
    uint32_t instanceAPIVersion;
    LOG_IF_ERROR(vkEnumerateInstanceVersion(&instanceAPIVersion),
                 "Failed to determine Vulkan instance version.")

    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceAPIVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceAPIVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceAPIVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceAPIVersion);

    logger(LOG_INFO, "Vulkan API %i.%i.%i.%i", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
    logger(LOG_INFO, "GLWF %s", glfwGetVersionString());
}

void logCapabilitiesInfo(const VkSurfaceCapabilitiesKHR capabilities)
{
    logger(LOG_INFO, "The physical device has the following features:");
    logger(LOG_INFO, "\t\tImage count range: [%d-%d]", capabilities.minImageCount, capabilities.maxImageCount);
    logger(LOG_INFO, "\t\tMax Image Array Layers: %d", capabilities.maxImageArrayLayers);
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
