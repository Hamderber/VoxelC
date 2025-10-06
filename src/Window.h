#pragma once

void windowPollEvents(const State_t *state)
{
    glfwPollEvents();
}

bool windowShouldClose(const State_t *state)
{
    return glfwWindowShouldClose(state->window.pWindow);
}

void surfaceCreate(State_t *state)
{
    // surface is just a cross-platform abstraction of the window (helps with vulkan)
    LOG_IF_ERROR(glfwCreateWindowSurface(state->context.instance, state->window.pWindow, state->context.pAllocator,
                                         &state->window.surface),
                 "Unable to create Vulkan window surface")
}

void surfaceDestroy(State_t *state)
{
    vkDestroySurfaceKHR(state->context.instance, state->window.surface, state->context.pAllocator);
}

/// @brief Destroy the swapchain's image views before allocating new ones. This is imporant because this method could be called
/// after the swapchain has already been made. If this happens, then there would be a memory leak from where the previous
/// swapchain image views were. The actual swapchain images themselves are deleted by the OS regardless.
/// @param state
void swapchainImagesFree(State_t *state)
{
    if (state->window.swapchain.handle != NULL && state->window.swapchain.pImageViews)
    {
        for (uint32_t i = 0U; i < state->window.swapchain.imageCount; i++)
        {
            vkDestroyImageView(state->context.device, state->window.swapchain.pImageViews[i], state->context.pAllocator);
        }

        free(state->window.swapchain.pImageViews);
        state->window.swapchain.pImageViews = NULL;

        free(state->window.swapchain.pImages);
        state->window.swapchain.pImages = NULL;
    }
}

VkSurfaceCapabilitiesKHR surfaceCapabilitiesGet(const Context_t *context, const Window_t *window)
{
    VkSurfaceCapabilitiesKHR capabilities;
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, window->surface, &capabilities),
                 "Failed to query physical device surface capabilities.");

    return capabilities;
}

VkSurfaceFormatKHR surfaceFormatsSelect(const Context_t *context, const Window_t *window)
{
    uint32_t formatCount;
    // null so that we just get the number of formats
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, window->surface, &formatCount, NULL),
                 "Failed to query physical device surface format count.")
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * formatCount);
    LOG_IF_ERROR(formats == NULL,
                 "Unable to allocate memory for Vulkan surface formats")
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, window->surface, &formatCount, formats),
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

    return format;
}

VkPresentModeKHR surfacePresentModesSelect(const Context_t *context, const Window_t *window)
{
    // See https://www.youtube.com/watch?v=nSzQcyQTtRY for the different present modes (visual examples)
    // Immedaite causes screen tearing (don't use) and I think Mailbox sounds the best. Unfortunately,
    // Mailbox isn't universally supported and is much more power-intensive (constant frame generation/discarding)
    // FIFO is required to exist on all platforms. All others are potentially not :(
    uint32_t presentModeCount;
    // null so that we just get the number of present modes
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice, window->surface, &presentModeCount, NULL),
                 "Failed to query physical device surface presentation modes.")
    VkPresentModeKHR *presentModes = malloc(sizeof(VkPresentModeKHR) * presentModeCount);
    LOG_IF_ERROR(presentModes == NULL,
                 "Unable to allocate memory for Vulkan present modes.")
    LOG_IF_ERROR(vkGetPhysicalDeviceSurfacePresentModesKHR(context->physicalDevice, window->surface, &presentModeCount, presentModes),
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

    return presentMode;
}

