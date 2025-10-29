#pragma region Includes
#include <vulkan/vulkan.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Tools
bool vulkan_format_hasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }

uint32_t vulkan_device_physicalMemoryType_get(const State_t *pSTATE, uint32_t memoryRequirements, VkMemoryPropertyFlags propertyFlags)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(pSTATE->context.physicalDevice, &memoryProperties);

    uint32_t memoryType = UINT32_MAX;
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        // Check if the corresponding bits of the filter are 1
        if ((memoryRequirements & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags)
        {
            memoryType = i;
            break;
        }

    logs_logIfError(memoryType == UINT32_MAX, "Failed to find suitable memory type!");

    return memoryType;
}

VkFormat vulkan_instance_formatSupportedFind(State_t *pState, VkFormat *pCandidates, size_t candidateCount,
                                             VkImageTiling tiling, VkFormatFeatureFlags features)
{
    VkFormatProperties properties;
    for (size_t i = 0; i < candidateCount; i++)
    {
        vkGetPhysicalDeviceFormatProperties(pState->context.physicalDevice, pCandidates[i], &properties);

        // Iterate and compare flags for the current format candidate and try to return the first found that is best
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
            return pCandidates[i];
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
            return pCandidates[i];
    }

    logs_log(LOG_ERROR, "The physical device has no supported format!");

    // Same as error
    return VK_FORMAT_MAX_ENUM;
}

void vulkan_deviceCapabilities_log(const VkPhysicalDeviceFeatures PHYSICAL_DEVICE_FEATURES, const VkSurfaceCapabilitiesKHR capabilities)
{
    logs_log(LOG_DEBUG, "The physical device has the following features:");
    logs_log(LOG_DEBUG, "\t\tImage count range: [%d-%d]", capabilities.minImageCount, capabilities.maxImageCount);
    logs_log(LOG_DEBUG, "\t\tMax Image Array Layers: %d", capabilities.maxImageArrayLayers);
    logs_log(LOG_DEBUG, "\t\tAnisotropic Filtering: %s", PHYSICAL_DEVICE_FEATURES.samplerAnisotropy ? "Supported" : "Unsupported");
    logs_log(LOG_DEBUG, "\t\tWireframe: %s", PHYSICAL_DEVICE_FEATURES.fillModeNonSolid ? "Supported" : "Unsupported");
    logs_log(LOG_DEBUG, "\t\tLogic Operations: %s", PHYSICAL_DEVICE_FEATURES.logicOp ? "Supported" : "Unsupported");
}

