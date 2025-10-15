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

// OS-specific inclusions for file/directory writing
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define MKDIR(path) _mkdir(path)
#else
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

// Base path always ../ relative to executable (bin)
static const char *pBASE_PATH = "../";

void file_close(FILE *file, const char *debugName)
{
    if (!file)
    {
        logs_log(LOG_ERROR, "Attempted to close invalid file at Address %p (%s)", file, debugName);
        return;
    }

    logs_log(LOG_DEBUG, "Closing file at Address %p (%s)", file, debugName);
    logs_logIfError(fclose(file),
                    "Error while closing file at Address %p (%s)", file, debugName);
}

FILE *file_open(const char *path, const char *mode, const char *debugName)
{
    if (!path || !mode)
    {
        logs_log(LOG_ERROR, "file_open() received invalid parameters (path=%p, mode=%p) [%s]", path, mode, debugName);
        return NULL;
    }

    FILE *file = NULL;

#ifdef _WIN32
    if (fopen_s(&file, path, mode) != 0)
    {
        logs_log(LOG_ERROR, "Failed to open file '%s' (mode='%s') [%s]", path, mode, debugName);
        return NULL;
    }
#else
    file = fopen(path, mode);
    if (!file)
    {
        logs_log(LOG_ERROR, "Failed to open file '%s' (mode='%s') [%s]", path, mode, debugName);
        return NULL;
    }
#endif

    logs_log(LOG_DEBUG, "Opened file '%s' in mode '%s' at Address %p [%s]", path, mode, (void *)file, debugName);

    // Flush immediately for safety in write modes
    if (strchr(mode, 'w') || strchr(mode, 'a'))
        fflush(file);

    return file;
}

/// @brief Checks if ../dirPath/dir/ exists and writes that path to *fullDir regardless
/// @param dir
/// @param dirPath
/// @param fullDir
/// @return bool
bool file_dirExists(const char *dir, char *fullDir)
{
    snprintf(fullDir, MAX_DIR_PATH_LENGTH, "%s%s", pBASE_PATH, dir);

    logs_log(LOG_DEBUG, "Checking if '%s' exists at '%s'", dir, fullDir);

    struct stat st = {0};
    return stat(fullDir, &st) != -1;
}

bool file_exists(const char *dir, const char *fileName, char *fullPath)
{
    if (!dir || !fileName)
    {
        logs_log(LOG_ERROR, "file_fileExists() received invalid parameters (dir=%p, fileName=%p)", dir, fileName);
        return false;
    }

    char fullDir[MAX_DIR_PATH_LENGTH];
    if (!file_dirExists(dir, fullDir))
    {
        logs_log(LOG_WARN, "Directory '%s' not found at '%s' while checking '%s'", dir, fullDir, fileName);
        return false;
    }

    // Build final path
    snprintf(fullPath, MAX_DIR_PATH_LENGTH, "%s/%s", fullDir, fileName);
    logs_log(LOG_DEBUG, "Checking if file '%s' exists at '%s'", fileName, fullPath);

#ifdef _WIN32
    // On Windows, use _access (or _waccess if wide chars)
    return (_access(fullPath, 0) == 0);
#else
    // On POSIX systems, use access() or stat()
    struct stat st = {0};
    return (stat(fullPath, &st) == 0);
#endif
}

/// @brief Creates the directory dir at ../dirPath/dir relative to bin/.exe
/// @param dir
/// @param dirPath
/// @return FILE_IO_RESULT_SUCCESS | FILE_IO_RESULT_FAILURE | FILE_IO_RESULT_DIR_ALREADY_EXISTS
FileIO_Result_t file_dirCreate(const char *dir, char *fullDir)
{
    if (!file_dirExists(dir, fullDir))
    {
        if (MKDIR(fullDir) != 0)
        {
            logs_log(LOG_ERROR, "Failed to create directory '%s'", fullDir);
            return FILE_IO_RESULT_FAILURE;
        }

        logs_log(LOG_DEBUG, "Created directory: '%s'", fullDir);
        return FILE_IO_RESULT_SUCCESS;
    }

    return FILE_IO_RESULT_DIR_ALREADY_EXISTS;
}

FILE *file_create(const char *dir, const char *fileName)
{
    logs_log(LOG_DEBUG, "Checking if '%s' exists before creating %s", dir, fileName);
    char fullDir[MAX_DIR_PATH_LENGTH];

    FileIO_Result_t result = file_dirCreate(dir, fullDir);

    if (result == FILE_IO_RESULT_FAILURE)
    {
        logs_log(LOG_ERROR, "Failed to create directory '%s'. '%s' was not saved.", dir, fileName);
        return NULL;
    }

    // Build final file path: ../dir/fileName
    char fullPath[MAX_DIR_PATH_LENGTH];
    snprintf(fullPath, sizeof(fullPath), "%s/%s", fullDir, fileName);

    FILE *file = NULL;
#ifdef _WIN32
    if (fopen_s(&file, fullPath, "w") != 0)
        file = NULL;
#else
    file = fopen(fullPath, "w");
#endif

    if (!file)
    {
        logs_log(LOG_ERROR, "Failed to open file '%s'", fullPath);
        return NULL;
    }

    logs_log(LOG_DEBUG, "File created: '%s'", fullPath);

    fflush(file);

    return file;
}
