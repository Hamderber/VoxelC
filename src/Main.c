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
    /// @brief UINT32_MAX means no family assigned
    uint32_t queueFamily;

    VkAllocationCallbacks *allocator;
    VkInstance instance;

    VkPhysicalDevice physicalDevice;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue queue;
} State_t;

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

void createWindow(State_t *state)
{
    // Vulkan => no api. OpenGL would require the OpenGL api
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, state->windowResizable);

    if (state->windowFullscreen)
    {
        state->monitor = glfwGetPrimaryMonitor();
        // If the window is fullscreen, set the window's resolution to the monitor's
        const GLFWvidmode *mode = glfwGetVideoMode(state->monitor);
        state->windowWidth = mode->width;
        state->windowHeight = mode->height;
    }

    // If not fullscreen, set window resolution to the default state values (set in main())
    state->window = glfwCreateWindow(state->windowWidth, state->windowHeight, state->windowTitle,
                                     state->monitor, NULL);
}

void createInstance(State_t *state)
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

    LOG_ERROR(vkCreateInstance(&createInfo, state->allocator, &state->instance),
              "Couldn't create Vulkan instance.");
}

void logInfo()
{
    uint32_t instanceAPIVersion;
    LOG_ERROR(vkEnumerateInstanceVersion(&instanceAPIVersion),
              "Failed to determine Vulkan instance version.");

    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceAPIVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceAPIVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceAPIVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceAPIVersion);

    logger(LOG_INFO, "Vulkan API %i.%i.%i.%i", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
    logger(LOG_INFO, "GLWF %s", glfwGetVersionString());
}

void selectPhysicalDevice(State_t *state)
{
    uint32_t count;

    LOG_ERROR(vkEnumeratePhysicalDevices(state->instance, &count, NULL),
              "Couldn't enumerate Vulkan-supported physical device count");

    if (count > 0)
    {
        logger(LOG_INFO, "Found %d Vulkan-supported physical device(s):", count);

        VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * count);
        LOG_ERROR(physicalDevices == NULL, "Unable to allocate memory for physical devices");
        LOG_ERROR(vkEnumeratePhysicalDevices(state->instance, &count, physicalDevices),
                  "Couldn't enumerate Vulkan-supported physical devices");

        char *deviceType;
        char *preamble;
        for (uint32_t i = 0; i < count; i++)
        {
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);

            // see line 2072 of vulkan_core.h for enum definitions. Didn't want to include that .h here
            switch (properties.deviceType)
            {
            case (1):
                deviceType = "Integrated GPU";
                break;
            case (2):
                deviceType = "Discrete GPU";
                break;
            case (3):
                deviceType = "Virtual GPU";
                break;
            case (4):
                deviceType = "CPU";
                break;
            default:
                deviceType = "Other";
                break;
            }

            preamble = i == 0 ? "(Selected)\t" : "\t\t";

            logger(LOG_INFO, "%s%s (%d) %s", preamble, properties.deviceName, properties.deviceID, deviceType);
        }

        state->physicalDevice = physicalDevices[0];

        free(physicalDevices);
    }
    else
    {
        // True here just forces the error log
        LOG_ERROR(true, "No Vulkan-supported physical devices found!");
    }
}

void createSurface(State_t *state)
{
    // surface is just a cross-platform abstraction of the window (helps with vulkan)
    LOG_ERROR(glfwCreateWindowSurface(state->instance, state->window, state->allocator, &state->surface),
              "Unable to create Vulkan window surface");
}

void selectQueueFamily(State_t *state)
{
    state->queueFamily = UINT32_MAX;
    // Different GPUs have different capabilities for different families. This can be very device-specific
    // once consdiering more than the first (default) family!
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc((sizeof(VkQueueFamilyProperties) * count));
    LOG_ERROR(queueFamilies == NULL, "Unable to allocate memory for GPU's queue families");

    vkGetPhysicalDeviceQueueFamilyProperties(state->physicalDevice, &count, queueFamilies);

    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < count; queueFamilyIndex++)
    {
        VkQueueFamilyProperties properties = queueFamilies[queueFamilyIndex];
        // Verify that the current queue family has the vulkan graphics bit flag and that glfw supports it.
        // Stop iterating one the correct one is found.
        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            glfwGetPhysicalDevicePresentationSupport(state->instance, state->physicalDevice, queueFamilyIndex))
        {
            state->queueFamily = queueFamilyIndex;

            logger(LOG_INFO, "Selected Queue Family %d of (0-%d) for use with Vulkan and GLFW.", queueFamilyIndex, count - 1);

            break;
        }
    }

    // This only logs the error if the above iteration found no suitable queue family
    LOG_ERROR(state->queueFamily == UINT32_MAX, "Unable to find a queue family with the supported Vulkan and GLFW flags.");

    free(queueFamilies);
}

void createDevice(State_t *state)
{
    const VkDeviceQueueCreateInfo queueCreateInfos = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = state->queueFamily,
        .queueCount = 1,                   // Only need to use the 1 queue
        .pQueuePriorities = &(float){1.0}, // Address to a 1.0 float because there is only the one queue
    };
    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueCreateInfos,
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &(const char *){VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    LOG_ERROR(vkCreateDevice(state->physicalDevice, &createInfo, state->allocator, &state->device),
              "Unable to create the Vulkan device.");
}

void getQueue(State_t *state)
{
    // Just like the CPU has threads for different cores, the GPU has queues for different families.
    // Unlike the CPU cores, the GPU's families are often used for different purposes instead of just
    // blasting out as many threads as possible. Ex: GPU Fam. 1 = compute, 2 = render, 3 = i/o with CPU
    // Allocating multiple queues when creating a device is suboptimal when they aren't actually used.
    // The device driver could (likely) cause underperformance because it has optimized itself to support
    // actually using those queues (workforce distribution)
    // 0 because we want the first (only) queue
    vkGetDeviceQueue(state->device, state->queueFamily, 0, &state->queue);
}

void init(State_t *state)
{
    logger(LOG_INFO, "Starting %s...", PROGRAM_NAME);
    logInfo();

    // Must init glfw first so that we can actually assign its error handler
    glfwInit();
    setupErrorHandling();

    createInstance(state);
    createWindow(state);

    selectPhysicalDevice(state);
    createSurface(state);
    selectQueueFamily(state);
    createDevice(state);
    getQueue(state);
}

void loop(State_t *state)
{
    while (!glfwWindowShouldClose(state->window))
    {
        // Handle the window events, including actually closing the window with the X
        glfwPollEvents();
    }
}

void cleanup(State_t *state)
{
    // Order matters here because of potential physical device and interdependency. createInstance() is
    // called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out.
    // The window doesn't need to be destroyed because GLFW handles it on its own.
    vkDestroyDevice(state->device, state->allocator);
    vkDestroySurfaceKHR(state->instance, state->surface, state->allocator);
    vkDestroyInstance(state->instance, state->allocator);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    state->window = NULL;
    state->instance = NULL;
    state->allocator = NULL;

    logger(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}

int main(void)
{
    // 4 min https://www.youtube.com/watch?v=yZrUWoIp_to&list=PLlKj-4rp1Gz0eBLIcq2wzd8uigFrJduJ-&index=3

    State_t state = {
        .applicationName = "VoxelC Application",
        .engineName = "VoxelC Engine",
        .windowTitle = "VoxelC",
        .windowWidth = 720,
        .windowHeight = 480,
        .windowResizable = false,
        .windowFullscreen = true,
        .vkAPIVersion = VK_API_VERSION_1_4};

    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}