/// @brief Iterates device's memory heaps to estimate VRAM. This will be overestimated on integrated GPUs (shares RAM)
static uint64_t estimate_dedicated_vram(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    bool heapHasHostVisible[VK_MAX_MEMORY_HEAPS] = {0};
    for (uint32_t t = 0; t < memoryProperties.memoryTypeCount; ++t)
        if (memoryProperties.memoryTypes[t].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            heapHasHostVisible[memoryProperties.memoryTypes[t].heapIndex] = true;

    // classic discrete VRAM
    uint64_t nonHostVisibleLocal = 0;
    // includes BAR/shared cases (bigger for integrated)
    uint64_t anyLocal = 0;

    for (uint32_t h = 0; h < memoryProperties.memoryHeapCount; ++h)
        if (memoryProperties.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
        {
            anyLocal += memoryProperties.memoryHeaps[h].size;
            if (!heapHasHostVisible[h])
                nonHostVisibleLocal += memoryProperties.memoryHeaps[h].size;
        }

    // Prefer truly non-host-visible VRAM when present. Dedicated GPUs will report nonHost as 0 without using device-specific
    // drivers. Otherwise, fall back to all device-local (BAR-enabled dGPU or iGPU).
    return nonHostVisibleLocal ? nonHostVisibleLocal : anyLocal;
}

/// @brief Logs the Vulkan API version
static void apiVersion_log(void)
{
    uint32_t instanceAPIVersion;
    logs_logIfError(vkEnumerateInstanceVersion(&instanceAPIVersion), "Failed to determine Vulkan instance version.");

    uint32_t apiVersionVariant = VK_API_VERSION_VARIANT(instanceAPIVersion);
    uint32_t apiVersionMajor = VK_API_VERSION_MAJOR(instanceAPIVersion);
    uint32_t apiVersionMinor = VK_API_VERSION_MINOR(instanceAPIVersion);
    uint32_t apiVersionPatch = VK_API_VERSION_PATCH(instanceAPIVersion);

    logs_log(LOG_DEBUG, "Vulkan API %i.%i.%i.%i", apiVersionVariant, apiVersionMajor, apiVersionMinor, apiVersionPatch);
}
#pragma endregion
#pragma region Dev. Capabilities
/// @brief Assigns the anisotropic filtering options to the state's renderer struct
static void device_anisotropicFilteringOptions_get(State_t *state)
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

/// @brief Enable anisotropy if supported
static void anisotropicFilteringOptions_tryEnable(State_t *pState)
{
    device_anisotropicFilteringOptions_get(pState);

    if (pState->context.physicalDeviceSupportedFeatures.samplerAnisotropy)
        pState->context.physicalDeviceEnabledFeatures.samplerAnisotropy = VK_TRUE;
    else
        logs_log(LOG_WARN, "Device does not support anisotropic filtering (feature will be disabled).");
}

/// @brief Enable wireframe drawing if supported
static void wireframeDrawing_tryEnable(State_t *pState)
{
    if (pState->context.physicalDeviceSupportedFeatures.fillModeNonSolid)
        pState->context.physicalDeviceEnabledFeatures.fillModeNonSolid = VK_TRUE;
    else
        logs_log(LOG_WARN, "Device does not support non-solid fill (wireframe disabled).");
}

/// @brief Enable logic operations if supported
static void logicOps_tryEnable(State_t *pState)
{
    if (pState->context.physicalDeviceSupportedFeatures.logicOp)
        pState->context.physicalDeviceEnabledFeatures.logicOp = VK_TRUE;
    else
        logs_log(LOG_WARN, "Device does not support logic operations! Some graphics features will be disabled.");
}
#pragma endregion
#pragma region Device Creation
/// @brief Creates a physicial device and assigns it to the state
static bool device_create(State_t *pState)
{
    // Query what features the selected GPU supports
    vkGetPhysicalDeviceFeatures(pState->context.physicalDevice, &pState->context.physicalDeviceSupportedFeatures);

    anisotropicFilteringOptions_tryEnable(pState);
    wireframeDrawing_tryEnable(pState);
    logicOps_tryEnable(pState);

    const VkDeviceQueueCreateInfo QUEUE_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = pState->context.queueFamily,
        // Only need to use the 1 queue
        .queueCount = 1,
        // Address to a 1.0 float because there is only the one queue
        .pQueuePriorities = &(float){1.0},
    };

    const VkDeviceCreateInfo DEVICE_CREATE_INFO = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &QUEUE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .enabledExtensionCount = (uint32_t)s_REQUIRED_EXTENSIONS_COUNT,
        .ppEnabledExtensionNames = s_pREQUIRED_EXTENSIONS,
        .pEnabledFeatures = &pState->context.physicalDeviceEnabledFeatures,
    };

    if (vkCreateDevice(pState->context.physicalDevice, &DEVICE_CREATE_INFO, pState->context.pAllocator, &pState->context.device) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to make the Vulkan device!");
        return false;
    }

    return true;
}

/// @brief Assign the device's queue family queue to the state's graphics queue
static void device_getQueue(State_t *pState)
{
    // Just like the CPU has threads for different cores, the GPU has queues for different families.
    // Unlike the CPU cores, the GPU's families are often used for different purposes instead of just
    // blasting out as many threads as possible. Ex: GPU Fam. 1 = compute, 2 = render, 3 = i/o with CPU
    // Allocating multiple queues when creating a device is suboptimal when they aren't actually used.
    // The device driver could (likely) cause underperformance because it has optimized itself to support
    // actually using those queues (workforce distribution)
    uint32_t queueIndex = 0;
    vkGetDeviceQueue(pState->context.device, pState->context.queueFamily, queueIndex, &pState->context.graphicsQueue);
}
#pragma endregion
#pragma region Dev. Compatibility
static bool extensions_supported(VkPhysicalDevice physicalDevice)
{
    uint32_t count = 0;
    VkResult result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &count, NULL);
    if (result != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to enumerate Vulkan physical device extension properties!");
        return false;
    }

    VkExtensionProperties *pExtensions = malloc(sizeof(*pExtensions) * count);
    if (!pExtensions)
    {
        logs_log(LOG_ERROR, "Failed to allocate memory for Vulkan physical device required extensions!");
        return false;
    }

    result = vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &count, pExtensions);
    if (result != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to enumerate Vulkan physical device extension properties!");
        free(pExtensions);
        return false;
    }

    bool supported = true;
    for (size_t i = 0; i < s_REQUIRED_EXTENSIONS_COUNT; ++i)
    {
        bool found = false;
        for (size_t j = 0; j < count; ++j)
        {
            if (strcmp(s_pREQUIRED_EXTENSIONS[i], pExtensions[j].extensionName) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            supported = false;
            break;
        }
    }

    free(pExtensions);
    return supported;
}

