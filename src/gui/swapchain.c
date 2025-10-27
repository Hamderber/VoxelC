#include <stdlib.h>
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"
#include "gui/window.h"

/// @brief Presents the current swapchain image to the window (adds to queue) and increments the current frame
/// @param state
void sc_imagePresent(State_t *state)
{
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &state->window.swapchain.imageAcquiredIndex,
        .swapchainCount = 1U,
        .pSwapchains = &state->window.swapchain.handle,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &state->renderer.renderFinishedSemaphores[state->renderer.currentFrame],
    };

    // Can't just catch this result with the error logs_log. Actually have to handle it.
    VkResult result = vkQueuePresentKHR(state->context.graphicsQueue, &presentInfo);

    // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
    // MUST be recreated immediately and presentation will just be attempted on the next frame.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        logs_log(LOG_WARN, "The swapchain is out of date and must be recreated! VkResult = %d", result);
        state->window.swapchain.recreate = true;

        // If this happens, does the CPU wait for a fence that will never compelte/reset?
    }
    else
    {
        logs_logIfError(result,
                        "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.");
    }

    // Advance to the next frame-in-flight slot
    state->renderer.currentFrame = (state->renderer.currentFrame + 1) % state->config.maxFramesInFlight;
}

/// @brief Gets the next image in the swapchain and assigns it in the state's renderer
/// @param state
void sc_imageAcquireNext(State_t *state)
{
    const uint64_t imageTimeout = UINT64_MAX;
    const uint32_t frameIndex = state->renderer.currentFrame;

    // Wait for the fence for this frame to ensure it’s not still in use
    logs_logIfError(vkWaitForFences(state->context.device, 1U,
                                    &state->renderer.inFlightFences[frameIndex],
                                    VK_TRUE, UINT64_MAX),
                    "Failed to wait for in-flight fence (frame %u)", frameIndex);

    VkResult result = vkAcquireNextImageKHR(state->context.device,
                                            state->window.swapchain.handle,
                                            imageTimeout,
                                            state->renderer.imageAcquiredSemaphores[frameIndex],
                                            VK_NULL_HANDLE,
                                            &state->window.swapchain.imageAcquiredIndex);

    // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
    // MUST be recreated immediately and presentation will just be attempted on the next frame.
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        logs_log(LOG_WARN, "The swapchain is out of date or suboptimal and must be recreated! VkResult = %d", result);
        state->window.swapchain.recreate = true;
        return;
    }
    else
    {
        logs_logIfError(result,
                        "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.");
    }
}

/// @brief Destroy the swapchain's image views before allocating new ones. This is imporant because this method could be called
/// after the swapchain has already been made. If this happens, then there would be a memory leak from where the previous
/// swapchain image views were. The actual swapchain images themselves are deleted by the OS regardless.
/// @param state
static void sc_imagesFree(State_t *state)
{
    if (state->window.swapchain.handle != NULL && state->window.swapchain.pImageViews)
    {
        for (uint32_t i = 0; i < state->window.swapchain.imageCount; i++)
        {
            vkDestroyImageView(state->context.device, state->window.swapchain.pImageViews[i], state->context.pAllocator);
        }

        free(state->window.swapchain.pImageViews);
        state->window.swapchain.pImageViews = NULL;

        free(state->window.swapchain.pImages);
        state->window.swapchain.pImages = NULL;
    }
}

/// @brief Allocates the memory and assigns the references for the swapchain images
/// @param state
static void sc_imagesGet(State_t *state)
{
    // null so that we just get the number of formats
    logs_logIfError(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle,
                                            &state->window.swapchain.imageCount, NULL),
                    "Failed to query the number of images in the swapchain.");
    logs_log(LOG_DEBUG, "The swapchain will contain a buffer of %d images.", state->window.swapchain.imageCount);

    state->window.swapchain.pImages = malloc(sizeof(VkImage) * state->window.swapchain.imageCount);
    logs_logIfError(state->window.swapchain.pImages == NULL,
                    "Unable to allocate memory for swapchain images.");

    logs_logIfError(vkGetSwapchainImagesKHR(state->context.device, state->window.swapchain.handle,
                                            &state->window.swapchain.imageCount, state->window.swapchain.pImages),
                    "Failed to get the images in the swapchain.");
}

/// @brief Creates the image views for each swapchain image
/// @param state
static void sc_imageViewsCreate(State_t *state)
{
    state->window.swapchain.pImageViews = malloc(sizeof(VkImageView) * state->window.swapchain.imageCount);
    logs_logIfError(state->window.swapchain.pImageViews == NULL,
                    "Unable to allocate memory for swapchain image views.");

    // Allows for supporting multiple image view layers for the same image etc.
    VkImageSubresourceRange subresourceRange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .layerCount = 1,
        .levelCount = 1,
    };

    for (uint32_t i = 0; i < state->window.swapchain.imageCount; i++)
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
        logs_logIfError(vkCreateImageView(state->context.device, &createInfo, state->context.pAllocator, &state->window.swapchain.pImageViews[i]),
                        "Failed to create swapchain image view %d", i);
    }
}

