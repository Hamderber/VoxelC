#pragma once

#include <stdbool.h>

/// @brief LOG_INFO | WARN | ERROR | DEBUG | UNIT TEST | PHYSICS
typedef enum LogLevel_e
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_CRASH,
    LOG_DEBUG,
    LOG_UNIT_TEST,
    LOG_PHYSICS,
} LogLevel_e;

/// @brief ISO 8601-Style Timestamp minus time zone and miliseconds
/// @param  isForFileName If the timestamp should include ':' (invalid for file names)
/// @return *char[32]
char *logs_timestampGet(bool isForFileName);

/// @brief Returns pointer with chars starting at the slash before the last occurence of 'src' Ex: .\src\foo\bar
const char *logs_pathAfterSrc(const char *pFULL_PATH);

/// @brief Writes to the current .log file (or console if not available) the passed log level and formatted string.
/// Debug/Unit test is only written when compiled in DEBUG mode.
void logs_log(LogLevel_e level, const char *format, ...);

/// @brief If the recieved func returns something that casts to a 0 then it is 'success.'
/// Otherwise log the location the function was called and the passed formatted string for debugging context.
/// If compiled to DEBUG, raises an abort signal on ANY non-zero return function. This will force full debugging.
/// @return bool if the passed function returned (int)0
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
#define logs_logIfError(errorCode, format, ...)                                               \
    do                                                                                        \
    {                                                                                         \
        logs_logIfError_impl(__FILE__, __func__, __LINE__, errorCode, format, ##__VA_ARGS__); \
    } while (0)

/// @brief Makes a .log file with the program name and initial timestamp. If the logs folder doesn't exist, create it. All
/// future logs_... functions will write to either the .log or console depending on if that file exists.
void logs_create(char *programName);

/// @brief Closes the open .log file but DOES NOT destroy the .log file.
void logs_destroy(void);