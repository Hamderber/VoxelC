#include "Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

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

    printf("\n");
}
