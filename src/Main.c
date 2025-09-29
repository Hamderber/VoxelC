#include "Toolkit.h"

int main(void)
{
    // Vulkan initialization:
    // 0 min https://youtu.be/-sdcRqpuOkU?si=TL-e2GkngVIV_h4c

    // Swapchain
    // https://www.youtube.com/watch?v=nSzQcyQTtRY

    Config_t config = {
        .applicationName = (PROGRAM_NAME, " Application"),
        .engineName = (PROGRAM_NAME, " Engine"),
        .windowTitle = PROGRAM_NAME,
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