/// @brief Gets the minimum number of images available to be put in the swapchain for the present mode
/// @param config
/// @param presentMode
/// @param min
/// @param max
/// @return uint32_t
static uint32_t sc_minImageCountGet(const AppConfig_t *config, const VkPresentModeKHR presentMode, uint32_t min, uint32_t max)
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

/// @brief Frees all images in flight
/// @param state
static void sc_imagesInFlightFree(State_t *state)
{
    if (state->renderer.imagesInFlight == NULL)
    {
        return;
    }

    for (uint32_t i = 0; i < state->window.swapchain.imageCount; ++i)
    {
        state->renderer.imagesInFlight[i] = VK_NULL_HANDLE;
    }
    free(state->renderer.imagesInFlight);
    state->renderer.imagesInFlight = NULL;
}

#pragma region Const/Dest-ructor
void swapchain_create(State_t *pState)
{
    // TODO: crashHandler for swapchain failure and finish refactoring this next
    logs_log(LOG_DEBUG, "Creating the swapchain...");

    VkSurfaceCapabilitiesKHR capabilities = window_surfaceCapabilities_get(&pState->context, &pState->window);

    VkSurfaceFormatKHR surfaceFormat = window_surfaceFormats_select(&pState->context, &pState->window);

#if defined(DEBUG)
    vulkan_deviceCapabilities_log(pState->context.physicalDeviceSupportedFeatures, capabilities);
#endif

    VkPresentModeKHR presentMode = window_surfacePresentModes_select(&pState->config, &pState->context, &pState->window);

    // Prevent the image extend from somehow exceeding what the physical device is capable of
    VkExtent2D imageExtent = {
        .width = cmath_clampU32t(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = cmath_clampU32t(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };

    if (imageExtent.width == 0 || imageExtent.height == 0)
    {
        logs_log(LOG_WARN, "Skipping swapchain recreation due to minimized window.");
        return;
    }

    pState->window.swapchain.imageExtent = imageExtent;
    pState->window.swapchain.format = surfaceFormat.format;
    pState->window.swapchain.colorSpace = surfaceFormat.colorSpace;

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = pState->window.surface,
        .queueFamilyIndexCount = 1U,                         // Only using 1 queue family
        .pQueueFamilyIndices = &pState->context.queueFamily, // Skip making an array for this since only using 1 queue family
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
        .oldSwapchain = pState->window.swapchain.handle ? pState->window.swapchain.handle : NULL,
        // Only really applicable to mobile devices. Phone screens etc. obviously have to support rotating 90/180 but this same
        // support is often not included with desktop/laptop GPUs. Identity just means keep the image the same.
        // Current transform is almost certainly VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
        .preTransform = capabilities.currentTransform,
        .imageExtent = pState->window.swapchain.imageExtent,
        .minImageCount = sc_minImageCountGet(&pState->config, presentMode, capabilities.minImageCount, capabilities.maxImageCount),
        .imageFormat = pState->window.swapchain.format,
        .imageColorSpace = pState->window.swapchain.colorSpace,
        .presentMode = presentMode,
    };

    // Prevent memory leak by freeing previous swapchain's image views if they exist
    sc_imagesFree(pState);
    VkSwapchainKHR swapchain;

    logs_logIfError(vkCreateSwapchainKHR(pState->context.device, &createInfo, pState->context.pAllocator, &swapchain),
                    "Failed to create Vulkan swapchain!");

    // Even though the state's initial swapchain is obviously null, this sets us up to properly assign the new one (drivers/etc.)
    vkDestroySwapchainKHR(pState->context.device, pState->window.swapchain.handle, pState->context.pAllocator);
    pState->window.swapchain.handle = swapchain;

    sc_imagesGet(pState);

    // Free old images in flight
    sc_imagesInFlightFree(pState);

    // Allocate the memory for the images in flight
    pState->renderer.imagesInFlight = malloc(sizeof(VkFence) * pState->window.swapchain.imageCount);
    logs_logIfError(pState->renderer.imagesInFlight == NULL,
                    "Failed to allocate memory for images in flight!");
    for (uint32_t i = 0; i < pState->window.swapchain.imageCount; ++i)
        pState->renderer.imagesInFlight[i] = VK_NULL_HANDLE;

    sc_imageViewsCreate(pState);
}

void swapchain_destroy(State_t *state)
{
    sc_imagesInFlightFree(state);
    sc_imagesFree(state);
    vkDestroySwapchainKHR(state->context.device, state->window.swapchain.handle, state->context.pAllocator);
}
#pragma endregion