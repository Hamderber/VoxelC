#include <stdio.h>
#include <stdbool.h>

#define MAX_FILE_NAME_LENGTH 256
#define MAX_DIR_PATH_LENGTH 512

/// @brief SUCCESS | DIR/FILE CREATED | FAILURE | DIR/FILE ALREADY EXISTS
typedef enum FileIO_Result_e
{
    FILE_IO_RESULT_SUCCESS = 0,
    FILE_IO_RESULT_DIR_CREATED = 0,
    FILE_IO_RESULT_FILE_CREATED = 0,
    FILE_IO_RESULT_FAILURE = 1,
    FILE_IO_RESULT_DIR_ALREADY_EXISTS = 2,
    FILE_IO_RESULT_FILE_ALREADY_EXISTS = 3,
} FileIO_Result_e;

/// @brief Closes the file, if open.
void fileIO_file_close(FILE *pFile, const char *pDEBUG_NAME);

/// @brief Attempts to open the file at the given path.
/// @return FileIO_Result_e
FileIO_Result_e fileIO_file_open(FILE **ppFile, const char *pPATH, const char *pMODE, const char *pDEBUG_NAME);

/// @brief Checks if the folder exists. Does NOT create it if the folder doesn't exist. Either way, populates the
/// char buffer with the full directory path.
/// @return bool
bool fileIO_dir_exists(const char *pFOLDER_NAME, char *pFullDir);

/// @brief Checks if the file exists. Populates the char buffer with the full file path (directories/../fileName)
/// @return bool
bool fileIO_file_exists(const char *pFOLDER_NAME, const char *pFILE_NAME, char *fullPath);

/// @brief Creates the folder at ../dirPath/folderName relative to bin/.exe and populates the char buffer with the
/// full directory path.
/// @return FileIO_Result_e
FileIO_Result_e fileIO_dir_create(const char *pFOLDER_NAME, char *pFullDir);

/// @brief Creates the file
/// @return FileIO_Result_e
FileIO_Result_e fileIO_file_create(FILE **ppFile, const char *pFOLDER_NAME, const char *pFILE_NAME);