void swapchainImageAcquireNext(State_t *state)
{
    uint64_t imageTimeout = UINT64_MAX;

    LOG_IF_ERROR(vkWaitForFences(state->context.device, 1U, &state->renderer.inFlightFences[state->renderer.currentFrame], VK_TRUE,
                                 UINT64_MAX),
                 "Failed to wait for fences.")

    // It is worth considering to handle certain errors here eventially because not all errors mean the swapchain
    // failed completely
    VkResult result = vkAcquireNextImageKHR(state->context.device, state->window.swapchain.handle, imageTimeout,
                                            state->renderer.imageAcquiredSemaphores[state->renderer.currentFrame],
                                            VK_NULL_HANDLE, // Don't care about fence for this
                                            &state->window.swapchain.imageAcquiredIndex);

    // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
    // MUST be recreated immediately and presentation will just be attempted on the next frame.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        logger(LOG_WARN, "The swapchain is out of date and must be recreated! VkResult = %d", result);
        state->window.swapchain.recreate = true;

        // If this happens, does the CPU wait for a fence that will never compelte/reset?
        return;
    }
    else
    {
        LOG_IF_ERROR(result,
                     "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.")
    }
}

void swapchainImagePresent(State_t *state)
{
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &state->window.swapchain.imageAcquiredIndex,
        .swapchainCount = 1U,
        .pSwapchains = &state->window.swapchain.handle,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &state->renderer.renderFinishedSemaphores[state->renderer.currentFrame],
    };

    // Can't just catch this result with the error logger. Actually have to handle it.
    VkResult result = vkQueuePresentKHR(state->context.queue, &presentInfo);

    // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
    // MUST be recreated immediately and presentation will just be attempted on the next frame.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        logger(LOG_WARN, "The swapchain is out of date and must be recreated! VkResult = %d", result);
        state->window.swapchain.recreate = true;

        // If this happens, does the CPU wait for a fence that will never compelte/reset?
    }
    else
    {
        LOG_IF_ERROR(result,
                     "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.")
    }

    state->renderer.currentFrame = (state->renderer.currentFrame + 1) % state->config.maxFramesInFlight;
}

void swapchainImagesGet(State_t *state)
{
    // null so that we just get the number of formats
    LOG_IF_ERROR(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle,
                                         &state->window.swapchain.imageCount, NULL),
                 "Failed to query the number of images in the swapchain.")
    logger(LOG_INFO, "The swapchain will contain a buffer of %d images.", state->window.swapchain.imageCount);

    state->window.swapchain.pImages = malloc(sizeof(VkImage) * state->window.swapchain.imageCount);
    LOG_IF_ERROR(state->window.swapchain.pImages == NULL,
                 "Unable to allocate memory for swapchain images.")

    LOG_IF_ERROR(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle,
                                         &state->window.swapchain.imageCount, state->window.swapchain.pImages),
                 "Failed to get the images in the swapchain.")
}

void swapchainImageViewsCreate(State_t *state)
{
    state->window.swapchain.pImageViews = malloc(sizeof(VkImageView) * state->window.swapchain.imageCount);
    LOG_IF_ERROR(state->window.swapchain.pImageViews == NULL,
                 "Unable to allocate memory for swapchain image views.")

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
            .format = state->window.swapchain.format,
            .image = state->window.swapchain.pImages[i],
            // You could map the green component to red to do color shifting if desired or something
            .components = state->config.swapchainComponentMapping,
            .subresourceRange = subresourceRange,
            // The view type of the window itself. Obviously, a screen is 2D. Maybe 3D is for VR or something.
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
        };
        // Pass the address of the specific spot in the array of swapchain image views
        LOG_IF_ERROR(vkCreateImageView(state->context.device, &createInfo, state->context.pAllocator, &state->window.swapchain.pImageViews[i]),
                     "Failed to create swapchain image view %d", i)
    }
}

uint32_t swapchainGetMinImageCount(const Config_t *config, const VkPresentModeKHR presentMode, uint32_t min, uint32_t max)
{
    // It is good to add 1 to the minimum image count when using Mailbox so that the pipeline isn't blocked while the image is
    // still being presented. Also have to make sure that the max image count isn't exceeded.
    // This number is basically how many images should be able to be stored/queued/generated in the buffer while the screen is still
    // drawing. This has a *** HUGE *** impact on performance.

    if (config->swapchainBuffering != SWAPCHAIN_BUFFERING_DEFAULT)
    {
        // If swapchainBuffering isn't default (0) then assign it ourselves
        return config->swapchainBuffering;
    }

    // maxImageCount may not be assigned if GPU doesn't declare/is unbounded
    max = (max ? max : UINT32_MAX);

    if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR && min + 1U <= max)
        return min + 1; // Mailbox
    else
    {
        return min; // Not Mailbox (FIFO)
    }
}