/// @brief Checks if the physical device has the required flags
static bool queue_families_supported(const VkInstance INSTANCE, VkPhysicalDevice physicalDevice, uint32_t *queueFamily)
{
    // Different GPUs have different capabilities for different families. This can be very device-specific
    // once consdiering more than the first (default) family!
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, NULL);

    VkQueueFamilyProperties *pQueueFamilies = malloc((sizeof(VkQueueFamilyProperties) * count));
    if (!pQueueFamilies)
    {
        logs_log(LOG_ERROR, "Failed to allocate memory for Vulkan device queue families!");
        return false;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, pQueueFamilies);

    bool supported = false;
    for (uint32_t i = 0; i < count; i++)
    {
        VkQueueFamilyProperties properties = pQueueFamilies[i];
        // Verify that the current queue family has the vulkan graphics bit flag and that glfw supports it.
        // Stop iterating one the correct one is found (if present)
        // Don't verify surface support here because the VKSurfaceKHR in state is VK_NULL_HANDLE at this point.
        if (properties.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            glfwGetPhysicalDevicePresentationSupport(INSTANCE, physicalDevice, i))
        {
            *queueFamily = i;
            supported = true;
            break;
        }
    }

    free(pQueueFamilies);
    return supported;
}
#pragma endregion
#pragma region Device Selection
/// @brief Selects the best physical device
static bool physicalDevice_select(State_t *pState)
{
    uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(pState->context.instance, &count, NULL);

    if (result != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to query Vulkan physical device count (res=%d).", result);
        return false;
    }

    if (count == 0)
        return false;

    VkPhysicalDevice *pPhysicalDevices = malloc(sizeof(VkPhysicalDevice) * count);
    if (!pPhysicalDevices)
    {
        logs_log(LOG_ERROR, "Failed to allocate memory for Vulkan physical devices!");
        return false;
    }

    if (vkEnumeratePhysicalDevices(pState->context.instance, &count, pPhysicalDevices) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Couldn't enumerate Vulkan-supported physical devices.");
        free(pPhysicalDevices);
        return false;
    }

    // Pick the best device
    int bestIndex = -1;
    int bestQueueFamily = -1;
    int bestScore = -1;
    uint64_t bestLocalBytes = 0;
    for (size_t i = 0; i < count; i++)
    {
        VkPhysicalDeviceProperties properties = {0};
        vkGetPhysicalDeviceProperties(pPhysicalDevices[i], &properties);

        int score = 0;
        switch (properties.deviceType)
        {
        case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):
            score = 3000;
            break;
        case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU):
            score = 2000;
            break;
        case (VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU):
            score = 1000;
            break;
        case (VK_PHYSICAL_DEVICE_TYPE_CPU):
            score = -1000;
            break;
        }

        uint32_t queueFamily = 0;
        if (!queue_families_supported(pState->context.instance, pPhysicalDevices[i], &queueFamily) ||
            !extensions_supported(pPhysicalDevices[i]))
            continue;
        else
            logs_log(LOG_DEBUG, "The best Queue Family for device %" PRIu32 " is %" PRIu32 " for use with Vulkan and GLFW.",
                     i, queueFamily);

        uint64_t vram = estimate_dedicated_vram(pPhysicalDevices[i]);
        // Tie-breakers for better resolution/vram when multiple of the best choice are available
        score += (int)(properties.limits.maxImageDimension2D / 1024);
        if (score > bestScore || (score == bestScore && vram > bestLocalBytes))
        {
            bestScore = score;
            bestLocalBytes = vram;
            bestIndex = (int)i;
            bestQueueFamily = queueFamily;
        }
    }

    if (bestIndex < 0)
    {
        free(pPhysicalDevices);
        logs_log(LOG_ERROR, "No suitable Vulkan physical devices found.");
        return false;
    }

#if defined(DEBUG)
    logs_log(LOG_DEBUG, "Found %" PRIu32 " Vulkan-supported physical device(s):", count);
    char *pDeviceType = NULL;
    char *pPreamble = NULL;
    for (uint32_t i = 0; i < count; i++)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(pPhysicalDevices[i], &properties);

        switch (properties.deviceType)
        {
        case (VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU):
            pDeviceType = "Integrated GPU";
            break;
        case (VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU):
            pDeviceType = "Discrete GPU";
            break;
        case (VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU):
            pDeviceType = "Virtual GPU";
            break;
        case (VK_PHYSICAL_DEVICE_TYPE_CPU):
            pDeviceType = "CPU";
            break;
        default:
            pDeviceType = "Other";
            break;
        }

        // Draw arrow in logs to show which device is selected
        pPreamble = (int)i == bestIndex ? "\t--> " : "\t\t";

        uint64_t vram = estimate_dedicated_vram(pPhysicalDevices[i]);

        logs_log(LOG_DEBUG, "%s%s (%" PRIu32 ") %s %" PRIu64 " Bytes VRAM (estimated)",
                 pPreamble, properties.deviceName, properties.deviceID, pDeviceType, vram);
    }
