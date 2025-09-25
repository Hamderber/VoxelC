#include <stdio.h>
#include <vulkan/vulkan.h>
#include <signal.h>
#include <stdarg.h>
#include <GLFW/glfw3.h>
#include "Logger.h"

#define PROGRAM_NAME "VoxelC"

int main(void)
{

    // 10 min https://www.youtube.com/watch?v=wSpwuX3c27Y&list=PLlKj-4rp1Gz0eBLIcq2wzd8uigFrJduJ-

    logger(LOG_INFO, "Starting %s...\n", PROGRAM_NAME);

    LOG_ERROR(2, "Hello world");

    return 0;
}