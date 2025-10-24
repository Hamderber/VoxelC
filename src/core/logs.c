#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include "logs.h"
#include "fileIO.h"

static FILE *s_pLogFile = NULL;
static char s_pLogName[MAX_FILE_NAME_LENGTH];

/// @brief Returns pointer with chars starting at the slash before the last occurence of 'src' Ex: .\src\foo\bar
/// @return const *char
static const char *logs_pathAfterSrc(const char *pFULL_PATH)
{
    if (!pFULL_PATH)
        return "(unknown)";

    const char *pMATCH = NULL;
    const char *pPTR = pFULL_PATH;

    // Find the last occurrence of "/src/" or "\src\"
    while (*pPTR)
    {
        // Walk the string and check if the *section matches the char criteria
        if ((pPTR[0] == '/' || pPTR[0] == '\\') &&
            (tolower((unsigned char)pPTR[1]) == 's') &&
            (tolower((unsigned char)pPTR[2]) == 'r') &&
            (tolower((unsigned char)pPTR[3]) == 'c') &&
            (pPTR[4] == '/' || pPTR[4] == '\\'))
        {
            // Skip leading slash before "src"
            pMATCH = pPTR + 1;
        }

        ++pPTR;
    }

    // If found, return pointer to the slash
    if (pMATCH)
        return pMATCH;

    // Otherwise, fall back to basename (just filename)
    const char *pSLASH_FWD = strrchr(pFULL_PATH, '/');
    const char *pSLASH_BACK = strrchr(pFULL_PATH, '\\');
    const char *pBASE = (pSLASH_FWD && pSLASH_BACK) ? (pSLASH_FWD > pSLASH_BACK ? pSLASH_FWD : pSLASH_BACK)
                                                    : (pSLASH_FWD ? pSLASH_FWD : pSLASH_BACK);
    return pBASE ? pBASE : pFULL_PATH;
}

char *logs_timestampGet(bool isForFileName)
{
    static char s_buffer[32];
    time_t t = time(NULL);
    struct tm *currentTime = localtime(&t);

    if (isForFileName)
    {
        snprintf(s_buffer, sizeof(s_buffer),
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
        snprintf(s_buffer, sizeof(s_buffer),
                 "%04d-%02d-%02d %02d:%02d:%02d",
                 currentTime->tm_year + 1900,
                 currentTime->tm_mon + 1,
                 currentTime->tm_mday,
                 currentTime->tm_hour,
                 currentTime->tm_min,
                 currentTime->tm_sec);
    }

    return s_buffer;
}

void logs_log(LogLevel_t level, const char *pFORMAT, ...)
{
    // log() is built-in don't use it
    const char *pPREFIX = "Info";
    switch (level)
    {
    case LOG_INFO:
        pPREFIX = "Info";
        break;
    case LOG_WARN:
        pPREFIX = "Warn";
        break;
    case LOG_ERROR:
        pPREFIX = "Eror";
        break;
    case LOG_PHYSICS:
        pPREFIX = "Phys";
        break;
    case LOG_UNIT_TEST:
#ifdef NDEBUG
        // Skip when not in debug mode
        return;
#endif
        pPREFIX = "Test";
        break;
    case LOG_DEBUG:
#ifdef NDEBUG
        return;
#endif
        pPREFIX = "Dbug";
        break;
    }

    va_list args;
    va_start(args, pFORMAT);

    // Print to file if open otherwise console
    FILE *pTarget = s_pLogFile ? s_pLogFile : stdout;

    const bool IS_FOR_FILE = false;
    fprintf(pTarget, "[%s][%s] ", pPREFIX, logs_timestampGet(IS_FOR_FILE));
    vfprintf(pTarget, pFORMAT, args);
    fprintf(pTarget, "\n");
    fflush(pTarget);

    va_end(args);
}

/// @brief Logs the version that the app was compiled at. DEBUG | RELEASE | UNSUPPORTED (default)
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

bool logs_logIfError_impl(const char *pFILE, const char *pFUNC, int line, int errorCode, const char *pFORMAT, ...)
{
    if (errorCode)
    {
        char buffer[MAX_DIR_PATH_LENGTH];

        const char *pSHORT_FILE = logs_pathAfterSrc(pFILE);

#ifdef _WIN32
        const char *pPREFIX = ".\\";
#else
        const char *pPREFIX = "./";
#endif

        int prefixLen = snprintf(buffer, sizeof(buffer),
                                 "%s%s -> %s -> %i -> Error(%i): ",
                                 pPREFIX, pSHORT_FILE, pFUNC, line, errorCode);

        if (pFORMAT && *pFORMAT)
        {
            va_list args;
            va_start(args, pFORMAT);
            vsnprintf(buffer + prefixLen, sizeof(buffer) - prefixLen, pFORMAT, args);
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

void logs_create(char *pProgramName)
{
    snprintf(s_pLogName, MAX_FILE_NAME_LENGTH, "%s_%s.log", pProgramName, logs_timestampGet(true));

    if (fileIO_file_create(&s_pLogFile, "logs", s_pLogName) != FILE_IO_RESULT_FILE_CREATED)
    {
        logs_log(LOG_ERROR, "Failed to open log file '%s'", s_pLogName);
        return;
    }

    logs_log(LOG_INFO, "Log file created: '%s'", s_pLogName);

    fflush(s_pLogFile);

    logs_appCompileVersion();
}

void logs_destroy(void)
{
    if (s_pLogFile)
    {
        fileIO_file_close(s_pLogFile, s_pLogName);
        s_pLogFile = NULL;
    }
}
