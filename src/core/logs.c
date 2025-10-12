#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "logs.h"

// OS-specific inclusions for file/directory writing
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#define LOGS_FILE_SHORT \
    (logs_pathAfterSrc(__FILE__)[0] == '\\' ? logs_pathAfterSrc(__FILE__) - 1 : logs_pathAfterSrc(__FILE__))
#define LOGS_FILE_PREFIX ".\\"
#else
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#define LOGS_FILE_SHORT logs_pathAfterSrc(__FILE__)
#define LOGS_FILE_PREFIX "./"
#endif

static FILE *gpLogFile = NULL;

/// @brief Returns pointer with chars starting at the slash before the last occurence of 'src' Ex: .\src\foo\bar
/// @param fullPath
/// @return const *char
static const char *logs_pathAfterSrc(const char *fullPath)
{
    if (!fullPath)
        return "(unknown)";

    const char *match = NULL;
    const char *p = fullPath;

    // Find the last occurrence of "/src/" or "\src\"
    while (*p)
    {
        if ((p[0] == '/' || p[0] == '\\') &&
            (tolower((unsigned char)p[1]) == 's') &&
            (tolower((unsigned char)p[2]) == 'r') &&
            (tolower((unsigned char)p[3]) == 'c') &&
            (p[4] == '/' || p[4] == '\\'))
        {
            match = p + 1; // skip leading slash before "src"
        }
        ++p;
    }

    // If found, return pointer to the slash
    if (match)
        return match;

    // Otherwise, fall back to basename (just filename)
    const char *slashFwd = strrchr(fullPath, '/');
    const char *slashBack = strrchr(fullPath, '\\');
    const char *base = (slashFwd && slashBack) ? (slashFwd > slashBack ? slashFwd : slashBack)
                                               : (slashFwd ? slashFwd : slashBack);
    return base ? base : fullPath;
}

/// @brief ISO 8601-Style Timestamp minus time zone and miliseconds
/// @param  isForFileName If the timestamp should include ':' (invalid for file names)
/// @return static *char[32]
static char *logs_timestampGet(bool isForFileName)
{
    static char buffer[32];
    time_t t = time(NULL);
    struct tm *currentTime = localtime(&t);

    if (isForFileName)
    {
        snprintf(buffer, sizeof(buffer),
                 "%04d-%02d-%02dT%02dh%02dm%02ds",
                 currentTime->tm_year + 1900,
                 currentTime->tm_mon + 1,
                 currentTime->tm_mday,
                 currentTime->tm_hour,
                 currentTime->tm_min,
                 currentTime->tm_sec);
    }
    else
    {
        snprintf(buffer, sizeof(buffer),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 currentTime->tm_year + 1900,
                 currentTime->tm_mon + 1,
                 currentTime->tm_mday,
                 currentTime->tm_hour,
                 currentTime->tm_min,
                 currentTime->tm_sec);
    }

    return buffer;
}

/// @brief Writes to the current .log file (or console if not available) the passed log level and formatted string.
/// Debug/Unit test is only written when compiled in DEBUG mode.
/// @param level
/// @param format
/// @param ...
void logs_log(LogLevel_t level, const char *format, ...) // log() is built-in don't use it
{
    const char *prefix;
    switch (level)
    {
    case LOG_INFO:
        prefix = "Info";
        break;
    case LOG_WARN:
        prefix = "Warn";
        break;
    case LOG_ERROR:
        prefix = "Eror";
        break;
    case LOG_PHYSICS:
        prefix = "Phys";
        break;
    case LOG_UNIT_TEST:
#ifdef NDEBUG
        return;
#endif
        prefix = "Test";
        break;
    case LOG_DEBUG:
#ifdef NDEBUG
        return;
#endif
        prefix = "Dbug";
        break;
    }

    va_list args;
    va_start(args, format);

    // Print to file if open otherwise console
    FILE *target = gpLogFile ? gpLogFile : stdout;

    fprintf(target, "[%s][%s] ", prefix, logs_timestampGet(false));
    vfprintf(target, format, args);
    fprintf(target, "\n");
    fflush(target);

    va_end(args);
}

/// @brief Logs the version that the app was compiled at. DEBUG | RELEASE | UNSUPPORTED (default)
/// @param  void
static void logs_appCompileVersion(void)
{
#if defined(DEBUG)
    logs_log(LOG_INFO, "Running in DEBUG mode.");
#elif defined(NDEBUG)
    logs_log(LOG_INFO, "Running in RELEASE mode.");
#else
    logs_log(LOG_WARN, "Running in an unsupported mode!");
#endif
}

/// @brief If the recieved func returns something that casts to a 0 then it is 'success.'
/// Otherwise log the location the function was called and the passed formatted string for debugging context.
/// If compiled to DEBUG, raises an abort signal on ANY non-zero return function. This will force full debugging.
/// @param file
/// @param func
/// @param line
/// @param errorCode
/// @param format
/// @param  ...
/// @return bool isSuccess (if the passed function returned (int)0
bool logs_logIfError_impl(const char *file, const char *func, int line, int errorCode, const char *format, ...)
{
    if (errorCode)
    {
        char buffer[512];

        const char *shortFile = logs_pathAfterSrc(file);

#ifdef _WIN32
        const char *prefix = ".\\";
#else
        const char *prefix = "./";
#endif

        int prefixLen = snprintf(buffer, sizeof(buffer),
                                 "%s%s -> %s -> %i -> Error(%i): ",
                                 prefix, shortFile, func, line, errorCode);

        if (format && *format)
        {
            va_list args;
            va_start(args, format);
            vsnprintf(buffer + prefixLen, sizeof(buffer) - prefixLen, format, args);
            va_end(args);
        }

        logs_log(LOG_ERROR, "%s", buffer);

#ifdef DEBUG
        raise(SIGABRT);
#endif
        return true;
    }

    return false;
}

/// @brief Makes a .log file with the program name and initial timestamp. If the logs folder doesn't exist, create it. All
/// future logs_... functions will write to either the .log or console depending on if that file exists.
/// Logs to console if this fails.
/// @param char* programName
void logs_create(char *programName)
{
    const char *logDir = "../logs";
    struct stat st = {0};

    if (stat(logDir, &st) == -1)
    {
        if (MKDIR(logDir) != 0)
        {
            logs_log(LOG_ERROR, "Failed to create logs directory at '%s'", logDir);
            return;
        }
    }

    // Build file path inside logs folder
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/%s_%s.log", logDir, programName, logs_timestampGet(true));

#ifdef _WIN32
    // Have to include this to prevent a compiler warning about fpoen being unsafe (Windows has a "safe" version)
    if (fopen_s(&gpLogFile, filename, "w") != 0)
    {
        gpLogFile = NULL;
    }
#else
    gpLogFile = fopen(filename, "w");
#endif

    if (!gpLogFile)
    {
        logs_log(LOG_ERROR, "Failed to open log file '%s'", filename);
        return;
    }

    logs_log(LOG_INFO, "Log file created: '%s'", filename);

    fflush(gpLogFile);

    logs_appCompileVersion();
}

/// @brief Closes the .log and nulls its pointer (if it exists) but DOES NOT destroy the .log file.
/// @param  void
void logs_destroy(void)
{
    if (gpLogFile)
    {
        logs_log(LOG_INFO, "Log file closed.");
        fclose(gpLogFile);
        gpLogFile = NULL;
    }
}
