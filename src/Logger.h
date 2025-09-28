#pragma once

typedef enum
{
    LOG_INFO,
    LOG_WARN
} LogLevel_t;

/// @brief If error is anything but zero, log that (in red). Include originating filepath,
/// function name, line number, and the passed error. Optional additional variable args.
/// Throw a signal abort to notify the debugger as well. Both vulkan and glfw "error code"
/// 0 means "success" / "not an error" which is convenient.
#define LOG_IF_ERROR(ERROR, FORMAT, ...)                                                \
    {                                                                                   \
        if (ERROR)                                                                      \
        {                                                                               \
            fprintf(stderr, "\n\033[31m%s -> %s -> %i -> Error(%i):\033[0m\n\t" FORMAT, \
                    __FILE__, __FUNCTION__, __LINE__, ERROR, ##__VA_ARGS__);            \
            raise(SIGABRT);                                                             \
        }                                                                               \
    }

void logger(LogLevel_t level, const char *format, ...);
