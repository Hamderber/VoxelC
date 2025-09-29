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
    uint32_t vkAPIVersion;
    int windowWidth;
    int windowHeight;
    bool windowResizable;
    bool windowFullscreen;
} Config_t;

typedef struct
{
    VkSwapchainKHR handle;
    uint32_t imageCount;
    bool recreate;
    VkImage *images;
    VkImageView *imageViews;
} Swapchain_t;

typedef struct
{
    // Vulkan
    Swapchain_t swapchain;
    VkSurfaceKHR surface;

    // GLFW
    GLFWwindow *windowHandle;
    int frameBufferWidth;
    int frameBufferHeight;
} Window_t;

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkQueue queue;
    VkAllocationCallbacks *allocator;
    /// @brief UINT32_MAX means no family assigned (set to max during creation)
    uint32_t queueFamily;
} Context_t;

typedef struct
{
    int placeholder;
} Renderer_t;

typedef struct
{
    Config_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
} State_t;

void glfwErrorCallback(int errorCode, const char *description)
{
    LOG_IF_ERROR(errorCode,
                 "GLFW: %s", description)
}

void exitCallback(void)
{
    // While the OS will handle the garbage collection/etc of all code at program termination anyway, a safe-exit
    // isn't necessary. But because glfw is a windowing library, it could've potentially altered system settings
    // for rendering/etc. It is very important to safely terminate glfw before fully exiting the program.
    glfwTerminate();
}

void setupErrorHandling()
{
    // Set function that is called when a glfw error is caught
    glfwSetErrorCallback(glfwErrorCallback);
    // Set function to be called at program termination
    atexit(exitCallback);
}

/// @brief This is requried even if window-is-resizable is hard-coded to false! This is because the user can
/// always minimize the window, which also causes a frame buffer size change.
/// @param window
/// @param frameBufferWidth
/// @param frameBufferHeight
void glfwFramebufferSizeCallback(GLFWwindow *window, int frameBufferWidth, int frameBufferHeight)
{
    logger(LOG_INFO, "Frame buffer size changed to %d by %d (WxH)", frameBufferWidth, frameBufferHeight);
    // The user pointer is saved as a void pointer, but we are effectively explicitly casting that void pointer back
    // to the State_t type.
    State_t *state = glfwGetWindowUserPointer(window);

    state->window.swapchain.recreate = true;
    state->window.frameBufferWidth = frameBufferWidth;
    state->window.frameBufferHeight = frameBufferHeight;
}

void surfaceCreate(State_t *state){
    // surface is just a cross-platform abstraction of the window (helps with vulkan)
    LOG_IF_ERROR(glfwCreateWindowSurface(state -> context.instance, state->window.windowHandle, state->context.allocator, &state->window.surface),
                 "Unable to create Vulkan window surface")}

uint32_t clamp_uint32_t(uint32_t num, uint32_t min, uint32_t max)
{
    if (num < min)
    {
        return min;
    }
    else if (num > max)
    {
        return max;
    }
    else
    {
        return num;
    }
}

/// @brief Destroy the swapchain's image views before allocating new ones. This is imporant because this method could be called
/// after the swapchain has already been made. If this happens, then there would be a memory leak from where the previous
/// swapchain image views were. The actual swapchain images themselves are deleted by the OS regardless.
/// @param state
void swapchainImagesFree(State_t *state)
{
    if (state->window.swapchain.handle != NULL && state->window.swapchain.imageViews)
    {
        for (uint32_t i = 0U; i < state->window.swapchain.imageCount; i++)
        {
            vkDestroyImageView(state->context.device, state->window.swapchain.imageViews[i], state->context.allocator);
        }

        free(state->window.swapchain.imageViews);
        state->window.swapchain.imageViews = NULL;

        free(state->window.swapchain.images);
        state->window.swapchain.images = NULL;
    }
}