void swapchainCreate(State_t *state)
{
    logger(LOG_INFO, "Creating the swapchain...");

    VkSurfaceCapabilitiesKHR capabilities = surfaceCapabilitiesGet(&state->context, &state->window);

    VkSurfaceFormatKHR surfaceFormat = surfaceFormatsSelect(&state->context, &state->window);

    logCapabilitiesInfo(capabilities);

    VkPresentModeKHR presentMode = surfacePresentModesSelect(&state->context, &state->window);

    // Prevent the image extend from somehow exceeding what the physical device is capable of
    VkExtent2D imageExtent = {
        .width = clamp_uint32_t(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = clamp_uint32_t(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };

    state->window.swapchain.imageExtent = imageExtent;
    state->window.swapchain.format = surfaceFormat.format;
    state->window.swapchain.colorSpace = surfaceFormat.colorSpace;

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
        .imageExtent = state->window.swapchain.imageExtent,
        .minImageCount = swapchainGetMinImageCount(&state->config, presentMode, capabilities.minImageCount, capabilities.maxImageCount),
        .imageFormat = state->window.swapchain.format,
        .imageColorSpace = state->window.swapchain.colorSpace,
        .presentMode = presentMode,
    };

    // Prevent memory leak by freeing previous swapchain's image views if they exist
    swapchainImagesFree(state);
    VkSwapchainKHR swapchain;

    LOG_IF_ERROR(vkCreateSwapchainKHR(state->context.device, &createInfo, state->context.pAllocator, &swapchain),
                 "Failed to create Vulkan swapchain!")

    // Even though the state's initial swapchain is obviously null, this sets us up to properly assign the new one (drivers/etc.)
    vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, state->context.pAllocator);
    state->window.swapchain.handle = swapchain;

    // state->window.swapchain.format = surfaceFormat.format;

    swapchainImagesGet(state);

    swapchainImageViewsCreate(state);
}

void swapchainDestroy(State_t *state)
{
    swapchainImagesFree(state);
    vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, state->context.pAllocator);
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
    state->window.pWindow = glfwCreateWindow(width, height, state->config.pWindowTitle, monitor, NULL);

    int frameBufferWidth;
    int frameBufferHeight;
    glfwGetFramebufferSize(state->window.pWindow, &frameBufferWidth, &frameBufferHeight);
    state->window.frameBufferWidth = frameBufferWidth;
    state->window.frameBufferHeight = frameBufferHeight;

    // This allows for the glfw window to keep a reference to the state. Thus, we don't have to make state a global variable.
    // This is necessary for things such as callback functions (see below in this method) where the callback function
    // otherwise wouldn't have access to the state.
    glfwSetWindowUserPointer(state->window.pWindow, state);

    // If the window changes size, call this function. There is a window-specific one, but the frame buffer one is better.
    // This allows for supporting retina displays and other screens that use subpixels (Vulkan sees subpixels as normal pixels).
    // For those types of displays, the window width/height and the frame buffer size would be different numbers. Also consider
    // that if the user has two monitors with only one being a retina display, they could drag the window from one screen to another
    // which would change the frame buffer size but NOT the actual window dimensions.
    glfwSetFramebufferSizeCallback(state->window.pWindow, glfwFramebufferSizeCallback);

    surfaceCreate(state);
    swapchainCreate(state);
}

void windowDestroy(State_t *state)
{
    swapchainDestroy(state);
    surfaceDestroy(state);
    glfwDestroyWindow(state->window.pWindow);
}