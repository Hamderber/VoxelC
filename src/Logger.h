#pragma once

typedef enum
{
    LOG_INFO,
    LOG_WARN
} LogLevel_t;

/// @brief If error is anything but zero, log that (in red). Include originating filepath,
/// function name, line number, and the passed error. Optional additional variable args.
/// Throw a signal abort to notify the debugger as well. Both vulkan and glfw "error code"
/// 0 means "success" / "not an error" which is convenient. Must be sure to explicitly cast
/// the ERROR to an int errorCode for proper handling. Because this is a macro, the preprocessor
/// would just copy-paste the actual function call itself into the if statement AND the fprintf
/// otherwise. So if a funciton errors, it would actually be called TWICE if just ERROR was used
/// in the if and the fprintf. It must be "errCode" instead of "errorCode" because functions that
/// the preprocessor will paste this into may already define an "errorCode" variable, which would
/// be overshadowed by this one.
#define LOG_IF_ERROR(ERROR, FORMAT, ...)                                                \
    {                                                                                   \
        int errCode;                                                                    \
        if ((errCode = ERROR))                                                          \
        {                                                                               \
            fprintf(stderr, "\n\033[31m%s -> %s -> %i -> Error(%i):\033[0m\n\t" FORMAT, \
                    __FILE__, __FUNCTION__, __LINE__, errCode, ##__VA_ARGS__);          \
            raise(SIGABRT);                                                             \
        }                                                                               \
    }

void logger(LogLevel_t level, const char *format, ...);