void swapchainCreate(State_t *state)
{
    logger(LOG_INFO, "Creating the swapchain...");

    VkSurfaceCapabilitiesKHR capabilities;
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->context.physicalDevice, state->window.surface, &capabilities),
                 "Failed to query physical device surface capabilities.");

    uint32_t formatCount;
    // null so that we just get the number of formats
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(state->context.physicalDevice, state->window.surface, &formatCount, NULL),
                 "Failed to query physical device surface format count.")
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    LOG_IF_ERROR(formats == NULL,
                 "Unable to allocate memory for Vulkan surface formats")
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(state->context.physicalDevice, state->window.surface, &formatCount, formats),
                 "Failed to query physical device surface formats.")

    VkSurfaceFormatKHR format = formats[0]; // Default to the first format ...
    for (uint32_t i = 0U; i < formatCount; i++)
    {
        // SRGB is the most commonly supported (and best) so we want that one if available
        // B8G8R8A8 = 8-bit blue/green/red/alpha components (so a 32-bit color depth)
        if (formats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR && formats[i].format == VK_FORMAT_B8G8R8A8_SRGB)
        {
            format = formats[i]; // ... Unless the preferred format is found.
            break;
        }
    }

    free(formats);

    logger(LOG_INFO, "The physical device has the following features:");
    logger(LOG_INFO, "\t\tImage count range: [%d-%d]", capabilities.minImageCount, capabilities.maxImageCount);
    logger(LOG_INFO, "\t\tMax Image Array Layers: %d", capabilities.maxImageArrayLayers);

    // See https://www.youtube.com/watch?v=nSzQcyQTtRY for the different present modes (visual examples)
    // Immedaite causes screen tearing (don't use) and I think Mailbox sounds the best. Unfortunately,
    // Mailbox isn't universally supported and is much more power-intensive (constant frame generation/discarding)
    // FIFO is required to exist on all platforms. All others are potentially not :(
    uint32_t presentModeCount;
    // null so that we just get the number of present modes
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(state->context.physicalDevice, state->window.surface, &presentModeCount, NULL),
                 "Failed to query physical device surface presentation modes.")
    VkPresentModeKHR *presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    LOG_IF_ERROR(presentModes == NULL,
                 "Unable to allocate memory for Vulkan present modes.")
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(state->context.physicalDevice, state->window.surface, &presentModeCount, presentModes),
                 "Failed to query physical device surface presentation modes.")

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // Default to FIFO ...
    for (uint32_t i = 0U; i < presentModeCount; i++)
    {
        if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = presentModes[i]; // ... Unless Mailbox is available
            break;
        }
    }

    free(presentModes);

    // Prevent the image extend from somehow exceeding what the physical device is capable of
    VkExtent2D imageExtent = {
        .width = clamp_uint32_t(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = clamp_uint32_t(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->window.surface,
        .queueFamilyIndexCount = 1U,                        // Only using 1 queue family
        .pQueueFamilyIndices = &state->context.queueFamily, // Skip making an array for this since only using 1 queue family
        // Don't render pixels that are obscured by some other program window (ex: Chrome). If using some complex
        // post-processing or other things, this should be false because it requires the whole image to be processed regardless.
        .clipped = true,
        // If this were not opaque, the window itself could have some type of alpha transparency feature that would cause the OS to
        // render some window behind this one through the alpha transparency sections. Obviously not applicable here, though
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        // >1 would be for something like a VR headset or something where you have the red and blue screen for 3D or something
        .imageArrayLayers = 1U,
        // Only using 1 queue for the swapchain/rendering so there is no need for concurrency here. Exclusive has a value of 0
        // so it doesn't technically need to be here, but stating this explicitly is good for readability/etc.
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        // https://www.youtube.com/watch?v=yZrUWoIp_to 26 min has a good explanation of these
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        // The value is initially null but is still stored when the swapchain needs to be recreated.
        // Because this is a pointer in a pointer, the swapchain itself must exist first here. Otherwise just NULL directly.
        .oldSwapchain = state->window.swapchain.handle ? state->window.swapchain.handle : NULL,
        // Only really applicable to mobile devices. Phone screens etc. obviously have to support rotating 90/180 but this same
        // support is often not included with desktop/laptop GPUs. Identity just means keep the image the same.
        // Current transform is almost certainly VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        .preTransform = capabilities.currentTransform,
        .imageExtent = imageExtent,
        // It is good to add 1 to the minimum image count when using Mailbox so that the pipeline isn't blocked while the image is
        // still being presented. Also have to make sure that the max image count isn't exceeded.
        // This number is basically how many images should be able to be stored/queued/generated in the buffer while the screen is still
        // drawing. This has a *** HUGE *** impact on performance.
        .minImageCount = presentMode == (VK_PRESENT_MODE_MAILBOX_KHR && // maxImageCount may not be assigned if GPU doesn't declare/is unbounded
                                         capabilities.minImageCount + 1U <= (capabilities.maxImageCount ? capabilities.maxImageCount : UINT32_MAX))
                             ? capabilities.minImageCount + 1 // Mailbox
                             : capabilities.minImageCount,    // Not Mailbox (FIFO)
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .presentMode = presentMode,
    };

    // Prevent memory leak by freeing previous swapchain's image views if they exist
    swapchainImagesFree(state);
    VkSwapchainKHR swapchain;

    LOG_IF_ERROR(vkCreateSwapchainKHR(state->context.device, &createInfo, state->context.allocator, &swapchain),
                 "Failed to create Vulkan swapchain!")
    // Even though the state's initial swapchain is obviously null, this sets us up to properly assign the new one (drivers/etc.)
    vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, state->context.allocator);
    state->window.swapchain.handle = swapchain;

    // null so that we just get the number of formats
    LOG_IF_ERROR(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle, &state->window.swapchain.imageCount, NULL),
                 "Failed to query the number of images in the swapchain.");
    logger(LOG_INFO, "The swapchain will contain a buffer of %d images.", state->window.swapchain.imageCount);
    state->window.swapchain.images = malloc(sizeof(VkImage) * state->window.swapchain.imageCount);
    LOG_IF_ERROR(state->window.swapchain.images == NULL,
                 "Unable to allocate memory for swapchain images.")
    LOG_IF_ERROR(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle,
                                         &state->window.swapchain.imageCount, state->window.swapchain.images),
                 "Failed to get the images in the swapchain.")
    state->window.swapchain.imageViews = malloc(sizeof(VkImageView) * state->window.swapchain.imageCount);
    LOG_IF_ERROR(state->window.swapchain.imageViews == NULL,
                 "Unable to allocate memory for swapchain image views.")

    VkComponentMapping componentMapping = {
        // RGBA is still red/blue/green/alpha. Identity is keep it default but it could be .._A/etc
        // Identity = 0 so this could be omitted, but explicit declaration is better visually
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    // Allows for supporting multiple image view layers for the same image etc.
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .layerCount = 1,
        .levelCount = 1,
    };

    for (uint32_t i = 0U; i < state->window.swapchain.imageCount; i++)
    {
        // This is defined in the loop because of using i for the swapchain image
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format = format.format,
            .image = state->window.swapchain.images[i],
            // You could map the green component to red to do color shifting if desired or something
            .components = componentMapping,
            .subresourceRange = subresourceRange,
            // The view type of the window itself. Obviously, a screen is 2D. Maybe 3D is for VR or something.
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
        };
        // Pass the address of the specific spot in the array of swapchain image views
        LOG_IF_ERROR(vkCreateImageView(state->context.device, &createInfo, state->context.allocator, &state->window.swapchain.imageViews[i]),
                     "Failed to create swapchain image view %d", i)
    }
}

