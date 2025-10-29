#pragma region Includes
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <inttypes.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/vk_instance.h"
#include "gui/window.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Presentation
void swapchain_image_acquireNext(State_t *pState)
{
    const uint64_t IMAGE_TIMEOUT = UINT64_MAX;
    const uint32_t FRAME_INDEX = pState->renderer.currentFrame;

    int crashLine = 0;
    do
    {
        // Wait for the fence for this frame to ensure it’s not still in use
        bool waitAll = VK_TRUE;
        uint32_t fenceCount = 1;
        if (vkWaitForFences(pState->context.device, fenceCount, &pState->renderer.inFlightFences[FRAME_INDEX], waitAll,
                            IMAGE_TIMEOUT) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to wait for in-flight fence (frame %" PRIu32 ")", FRAME_INDEX);
            break;
        }

        VkFence fence = VK_NULL_HANDLE;
        VkResult result = vkAcquireNextImageKHR(pState->context.device, pState->window.swapchain.handle, IMAGE_TIMEOUT,
                                                pState->renderer.imageAcquiredSemaphores[FRAME_INDEX], fence,
                                                &pState->window.swapchain.imageAcquiredIndex);

        // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
        // MUST be recreated immediately and presentation will just be attempted on the next frame.
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            logs_log(LOG_DEBUG, "The swapchain is out of date or suboptimal and must be recreated! VkResult = %d", (int)result);
            pState->window.swapchain.recreate = true;
            return;
        }
        else if (result != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.");
            break;
        }

        return;
    } while (0);

    crashHandler_crash_graceful(
        CRASH_LOCATION_LINE(crashLine),
        "The program cannot continue without being able to successfully present images from the swapchain.");
}

void swapchain_image_present(State_t *pState)
{
    const VkPresentInfoKHR PRESENT_INFO = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pImageIndices = &pState->window.swapchain.imageAcquiredIndex,
        .swapchainCount = 1,
        .pSwapchains = &pState->window.swapchain.handle,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &pState->renderer.renderFinishedSemaphores[pState->renderer.currentFrame],
    };

    do
    {
        VkResult result = vkQueuePresentKHR(pState->context.graphicsQueue, &PRESENT_INFO);
        // If the swapchain gets out of date, it is impossible to present the image and it will hang. The swapchain
        // MUST be recreated immediately and presentation will just be attempted on the next frame.
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            logs_log(LOG_DEBUG, "The swapchain is out of date and must be recreated! VkResult = %d", (int)result);
            pState->window.swapchain.recreate = true;
        }
        else if (result != VK_SUCCESS)
        {
            logs_log(LOG_ERROR, "Failed to present the next image in the swapchain! This is NOT due to the swapchain being out of date.");
            break;
        }

        // Advance to the next frame-in-flight slot
        pState->renderer.currentFrame = (pState->renderer.currentFrame + 1) % pState->config.maxFramesInFlight;
        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without being able to successfully present images from the swapchain.");
}
#pragma endregion
#pragma region Image Views
/// @brief Creates the image views for each swapchain image
static void imageViews_create(State_t *pState)
{
    for (uint32_t i = 0; i < pState->window.swapchain.imageCount; ++i)
        pState->renderer.imagesInFlight[i] = VK_NULL_HANDLE;

    pState->window.swapchain.pImageViews = NULL;
    do
    {
        pState->window.swapchain.pImageViews = malloc(sizeof(VkImageView) * pState->window.swapchain.imageCount);
        if (!pState->window.swapchain.pImageViews)
        {
            logs_log(LOG_ERROR, "Failed to allocate memory for the swapchain image views!");
            break;
        }

        // Allows for supporting multiple image view layers for the same image etc.
        const VkImageSubresourceRange SUB_RESOURCE_RANGE = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .layerCount = 1,
            .levelCount = 1,
        };

        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .format = pState->window.swapchain.format,
            // You could map the green component to red to do color shifting if desired or something
            .components = pState->config.swapchainComponentMapping,
            .subresourceRange = SUB_RESOURCE_RANGE,
            // The view type of the window itself. Obviously, a screen is 2D. Maybe 3D is for VR or something.
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
        };

        bool fail = false;
        for (uint32_t i = 0; i < pState->window.swapchain.imageCount && !fail; i++)
        {
            // This is defined in the loop because of using i for the swapchain image
            createInfo.image = pState->window.swapchain.pImages[i];
            // Pass the address of the specific spot in the array of swapchain image views
            if (vkCreateImageView(pState->context.device, &createInfo, pState->context.pAllocator,
                                  &pState->window.swapchain.pImageViews[i]) != VK_SUCCESS)
            {
                logs_log(LOG_ERROR, "Failed to create swapchain image view %" PRIu32 "!", i);
                fail = true;
            }
        }

        if (!fail)
            return;
    } while (0);

    free(pState->window.swapchain.pImageViews);
    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without having image views to display to the window.");
}
#pragma endregion
#pragma region Frames-in-flight
/// @brief Frees all images in flight
static void imagesInFlight_free(State_t *pState)
{
    if (pState->renderer.imagesInFlight == NULL)
        return;

    for (uint32_t i = 0; i < pState->window.swapchain.imageCount; ++i)
        pState->renderer.imagesInFlight[i] = VK_NULL_HANDLE;

    free(pState->renderer.imagesInFlight);
    pState->renderer.imagesInFlight = NULL;
}
#pragma endregion
#pragma region Image
/// @brief Gets the minimum number of images available to be put in the swapchain for the present mode
static uint32_t minImageCount_get(const AppConfig_t *pCONFIG, const VkPresentModeKHR PRESENT_MODE, uint32_t min, uint32_t max)
{
    // It is good to add 1 to the minimum image count when using Mailbox so that the pipeline isn't blocked while the image is
    // still being presented. Also have to make sure that the max image count isn't exceeded.
    // This number is basically how many images should be able to be stored/queued/generated in the buffer while the screen is still
    // drawing. This has a *** HUGE *** impact on performance.

    if (pCONFIG->swapchainBuffering != SWAPCHAIN_BUFFERING_DEFAULT)
        // If swapchainBuffering isn't default (0) then assign it ourselves
        return pCONFIG->swapchainBuffering;

    // maxImageCount may not be assigned if GPU doesn't declare/is unbounded
    max = (max ? max : UINT32_MAX);

    if (PRESENT_MODE == VK_PRESENT_MODE_MAILBOX_KHR && min + 1 <= max)
        // Mailbox
        return min + 1;
    else
        // Not Mailbox (FIFO)
        return min;
}

