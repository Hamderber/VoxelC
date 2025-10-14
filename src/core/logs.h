#pragma once

#include <stdbool.h>

// LOG_INFO | WARN | ERROR | DEBUG | UNIT TEST | PHYSICS
typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG,
    LOG_UNIT_TEST,
    LOG_PHYSICS,
} LogLevel_t;

void logs_log(LogLevel_t level, const char *format, ...);

bool logs_logIfError_impl(const char *file, const char *func, int line, int errorCode, const char *format, ...);

// This macro acts as a wrapper for logging if the recieved error code is not 0. Vulkan, GLFW, and other libraries often
// associate 'success' with 0. Thus, if a non-0 code is output by the passed func, it will be logged to the current
// .log file. It should be noted that some results (Vulkan specifically) may not be 'errors' but are still =/= 0
// and may be erroneously logged as errors unless screened before this is called. In such a case, using
// logs_log(LOG_ERROR, "...") would be a better implementation.
//
// If the recieved func returns something that casts to a 0 then it is 'success.'
// Otherwise log the location the function was called and the passed formatted string for debugging context.
// If compiled to DEBUG, raises an abort signal on ANY non-zero return function. This will force full debugging.
#define logs_logIfError(errorCode, format, ...) logs_logIfError_impl(__FILE__, __func__, __LINE__, errorCode, format, ##__VA_ARGS__)

void logs_create(char *programName);

void logs_destroy(void);