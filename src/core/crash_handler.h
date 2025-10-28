#pragma once

#include <stdio.h>
#include <threads.h>

/// @brief Formats the CRASH_LOCATION macro for use as a *char
static inline const char *crash_location_fmt(const char *pFILE, const char *pFUNC)
{
    // One buffer per thread to be printf-safe across threads
    static _Thread_local char buf[256];
    (void)snprintf(buf, sizeof buf, "%s at %s:", pFILE, pFUNC);
    return buf;
}

/// @brief Formats the CRASH_LOCATION_LINE macro for use as a *char
static inline const char *crash_location_fmt_line(const char *pFILE, const char *pFUNC, int line)
{
    static _Thread_local char buf[256];
    (void)snprintf(buf, sizeof buf, "%s at %s at line %d", pFILE, pFUNC, line);
    return buf;
}

/// @brief Use this when there is only one crash option in the function
#define CRASH_LOCATION crash_location_fmt(logs_pathAfterSrc(__FILE__), __func__)
/// @brief Use this when there are multiple crash options in the function
#define CRASH_LOCATION_LINE(line) crash_location_fmt_line(logs_pathAfterSrc(__FILE__), __func__, (int)line)

void crashHandler_crash_graceful(const char *pLOCATION, const char *pCONTEXT);