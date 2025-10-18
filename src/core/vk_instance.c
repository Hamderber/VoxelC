#include "core/logs.h"
#include "core/types/state_t.h"
#include <vulkan/vulkan.h>
#include <string.h>
#include <stdlib.h>
#include "core/vk_instance.h"

/// @brief Logs the Vukan version
/// @param  void
void vki_logAPI(void)
{
    uint32_t instanceAPIVersion;
    logs_logIfError(vkEnumerateInstanceVersion(&instanceAPIVersion), "Failed to determine Vulkan instance version.");

    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceAPIVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceAPIVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceAPIVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceAPIVersion);

    logs_log(LOG_DEBUG, "Vulkan API %i.%i.%i.%i", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
}

/// @brief Logs the physical device's features and capabilities
/// @param physicalDeviceFeatures
/// @param capabilities
void vki_logCapabilities(VkPhysicalDeviceFeatures physicalDeviceFeatures, const VkSurfaceCapabilitiesKHR capabilities)
{
    logs_log(LOG_DEBUG, "The physical device has the following features:");
    logs_log(LOG_DEBUG, "\t\tImage count range: [%d-%d]", capabilities.minImageCount, capabilities.maxImageCount);
    logs_log(LOG_DEBUG, "\t\tMax Image Array Layers: %d", capabilities.maxImageArrayLayers);
    logs_log(LOG_DEBUG, "\t\tAnisotropic Filtering: %s", physicalDeviceFeatures.samplerAnisotropy ? "Supported" : "Unsupported");
}

/// @brief Gets the first memory type (best) that matches the property flags for the state's physical device
/// @param state
/// @param memoryRequirements
/// @param propertyFlags
/// @return uint32_t
uint32_t vki_physicalMemoryTypeGet(State_t *state, uint32_t memoryRequirements, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(state->context.physicalDevice, &memoryProperties);

    uint32_t memoryType = UINT32_MAX;

    for (uint32_t i = 0U; i < memoryProperties.memoryTypeCount; i++)
    {
        // Check if the corresponding bits of the filter are 1
        if ((memoryRequirements & (1 << i)) &&
            (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            memoryType = i;
            break;
        }
    }

    logs_logIfError(memoryType == UINT32_MAX,
                    "Failed to find suitable memory type!");

    return memoryType;
}

/// @brief Finds the first (best) format for the physical device that matches all criteria
/// @param state
/// @param candidates
/// @param candidateCount
/// @param tiling
/// @param features
/// @return VkFormat
VkFormat vki_formatSupportedFind(State_t *state, VkFormat *candidates, size_t candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
    for (size_t i = 0; i < candidateCount; i++)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(state->context.physicalDevice, candidates[i], &properties);

        // Iterate and compare flags for the current format candidate and try to return the first found that is best

        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
        {
            return candidates[i];
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
        {
            return candidates[i];
        }
    }

    logs_log(LOG_ERROR, "The physical device has no supported format!");

    // Same as error
    return VK_FORMAT_MAX_ENUM;
}

/// @brief Checks if the format has VK_FORMAT_D32_SFLOAT_S8_UINT or VK_FORMAT_D24_UNORM_S8_UINT
/// @param format
/// @return bool
bool vki_formatHasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/// @brief Enumerates the Vulkan instance's layer properties to determine if the pVALIDATION_LAYERS are supported
/// @param void
/// @return VkBool32
VkBool32 vki_validationLayerSupportCheck(void)
{
    uint32_t layerCount = 0;
    logs_logIfError(vkEnumerateInstanceLayerProperties(&layerCount, NULL),
                    "Failed to get instance layer properties!");
    VkLayerProperties *availableLayers = malloc(sizeof(VkLayerProperties) * layerCount);
    logs_logIfError(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers),
                    "Failed to enumerate instance layer properties!");

    VkBool32 layerFound = VK_FALSE;
    for (uint32_t i = 0; i < VALIDATION_LAYER_COUNT; ++i)
    {
        for (uint32_t j = 0; j < layerCount; ++j)
        {
            // Check if the string in the current validation layer is the same as the one in the available layer
            if (strcmp(pVALIDATION_LAYERS[i], availableLayers[j].layerName) == 0)
            {
                layerFound = VK_TRUE;
                break;
            }
        }
    }

    free(availableLayers);
    return layerFound;
}

