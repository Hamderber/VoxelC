#pragma once

/// @brief If error is anything but zero, log that (in red). Include originating filepath,
/// function name, line number, and the passed error. Optional additional variable args.
/// Throw a signal abort to notify the debugger as well. Both vulkan and glfw "error code"
/// 0 means "success" / "not an error" which is convenient. Must be sure to explicitly cast
/// the ERROR to an int errorCode for proper handling. Because this is a macro, the preprocessor
/// would just copy-paste the actual function call itself into the if statement AND the fprintf
/// otherwise. So if a funciton errors, it would actually be called TWICE if just ERROR was used
/// in the if and the fprintf. It must be "macroErrorCode" instead of "errorCode" because functions that
/// the preprocessor will paste this into may already define an "errorCode" variable, which would
/// be overshadowed by this one.
#define LOG_IF_ERROR(ERROR, FORMAT, ...)                                                \
    {                                                                                   \
        int macroErrorCode = ERROR;                                                     \
        if (macroErrorCode)                                                             \
        {                                                                               \
            fprintf(stderr, "\n\033[31m%s -> %s -> %i -> Error(%i):\033[0m\n\t" FORMAT, \
                    __FILE__, __func__, __LINE__, macroErrorCode, ##__VA_ARGS__);       \
            raise(SIGABRT);                                                             \
        }                                                                               \
    }

// Logging
typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG,
    LOG_PHYSICS,
} LogLevel_t;

static const char *LogLevelStrs[] = {
    "Info",
    "Warn",
    "Eror",
    "Dbug",
    "Phys",
};

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

void logVulkanInfo(void)
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