#include <stdio.h>
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

void fileIO_file_close(FILE *pFile, const char *pDEBUG_NAME)
{
    if (!pFile)
    {
        logs_log(LOG_ERROR, "Attempted to close invalid file at address %p (%s)", pFile, pDEBUG_NAME);
        return;
    }

    logs_log(LOG_DEBUG, "Closing file at address %p (%s)", pFile, pDEBUG_NAME);
    logs_logIfError(fclose(pFile), "Error while closing file at address %p (%s)", pFile, pDEBUG_NAME);
}

FileIO_Result_e fileIO_file_open(FILE **ppFile, const char *pPATH, const char *pMODE, const char *pDEBUG_NAME)
{
    if (!pPATH || !pMODE)
    {
        logs_log(LOG_ERROR, "Received invalid parameters (path=%p, mode=%p) [%s]", pPATH, pMODE, pDEBUG_NAME);
        *ppFile = NULL;
        return FILE_IO_RESULT_FAILURE;
    }

#ifdef _WIN32
    errno_t result = fopen_s(ppFile, pPATH, pMODE);
    if (result != 0)
    {
        logs_log(LOG_ERROR, "Failed to open file '%s' (mode='%s') [%s]", pPATH, pMODE, pDEBUG_NAME);
        return FILE_IO_RESULT_FAILURE;
    }
#else
    *ppFile = fopen(pPATH, pMODE);
    if (!*ppFile)
    {
        logs_log(LOG_ERROR, "Failed to open file '%s' (mode='%s') [%s]", pPATH, pMODE, pDEBUG_NAME);
        return FILE_IO_RESULT_FAILURE;
    }
#endif

    logs_log(LOG_DEBUG, "Opened file '%s' in mode '%s' at address %p [%s]", pPATH, pMODE, (void *)*ppFile, pDEBUG_NAME);

    // Flush immediately for safety in write modes
    if (strchr(pMODE, 'w') || strchr(pMODE, 'a'))
        fflush(*ppFile);

    return FILE_IO_RESULT_SUCCESS;
}

bool fileIO_dir_exists(const char *pFOLDER_NAME, char *pFullDir)
{
    snprintf(pFullDir, MAX_DIR_PATH_LENGTH, "%s%s", pBASE_PATH, pFOLDER_NAME);
    logs_log(LOG_DEBUG, "Checking if '%s' exists at '%s'", pFOLDER_NAME, pFullDir);

    struct stat fileStats = {0};
    return stat(pFullDir, &fileStats) != -1;
}

bool fileIO_file_exists(const char *pFOLDER_NAME, const char *pFILE_NAME, char *pFullPath)
{
    if (!pFOLDER_NAME || !pFILE_NAME)
    {
        logs_log(LOG_ERROR, "Received invalid parameters (dir=%p, fileName=%p)", pFOLDER_NAME, pFILE_NAME);
        return false;
    }

    char pFullDir[MAX_DIR_PATH_LENGTH];
    if (!fileIO_dir_exists(pFOLDER_NAME, pFullDir))
    {
        logs_log(LOG_WARN, "Directory '%s' not found at '%s' while checking '%s'", pFOLDER_NAME, pFullDir, pFILE_NAME);
        return false;
    }

    snprintf(pFullPath, MAX_DIR_PATH_LENGTH, "%s/%s", pFullDir, pFILE_NAME);
    logs_log(LOG_DEBUG, "Checking if file '%s' exists at '%s'", pFILE_NAME, pFullPath);

#ifdef _WIN32
    // Windows
    return (_access(pFullPath, 0) == 0);
#else
    // POSIX systems
    struct stat fileStats = {0};
    return (stat(pFullPath, &fileStats) == 0);
#endif
}

FileIO_Result_e fileIO_dir_create(const char *pFOLDER_NAME, char *pFullDir)
{
    if (!fileIO_dir_exists(pFOLDER_NAME, pFullDir))
    {
        // Make the folder for the current OS platform
        if (MKDIR(pFullDir) != 0)
        {
            logs_log(LOG_ERROR, "Failed to create directory '%s'", pFullDir);
            return FILE_IO_RESULT_FAILURE;
        }

        logs_log(LOG_DEBUG, "Created directory: '%s'", pFullDir);
        return FILE_IO_RESULT_DIR_CREATED;
    }

    return FILE_IO_RESULT_DIR_ALREADY_EXISTS;
}

FileIO_Result_e fileIO_file_create(FILE **ppFile, const char *pFOLDER_NAME, const char *pFILE_NAME)
{
    logs_log(LOG_DEBUG, "Checking if '%s' exists before creating %s", pFOLDER_NAME, pFILE_NAME);

    char pFullDir[MAX_DIR_PATH_LENGTH];
    if (fileIO_dir_create(pFOLDER_NAME, pFullDir) == FILE_IO_RESULT_FAILURE)
    {
        logs_log(LOG_ERROR, "Failed to create directory '%s'. '%s' was not saved.", pFOLDER_NAME, pFILE_NAME);
        *ppFile = NULL;
        return FILE_IO_RESULT_FAILURE;
    }

    // Build final file path: ../folderName/fileName
    char pFullPath[MAX_DIR_PATH_LENGTH];
    snprintf(pFullPath, sizeof(pFullPath), "%s/%s", pFullDir, pFILE_NAME);

#ifdef _WIN32
    if (fopen_s(ppFile, pFullPath, "w") != 0)
        *ppFile = NULL;
#else
    *ppFile = fopen(pFullPath, "w");
#endif

    if (!*ppFile)
    {
        logs_log(LOG_ERROR, "Failed to open file '%s'", pFullPath);
        *ppFile = NULL;
        return FILE_IO_RESULT_FAILURE;
    }

    logs_log(LOG_DEBUG, "File created: '%s'", pFullPath);

    fflush(*ppFile);

    return FILE_IO_RESULT_FILE_CREATED;
}