/// @brief Creates the actual Vulkan instance
/// @param state
static void vki_instanceCreate(State_t *state)
{
    // Vulkan extensions required by GLFW
    uint32_t requiredExtensionsCount;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    // Add debug utils extension if validation is requested
    char *extensions[16];
    memcpy(extensions, requiredExtensions, sizeof(char *) * requiredExtensionsCount);
    uint32_t extensionCount = requiredExtensionsCount;

    if (state->config.vulkanValidation)
    {
        extensions[extensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        if (!vki_validationLayerSupportCheck())
        {
            logs_log(LOG_WARN, "Validation layers requested, but not available.");
            state->config.vulkanValidation = false;
        }
    }

    const VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = state->config.vkAPIVersion,
        .pApplicationName = state->config.pApplicationName,
        .pEngineName = state->config.pEngineName,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = extensions,
    };

    if (state->config.vulkanValidation)
    {
        createInfo.enabledLayerCount = VALIDATION_LAYER_COUNT;
        createInfo.ppEnabledLayerNames = pVALIDATION_LAYERS;
        logs_log(LOG_DEBUG, "Vulkan enabled with validation layers.");
    }

    logs_logIfError(vkCreateInstance(&createInfo, state->context.pAllocator, &state->context.instance),
                    "Couldn't create Vulkan instance.");
}

/// @brief Assigns the anisotropic filtering options to the state's renderer struct
/// @param state
static void vki_anisotropicFilteringOptionsGet(State_t *state)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(state->context.physicalDevice, &properties);

    float afMax = properties.limits.maxSamplerAnisotropy;
    logs_log(LOG_DEBUG, "The selected physical device supports up to %d x anisotropic filtering.", (int)afMax);

    if (afMax <= 1.0F)
    {
        logs_log(LOG_WARN, "Anisotropic filtering not supported (maxSamplerAnisotropy <= 1 x).");
        state->renderer.anisotropicFilteringOptionsCount = 0;
        state->renderer.anisotropicFilteringOptions = NULL;
        return;
    }

    // 1x, 2x, 4x, 8x, 16x max supported (powers of 2)
    int size = (int)floorf(log2f(afMax)) + 1;
    state->renderer.anisotropicFilteringOptionsCount = size;
    state->renderer.anisotropicFilteringOptions = malloc(sizeof(AnisotropicFilteringOptions_t) * size);

    logs_logIfError(state->renderer.anisotropicFilteringOptions == NULL,
                    "Failed to allocate anisotropic filtering options!");

    // Fill the table (1x, 2x, 4x, 8x, 16x)
    for (int i = 0; i < size; i++)
    {
        // Binary shift increments in powers of 2
        state->renderer.anisotropicFilteringOptions[i] = (AnisotropicFilteringOptions_t)(1 << i);
    }
}

/// @brief Selects the best physical device
/// @param state
static void vki_physicalDeviceSelect(State_t *state)
{
    uint32_t count;

    VkResult enumeratePhysicalDevicesResult = vkEnumeratePhysicalDevices(state->context.instance, &count, NULL);
    // VK_INCOMPLETE isn't considered a failure. If the user somehow has multiple physical devices but one isn't
    // available for use, then VK_INCOMPLETE will be returned to just signify that not all physical devices were
    // enumerated. (Ex: dual GPUs) https://registry.khronos.org/vulkan/specs/latest/man/html/vkEnumeratePhysicalDevices.html
    logs_logIfError(enumeratePhysicalDevicesResult != VK_INCOMPLETE && enumeratePhysicalDevicesResult != VK_SUCCESS,
                    "Couldn't enumerate Vulkan-supported physical device count.");
    ;
    if (count > 0)
    {
        logs_log(LOG_DEBUG, "Found %d Vulkan-supported physical device(s):", count);

        VkPhysicalDevice *physicalDevices = malloc(sizeof(VkPhysicalDevice) * count);
        logs_logIfError(physicalDevices == NULL,
                        "Unable to allocate memory for physical devices.");
        logs_logIfError(vkEnumeratePhysicalDevices(state->context.instance, &count, physicalDevices),
                        "Couldn't enumerate Vulkan-supported physical devices.");

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

            preamble = i == 0 ? "\t--> " : "\t\t";

            logs_log(LOG_DEBUG, "%s%s (%d) %s", preamble, properties.deviceName, properties.deviceID, deviceType);
        }

        state->context.physicalDevice = physicalDevices[0];

        free(physicalDevices);
    }
    else
    {
        // True here just forces the error log
        logs_logIfError(true,
                        "No Vulkan-supported physical devices found!");
    }
}

