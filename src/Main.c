#include <stdlib.h>
#include <stdio.h>
#include <vulkan/vulkan.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "Logger.h"

#define PROGRAM_NAME "VoxelC"

typedef struct
{
    const char *applicationName;
    const char *engineName;
    const char *windowTitle;
    int windowWidth, windowHeight;
    bool windowResizable;
    bool windowFullscreen;

    GLFWmonitor *monitor;
    GLFWwindow *window;

    uint32_t vkAPIVersion;
    VkAllocationCallbacks *allocator;
    VkInstance instance;
} State;

void glfwErrorCallback(int errorCode, const char *description)
{
    LOG_ERROR(errorCode, "GLFW: %s", description);
}

void exitCallback(void)
{
    /* While the OS will handle the garbage collection/etc of all code at program termination anyway, a safe-exit
       isn't necessary. But because glfw is a windowing library, it could've potentially altered system settings
       for rendering/etc. It is very important to safely terminate glfw before fully exiting the program. */
    glfwTerminate();
}

void setupErrorHandling()
{
    // Set function that is called when a glfw error is caught
    glfwSetErrorCallback(glfwErrorCallback);
    // Set function to be called at program termination
    atexit(exitCallback);
}

void createWindow(State *state)
{
    // Vulkan => no api. OpenGL would require the OpenGL api
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, state->windowResizable);

    state->monitor = state->windowFullscreen ? glfwGetPrimaryMonitor() : NULL;

    state->window = glfwCreateWindow(state->windowWidth, state->windowHeight, state->windowTitle,
                                     state->monitor, NULL);
}

void createInstance(State *state)
{
    uint32_t requiredExtensionsCount;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    const VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = state->vkAPIVersion,
        .pApplicationName = state->applicationName,
        .pEngineName = state->engineName};

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = requiredExtensionsCount,
        .ppEnabledExtensionNames = requiredExtensions};

    LOG_ERROR(vkCreateInstance(&createInfo, state->allocator, &state->instance), "Couldn't create Vulkan instance.");
}

void logInfo()
{
    uint32_t instanceAPIVersion;
    LOG_ERROR(vkEnumerateInstanceVersion(&instanceAPIVersion), "Failed to determine Vulkan instance version.");

    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceAPIVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceAPIVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceAPIVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceAPIVersion);

    logger(LOG_INFO, "Vulkan API %i.%i.%i.%i", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
    logger(LOG_INFO, "GLWF %s", glfwGetVersionString());
}

void init(State *state)
{
    logger(LOG_INFO, "Starting %s...", PROGRAM_NAME);
    logInfo();

    // Must init glfw first so that we can actually assign its error handler
    glfwInit();
    setupErrorHandling();

    createInstance(state);
    createWindow(state);
}

void loop(State *state)
{
    while (!glfwWindowShouldClose(state->window))
    {
        // Handle the window events, including actually closing the window with the X
        glfwPollEvents();
    }
}

void cleanup(State *state)
{
    // Order matters here because of potential physical device and interdependency. createInstance() is
    // called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out
    glfwDestroyWindow(state->window);
    vkDestroyInstance(state->instance, state->allocator);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    state->window = NULL;
    state->instance = NULL;
    state->allocator = NULL;

    logger(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}

int main(void)
{
    // 9 min https://www.youtube.com/watch?v=WxZ_YBqyRLI&list=PLlKj-4rp1Gz0eBLIcq2wzd8uigFrJduJ-&index=2

    State state = {
        .applicationName = "VoxelC Application",
        .engineName = "VoxelC Engine",
        .windowTitle = "VoxelC",
        .windowWidth = 720,
        .windowHeight = 480,
        .windowResizable = false,
        .windowFullscreen = false,
        .vkAPIVersion = VK_API_VERSION_1_4};

    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}