#endif

    pState->context.physicalDevice = pPhysicalDevices[bestIndex];
    pState->context.queueFamily = bestQueueFamily;

    logs_log(LOG_DEBUG, "Device %" PRIu32 " will be used with queue family %" PRIu32 ".", bestIndex, bestQueueFamily);

    free(pPhysicalDevices);
    return true;
}
#pragma region
#pragma region Instance Creation
/// @brief Enumerates the Vulkan instance's layer properties to determine if the validation layers are supported. If any aren't,
/// it will be treated as if none of them are (conservative)
static VkBool32 validationLayerSupportCheck(void)
{

    uint32_t layerCount = 0;
    VkResult result = vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    if (result != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to query instance layer property count (res=%d).", result);
        return VK_FALSE;
    }

    if (layerCount == 0)
        return VK_FALSE;

    VkLayerProperties *pLayers = malloc(sizeof(VkLayerProperties) * layerCount);

    if (!pLayers)
    {
        logs_log(LOG_ERROR, "Ran out of memory while allocating for layer properties!");
        free(pLayers);
        return VK_FALSE;
    }

    result = vkEnumerateInstanceLayerProperties(&layerCount, pLayers);

    if (result != VK_SUCCESS)
    {
        // Assume unsupported if VK_INCOMPLETE
        logs_log(LOG_ERROR, "Failed to query instance layer properties (res=%d).", result);
        return VK_FALSE;
    }

    for (size_t i = 0; i < s_REQUIRED_LAYERS_COUNT; ++i)
    {
        VkBool32 found = VK_FALSE;
        for (size_t j = 0; j < layerCount; ++j)
        {
            // Check if the string in the current validation layer is the same as the one in the available layer
            if (strcmp(s_pREQUIRED_LAYERS[i], pLayers[j].layerName) == 0)
            {
                found = VK_TRUE;
                break;
            }
        }
        if (!found)
        {
            free(pLayers);
            return VK_FALSE;
        }
    }

    free(pLayers);
    return VK_TRUE;
}

/// @brief Creates the actual Vulkan instance
static bool instance_create(State_t *pState)
{
    // Vulkan extensions required by GLFW
    uint32_t requiredExtensionsCount = 0;
    const char **ppREQUIRED_EXTENSIONS = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    // Add debug utils extension if validation is requested. Arbitrary extensions size
    char *pExtensions[256];
    memcpy(pExtensions, ppREQUIRED_EXTENSIONS, sizeof(char *) * requiredExtensionsCount);
    uint32_t extensionCount = requiredExtensionsCount;

    if (pState->config.vulkanValidation)
    {
        if (!validationLayerSupportCheck())
        {
            logs_log(LOG_WARN, "Vulkan validation layers requested but are not available.");
            pState->config.vulkanValidation = false;
        }
        else
            pExtensions[extensionCount++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    }

    const VkApplicationInfo APPLICATION_INFO = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = pState->config.vkAPIVersion,
        .pApplicationName = pState->config.pAPPLICATION_NAME,
        .pEngineName = pState->config.pENGINE_NAME,
    };

    const VkBool32 VERBOSE = true;
    const VkLayerSettingEXT LAYER_SETTING = {
        .pLayerName = "VK_LAYER_KHRONOS_validation",
        .pSettingName = "printf_verbose",
        .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,
        .valueCount = 1,
        .pValues = &VERBOSE,
    };

    VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .pNext = VK_NULL_HANDLE,
        .settingCount = 1,
        .pSettings = &LAYER_SETTING,
    };

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &APPLICATION_INFO,
        .enabledExtensionCount = extensionCount,
        .ppEnabledExtensionNames = pExtensions,
    };

    if (pState->config.vulkanValidation)
    {
        createInfo.enabledLayerCount = (uint32_t)s_REQUIRED_LAYERS_COUNT;
        createInfo.ppEnabledLayerNames = s_pREQUIRED_LAYERS;
        createInfo.pNext = &layerSettingsCreateInfo,
        logs_log(LOG_DEBUG, "Vulkan enabled with validation layers.");
    }

    if (vkCreateInstance(&createInfo, pState->context.pAllocator, &pState->context.instance) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to create the Vulkan instance!");
        return false;
    }

    return true;
}
#pragma endregion
#pragma region Const/Dest-ructor
void vulkan_init(State_t *pState)
{
#if defined(DEBUG)
    apiVersion_log();
#endif

    if (!instance_create(pState))
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a Vulkan instance.");

    if (!physicalDevice_select(pState) || !device_create(pState))
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a Vulkan physical device.");

    device_getQueue(pState);
}

void vulkan_instance_destroy(State_t *pState)
{
    vkDestroyDevice(pState->context.device, pState->context.pAllocator);
    vkDestroyInstance(pState->context.instance, pState->context.pAllocator);

    free(pState->renderer.anisotropicFilteringOptions);
    pState->renderer.anisotropicFilteringOptions = NULL;
}
#pragma endregion