void windowCreate(State_t *state)
{
    // Vulkan => no api. OpenGL would require the OpenGL api
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, state->config.windowResizable);

    int width = state->config.windowWidth;
    int height = state->config.windowHeight;
    // There is no need to store the actual monitor reference. Just the window.
    GLFWmonitor *monitor = NULL;

    if (state->config.windowFullscreen)
    {
        monitor = glfwGetPrimaryMonitor();
        // If the window is fullscreen, set the window's resolution to the monitor's
        const GLFWvidmode *mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
    }

    // If not fullscreen, set window resolution to the default state values (set in main())
    state->window.windowHandle = glfwCreateWindow(width, height, state->config.windowTitle, monitor, NULL);

    int frameBufferWidth;
    int frameBufferHeight;
    glfwGetFramebufferSize(state->window.windowHandle, &frameBufferWidth, &frameBufferHeight);
    state->window.frameBufferWidth = frameBufferWidth;
    state->window.frameBufferHeight = frameBufferHeight;

    // This allows for the glfw window to keep a reference to the state. Thus, we don't have to make state a global variable.
    // This is necessary for things such as callback functions (see below in this method) where the callback function
    // otherwise wouldn't have access to the state.
    glfwSetWindowUserPointer(state->window.windowHandle, state);

    // If the window changes size, call this function. There is a window-specific one, but the frame buffer one is better.
    // This allows for supporting retina displays and other screens that use subpixels (Vulkan sees subpixels as normal pixels).
    // For those types of displays, the window width/height and the frame buffer size would be different numbers. Also consider
    // that if the user has two monitors with only one being a retina display, they could drag the window from one screen to another
    // which would change the frame buffer size but NOT the actual window dimensions.
    glfwSetFramebufferSizeCallback(state->window.windowHandle, glfwFramebufferSizeCallback);

    surfaceCreate(state);
    swapchainCreate(state);
}