/// @brief Destroy the swapchain's image views before allocating new ones. This is imporant because this method could be called
/// after the swapchain has already been made. If this happens, then there would be a memory leak from where the previous
/// swapchain image views were. The actual swapchain images themselves are deleted by the OS regardless.
static void images_free(State_t *pState)
{
    if (pState->window.swapchain.handle != NULL && pState->window.swapchain.pImageViews)
    {
        for (uint32_t i = 0; i < pState->window.swapchain.imageCount; i++)
            vkDestroyImageView(pState->context.device, pState->window.swapchain.pImageViews[i], pState->context.pAllocator);

        free(pState->window.swapchain.pImageViews);
        pState->window.swapchain.pImageViews = NULL;

        free(pState->window.swapchain.pImages);
        pState->window.swapchain.pImages = NULL;
    }
}

/// @brief Allocates the memory and assigns the references for the swapchain images
static void images_allocate(State_t *pState)
{
    pState->window.swapchain.pImages = NULL;

    int crashLine = 0;
    do
    {
        if (vkGetSwapchainImagesKHR(pState->context.device, pState->window.swapchain.handle,
                                    &pState->window.swapchain.imageCount, NULL) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to query the number of images in the swapchain!");
            break;
        }

        logs_log(LOG_DEBUG, "The swapchain will contain a buffer of %d images.", pState->window.swapchain.imageCount);
        pState->window.swapchain.pImages = malloc(sizeof(VkImage) * pState->window.swapchain.imageCount);

        if (!pState->window.swapchain.pImages)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for the swapchain images!");
            break;
        }

        if (vkGetSwapchainImagesKHR(pState->context.device, pState->window.swapchain.handle,
                                    &pState->window.swapchain.imageCount, pState->window.swapchain.pImages) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to get the images for the swapchain!");
            break;
        }

        return;
    } while (0);

    free(pState->window.swapchain.pImages);
    crashHandler_crash_graceful(
        CRASH_LOCATION_LINE(crashLine),
        "The program cannot continue without having a storage location for the swapchain's images.");
}
#pragma endregion
#pragma region Create Info
/// @brief Returns if the swapchain should be created based off of image extents. (Don't recreate the swapchain for a minimuzed)
/// window.
static bool createInfo_get(State_t *pState, VkSwapchainCreateInfoKHR *pCreateInfo)
{
    VkSurfaceCapabilitiesKHR capabilities = window_surfaceCapabilities_get(&pState->context, &pState->window);
    VkSurfaceFormatKHR surfaceFormat = window_surfaceFormats_select(&pState->context, &pState->window);
    VkPresentModeKHR presentMode = window_surfacePresentModes_select(&pState->config, &pState->context, &pState->window);

#if defined(DEBUG)
    vulkan_deviceCapabilities_log(pState->context.physicalDeviceSupportedFeatures, capabilities);
#endif

    // Prevent the image extend from somehow exceeding what the physical device is capable of
    VkExtent2D imageExtent = {
        .width = cmath_clampU32t(capabilities.currentExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        .height = cmath_clampU32t(capabilities.currentExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height),
    };

    if (imageExtent.width == 0 || imageExtent.height == 0)
    {
        logs_log(LOG_DEBUG, "Skipping swapchain recreation due to minimized window.");
        return false;
    }

    pState->window.swapchain.imageExtent = imageExtent;
    pState->window.swapchain.format = surfaceFormat.format;
    pState->window.swapchain.colorSpace = surfaceFormat.colorSpace;

    *pCreateInfo = (VkSwapchainCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = pState->window.surface,
        // Only using 1 queue family
        .queueFamilyIndexCount = 1,
        // Skip making an array for this since only using 1 queue family
        .pQueueFamilyIndices = &pState->context.queueFamily,
        // Don't render pixels that are obscured by some other program window (ex: Chrome). If using some complex
        // post-processing or other things, this should be false because it requires the whole image to be processed regardless.
        .clipped = true,
        // If this were not opaque, the window itself could have some type of alpha transparency feature that would cause the OS to
        // render some window behind this one through the alpha transparency sections. Obviously not applicable here, though
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        // >1 would be for something like a VR headset or something where you have the red and blue screen for 3D or something
        .imageArrayLayers = 1,
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
        .minImageCount = minImageCount_get(&pState->config, presentMode, capabilities.minImageCount, capabilities.maxImageCount),
        .imageFormat = pState->window.swapchain.format,
        .imageColorSpace = pState->window.swapchain.colorSpace,
        .presentMode = presentMode,
    };

    return true;
}
#pragma endregion
#pragma region Const/Dest-ructor
void swapchain_create(State_t *pState)
{
    // False if window is 0x0 (minimized)
    VkSwapchainCreateInfoKHR createInfo = {0};
    if (!createInfo_get(pState, &createInfo))
        return;

    // Don't just call swapchain_destroy here. The old swapchain handle is passed to the new create info so that the images
    // still in-flight can be processed.
    logs_log(LOG_DEBUG, "(Re)creating the swapchain...");

    VkSwapchainKHR swapchain = {0};
    if (vkCreateSwapchainKHR(pState->context.device, &createInfo, pState->context.pAllocator, &swapchain) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to create the swapchain!");
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a swapchain to use for window presentation.");
        return;
    }

    // Prevent memory leaks by freeing previous swapchain's image views if they exist
    images_free(pState);
    imagesInFlight_free(pState);
    vkDestroySwapchainKHR(pState->context.device, pState->window.swapchain.handle, pState->context.pAllocator);
    pState->window.swapchain.handle = swapchain;

    images_allocate(pState);

    pState->renderer.imagesInFlight = malloc(sizeof(VkFence) * pState->window.swapchain.imageCount);
    if (!pState->renderer.imagesInFlight)
    {
        logs_log(LOG_ERROR, "Failed to allocate memory for the swapchain's images-in-flight!");
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a place to store the images for display.");
        return;
    }

    imageViews_create(pState);
}

void swapchain_destroy(State_t *pState)
{
    imagesInFlight_free(pState);
    images_free(pState);
    vkDestroySwapchainKHR(pState->context.device, pState->window.swapchain.handle, pState->context.pAllocator);
}
#pragma endregion