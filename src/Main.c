#include "Toolkit.h"

int main(void)
{
    // Vulkan initialization:
    // 0 min https://www.youtube.com/watch?v=6hFJvlYbtF0

    VkComponentMapping componentMapping = {
        // RGBA is still red/blue/green/alpha. Identity is keep it default but it could be .._A/etc
        // Identity = 0 so this could be omitted, but explicit declaration is better visually
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    Config_t config = {
        .applicationName = (PROGRAM_NAME, " Application"),
        .engineName = (PROGRAM_NAME, " Engine"),
        .windowTitle = PROGRAM_NAME,
        .windowWidth = 720,
        .windowHeight = 480,
        // If the window gets resized, the swapchain MUST be recreated
        .windowResizable = true,
        .windowFullscreen = false,
        .vkAPIVersion = VK_API_VERSION_1_4,
        .swapchainComponentMapping = componentMapping,
        // default means that it will be auto-assigned during swapchain creation
        .swapchainBuffering = SWAPCHAIN_BUFFERING_DEFAULT};

    State_t state = {
        .config = config};

    init(&state);
    loop(&state);
    cleanup(&state);

    return EXIT_SUCCESS;
}