void instanceCreate(State_t *state)
{
    uint32_t requiredExtensionsCount;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    const VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = state->config.vkAPIVersion,
        .pApplicationName = state->config.applicationName,
        .pEngineName = state->config.engineName};

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = requiredExtensionsCount,
        .ppEnabledExtensionNames = requiredExtensions};

    LOG_IF_ERROR(vkCreateInstance(&createInfo, state->context.allocator, &state->context.instance),
                 "Couldn't create Vulkan instance.")
}

void logInfo()
{
    uint32_t instanceAPIVersion;
    LOG_IF_ERROR(vkEnumerateInstanceVersion(&instanceAPIVersion),
                 "Failed to determine Vulkan instance version.")

    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceAPIVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceAPIVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceAPIVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceAPIVersion);

    logger(LOG_INFO, "Vulkan API %i.%i.%i.%i", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
    logger(LOG_INFO, "GLWF %s", glfwGetVersionString());
}

void physicalDeviceSelect(State_t *state)
{
    uint32_t count;

    LOG_IF_ERROR(vkEnumeratePhysicalDevices(state->context.instance, &count, NULL),
                 "Couldn't enumerate Vulkan-supported physical device count.")

    if (count > 0)
    {
        logger(LOG_INFO, "Found %d Vulkan-supported physical device(s):", count);

        VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * count);
        LOG_IF_ERROR(physicalDevices == NULL,
                     "Unable to allocate memory for physical devices.")
        LOG_IF_ERROR(vkEnumeratePhysicalDevices(state->context.instance, &count, physicalDevices),
                     "Couldn't enumerate Vulkan-supported physical devices.")

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

        state->context.physicalDevice = physicalDevices[0];

        free(physicalDevices);
    }
    else
    {
        // True here just forces the error log
        LOG_IF_ERROR(true,
                     "No Vulkan-supported physical devices found!")
    }
}

void queueFamilySelect(State_t *state)
{
    state->context.queueFamily = UINT32_MAX;
    // Different GPUs have different capabilities for different families. This can be very device-specific
    // once consdiering more than the first (default) family!
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(state->context.physicalDevice, &count, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc((sizeof(VkQueueFamilyProperties) * count));
    LOG_IF_ERROR(queueFamilies == NULL,
                 "Unable to allocate memory for GPU's queue families.")

    vkGetPhysicalDeviceQueueFamilyProperties(state->context.physicalDevice, &count, queueFamilies);

    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < count; queueFamilyIndex++)
    {
        VkQueueFamilyProperties properties = queueFamilies[queueFamilyIndex];
        // Verify that the current queue family has the vulkan graphics bit flag and that glfw supports it.
        // Stop iterating one the correct one is found.
        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            glfwGetPhysicalDevicePresentationSupport(state->context.instance, state->context.physicalDevice, queueFamilyIndex))
        {
            state->context.queueFamily = queueFamilyIndex;

            logger(LOG_INFO, "Selected Queue Family %d of (0-%d) for use with Vulkan and GLFW.", queueFamilyIndex, count - 1);

            break;
        }
    }

    // This only logs the error if the above iteration found no suitable queue family
    LOG_IF_ERROR(state->context.queueFamily == UINT32_MAX,
                 "Unable to find a queue family with the supported Vulkan and GLFW flags.")

    free(queueFamilies);
}

void deviceCreate(State_t *state)
{
    const VkDeviceQueueCreateInfo queueCreateInfos = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = state->context.queueFamily,
        .queueCount = 1,                   // Only need to use the 1 queue
        .pQueuePriorities = &(float){1.0}, // Address to a 1.0 float because there is only the one queue
    };
    const VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queueCreateInfos,
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = &(const char *){VK_KHR_SWAPCHAIN_EXTENSION_NAME}};

    LOG_IF_ERROR(vkCreateDevice(state->context.physicalDevice, &createInfo, state->context.allocator, &state->context.device),
                 "Unable to create the Vulkan device.")
}

void queueGet(State_t *state)
{
    // Just like the CPU has threads for different cores, the GPU has queues for different families.
    // Unlike the CPU cores, the GPU's families are often used for different purposes instead of just
    // blasting out as many threads as possible. Ex: GPU Fam. 1 = compute, 2 = render, 3 = i/o with CPU
    // Allocating multiple queues when creating a device is suboptimal when they aren't actually used.
    // The device driver could (likely) cause underperformance because it has optimized itself to support
    // actually using those queues (workforce distribution)
    // 0 because we want the first (only) queue
    vkGetDeviceQueue(state->context.device, state->context.queueFamily, 0, &state->context.queue);
}

