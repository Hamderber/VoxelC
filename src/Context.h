#pragma once

void instanceCreate(State_t *state)
{
    uint32_t requiredExtensionsCount;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    const VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = state->config.vkAPIVersion,
        .pApplicationName = state->config.pApplicationName,
        .pEngineName = state->config.pEngineName};

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = requiredExtensionsCount,
        .ppEnabledExtensionNames = requiredExtensions};

    LOG_IF_ERROR(vkCreateInstance(&createInfo, state->context.pAllocator, &state->context.instance),
                 "Couldn't create Vulkan instance.")
}

void physicalDeviceSelect(State_t *state)
{
    uint32_t count;

    VkResult enumeratePhysicalDevicesResult = vkEnumeratePhysicalDevices(state->context.instance, &count, NULL);
    // VK_INCOMPLETE isn't considered a failure. If the user somehow has multiple physical devices but one isn't
    // available for use, then VK_INCOMPLETE will be returned to just signify that not all physical devices were
    // enumerated. (Ex: dual GPUs) https://registry.khronos.org/vulkan/specs/latest/man/html/vkEnumeratePhysicalDevices.html
    LOG_IF_ERROR(enumeratePhysicalDevicesResult != VK_INCOMPLETE && enumeratePhysicalDevicesResult != VK_SUCCESS,
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

    LOG_IF_ERROR(vkCreateDevice(state->context.physicalDevice, &createInfo, state->context.pAllocator, &state->context.device),
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

void contextCreate(State_t *state)
{
    instanceCreate(state);
    physicalDeviceSelect(state);
    queueFamilySelect(state);
    deviceCreate(state);
    queueGet(state);
}

void contextDestroy(State_t *state)
{
    vkDestroyDevice(state->context.device, state->context.pAllocator);
    vkDestroyInstance(state->context.instance, state->context.pAllocator);
}

#pragma once

void instanceCreate(State_t *state)
{
    uint32_t requiredExtensionsCount;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    const VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .apiVersion = state->config.vkAPIVersion,
        .pApplicationName = state->config.pApplicationName,
        .pEngineName = state->config.pEngineName};

    const VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = requiredExtensionsCount,
        .ppEnabledExtensionNames = requiredExtensions};

    LOG_IF_ERROR(vkCreateInstance(&createInfo, state->context.pAllocator, &state->context.instance),
                 "Couldn't create Vulkan instance.")
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

    LOG_IF_ERROR(vkCreateDevice(state->context.physicalDevice, &createInfo, state->context.pAllocator, &state->context.device),
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

void contextCreate(State_t *state)
{
    instanceCreate(state);
    physicalDeviceSelect(state);
    queueFamilySelect(state);
    deviceCreate(state);
    queueGet(state);
}

void contextDestroy(State_t *state)
{
    vkDestroyDevice(state->context.device, state->context.pAllocator);
    vkDestroyInstance(state->context.instance, state->context.pAllocator);
}
