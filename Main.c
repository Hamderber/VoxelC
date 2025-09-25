#include <stdio.h>
#include "./Debug/Logger.h"

#define PROGRAM_NAME "VoxelC"

int main(void)
{
    logger(LOG_INFO, "Starting %s...\n", PROGRAM_NAME);

    return 0;
}