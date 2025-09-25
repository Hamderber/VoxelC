#pragma once

typedef enum
{
    LOG_INFO,
    LOG_WARN
} LogLevel_t;

void logger(LogLevel_t level, const char *format, ...);
void loggerError(ERROR, FORMAT, ...);
