#pragma once
#include <stdarg.h>

typedef enum
{
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel_t;

void logger(LogLevel_t level, const char *format, ...);
