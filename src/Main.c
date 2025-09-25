#include <stdio.h>
#include <signal.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "Logger.h"

#define PROGRAM_NAME "VoxelC"

// If error is anything but zero, log that
#define PANIC(ERROR, FORMAT, ...)                                                                                             \
    {                                                                                                                         \
        if (ERROR)                                                                                                            \
        {                                                                                                                     \
            fprintf(stderr, "%s -> %s -> %i -> Error(%i):\n\t" FORMAT, __FILE__, __FUNCTION__, __LINE__, ERROR, __VA_ARGS__); \
            raise(SIGABRT);                                                                                                    \
        }                                                                                                                      \
    }

int main(void)
{
    
    // 10 min https://www.youtube.com/watch?v=wSpwuX3c27Y&list=PLlKj-4rp1Gz0eBLIcq2wzd8uigFrJduJ-

    logger(LOG_INFO, "Starting %s...\n", PROGRAM_NAME);

    PANIC(1, "Hello world %d", 1);

    return 0;
}