void swapchainDestroy(State_t *state)
{
    swapchainImagesFree(state);
    vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, state->context.allocator);
    vkDestroySurfaceKHR(state->context.instance, state->window.surface, state->context.allocator);
}

void windowDestroy(State_t *state)
{
    swapchainDestroy(state);
    glfwDestroyWindow(state->window.windowHandle);
}

void contextDestroy(State_t *state)
{
    vkDestroyDevice(state->context.device, state->context.allocator);
    vkDestroyInstance(state->context.instance, state->context.allocator);
}

void rendererDestroy(State_t *state)
{
    // Placeholder
}

void contextCreate(State_t *state)
{
    instanceCreate(state);
    physicalDeviceSelect(state);
    queueFamilySelect(state);
    deviceCreate(state);
    queueGet(state);
}

void rendererCreate(State_t *state)
{
    // Placeholder
}

bool windowShouldClose(State_t *state)
{
    return glfwWindowShouldClose(state->window.windowHandle);
}

void init(State_t *state)
{
    logger(LOG_INFO, "Starting %s...", PROGRAM_NAME);
    logInfo();

    // Must init glfw first so that we can actually assign its error handler
    glfwInit();
    setupErrorHandling();

    contextCreate(state);
    windowCreate(state);
    rendererCreate(state);
}

void loop(State_t *state)
{
    while (!windowShouldClose(state))
    {
        // Handle the window events, including actually closing the window with the X
        glfwPollEvents();

        // Must call this after the glfw poll events because resizing the window and the associated callback would be
        // generated from that function. This will only hit AFTER the user has LET GO of the side of the window during resize.
        // This means that each time the window changes, the swapchain will only be recreated once the user STOPS the resize
        // process.
        if (state->window.swapchain.recreate)
        {
            state->window.swapchain.recreate = false;
            swapchainCreate(state);
            logger(LOG_INFO, "Re-created the swapchain.");
        }

        // uint64_t imageTimeout = UINT64_MAX;
        // // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
        // // Fence: same above but for CPU
        // VkSemaphore semaphore = NULL;
        // VkFence fence = NULL;
        // uint32_t imageIndex;
        // LOG_IF_ERROR(vkAcquireNextImageKHR(state->device, state->swapchain, imageTimeout, semaphore, fence, &imageIndex),
        //              "Failed to aquire the next image from the swapchain. Index: %d", imageIndex)

        // VkPresentInfoKHR presentInfo = {
        //     .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        //     // Don't just pass the address to imageIndex because it wouldn't be a const. Declare it inline
        //     .pImageIndices = &(uint32_t){imageIndex},
        //     .swapchainCount = 1,
        //     .pSwapchains = &state->swapchain,
        // };

        // // Can't just catch this result with the error logger. Actually have to handle it.
        // VkResult result = vkQueuePresentKHR(state->queue, &presentInfo);

        // // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
        // // MUST be recreated immediately and presentation will just be attempted on the next frame.
        // if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        // {
        //     logger(LOG_WARN, "The swapchain is out of date and must be recreated! VkResult = %d", result);
        //     state->recreateSwapchain = true;
        // }
        // else
        // {
        //     LOG_IF_ERROR(result,
        //                  "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.")
        // }
    }
}

void cleanup(State_t *state)
{
    // Order matters here (including order inside of destroy functions)because of potential physical device and interdependency.
    // instanceCreate() is called first for init vulkan so it must be destroyed last. Last In First Out / First In Last Out.
    // The window doesn't need to be destroyed because GLFW handles it on its own. Stated explicitly for legibility.
    rendererDestroy(state);
    windowDestroy(state);
    contextDestroy(state);
    // Best practice to mitigate dangling pointers. Not strictly necessary, though
    state->window.swapchain.handle = NULL;
    state->context.instance = NULL;
    state->context.allocator = NULL;

    logger(LOG_INFO, "%s exited sucessfully.", PROGRAM_NAME);
}

int main(void)
{
    // Vulkan initialization:
    // 0 min https://youtu.be/-sdcRqpuOkU?si=TL-e2GkngVIV_h4c

    // Swapchain
    // https://www.youtube.com/watch?v=nSzQcyQTtRY

    Config_t config = {
        .applicationName = "VoxelC Application",
        .engineName = "VoxelC Engine",
        .windowTitle = "VoxelC",
        .windowWidth = 720,
        .windowHeight = 480,
        // If the window gets resized, the swapchain MUST be recreated
        .windowResizable = true,
        .windowFullscreen = false,
        .vkAPIVersion = VK_API_VERSION_1_4};

    State_t state = {
        .config = config};

    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}
