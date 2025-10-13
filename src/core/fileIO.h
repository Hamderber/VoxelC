#include <stdio.h>
#include <stdbool.h>

#define MAX_FILE_NAME_LENGTH 256
#define MAX_DIR_PATH_LENGTH 512

typedef enum
{
    FILE_IO_RESULT_SUCCESS = 0,
    FILE_IO_RESULT_FAILURE = 1,
    FILE_IO_RESULT_DIR_ALREADY_EXISTS = 2,
} FileIO_Result_t;

void file_close(FILE *file, const char *debugName);

FILE *file_open(const char *path, const char *mode, const char *debugName);

bool file_dirExists(const char *dir, char *fullDir);

FileIO_Result_t file_dirCreate(const char *dir, char *fullDir);

FILE *file_create(const char *dir, const char *fileName);