/// @brief Selects the queue family that has the best properties
/// @param state
static void vki_queueFamilySelect(State_t *state)
{
    state->context.queueFamily = UINT32_MAX;
    // Different GPUs have different capabilities for different families. This can be very device-specific
    // once consdiering more than the first (default) family!
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(state->context.physicalDevice, &count, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc((sizeof(VkQueueFamilyProperties) * count));
    logs_logIfError(queueFamilies == NULL,
                    "Unable to allocate memory for GPU's queue families.");

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

            logs_log(LOG_DEBUG, "Selected Queue Family %d of (0-%d) for use with Vulkan and GLFW.", queueFamilyIndex, count - 1);

            break;
        }
    }

    // This only logs the error if the above iteration found no suitable queue family
    logs_logIfError(state->context.queueFamily == UINT32_MAX,
                    "Unable to find a queue family with the supported Vulkan and GLFW flags.");

    free(queueFamilies);
}

/// @brief Creates a physicial device and assigns it to the state
/// @param state
static void vki_deviceCreate(State_t *state)
{
    // Query what features the selected GPU supports
    vkGetPhysicalDeviceFeatures(state->context.physicalDevice, &state->context.physicalDeviceFeatures);

    // Enable anisotropy if supported
    if (state->context.physicalDeviceFeatures.samplerAnisotropy)
    {
        state->context.physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
    }
    else
    {
        logs_log(LOG_WARN, "Device does not support anisotropic filtering (feature will be disabled).");
    }

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
        .ppEnabledExtensionNames = &(const char *){VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        .pEnabledFeatures = &state->context.physicalDeviceFeatures,
    };

    logs_logIfError(vkCreateDevice(state->context.physicalDevice, &createInfo, state->context.pAllocator, &state->context.device),
                    "Unable to create the Vulkan device.");
}

/// @brief Creates the state's Vulkan instance and associated contexts
/// @param state
void vki_create(State_t *state)
{
    vki_logAPI();

    vki_instanceCreate(state);
    vki_physicalDeviceSelect(state);
    vki_anisotropicFilteringOptionsGet(state);
    vki_queueFamilySelect(state);
    vki_deviceCreate(state);

    // Just like the CPU has threads for different cores, the GPU has queues for different families.
    // Unlike the CPU cores, the GPU's families are often used for different purposes instead of just
    // blasting out as many threads as possible. Ex: GPU Fam. 1 = compute, 2 = render, 3 = i/o with CPU
    // Allocating multiple queues when creating a device is suboptimal when they aren't actually used.
    // The device driver could (likely) cause underperformance because it has optimized itself to support
    // actually using those queues (workforce distribution)
    // 0 because we want the first (only) queue
    vkGetDeviceQueue(state->context.device, state->context.queueFamily, 0, &state->context.graphicsQueue);
}

/// @brief Destroys the state's Vulkan instance
/// @param state
void vki_destroy(State_t *state)
{
    vkDestroyDevice(state->context.device, state->context.pAllocator);
    vkDestroyInstance(state->context.instance, state->context.pAllocator);
    free(state->renderer.anisotropicFilteringOptions);
}
