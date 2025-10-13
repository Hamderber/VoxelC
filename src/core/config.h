#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>

typedef enum
{
    AF_1 = 1,
    AF_2 = 2,
    AF_4 = 4,
    AF_8 = 8,
    AF_16 = 16,
} AnisotropicFilteringOptions_t;

typedef enum
{
    SWAPCHAIN_BUFFERING_DEFAULT = 0,
    SWAPCHAIN_BUFFERING_SINGLE = 1,
    SWAPCHAIN_BUFFERING_DOUBLE = 2,
    SWAPCHAIN_BUFFERING_TRIPLE = 3,
    SWAPCHAIN_BUFFERING_QUADRUPLE = 4,
} SwapchainBuffering_t;

typedef struct
{
    const char *pApplicationName;
    const char *pEngineName;
    const char *pWindowTitle;
    uint32_t vkAPIVersion;
    SwapchainBuffering_t swapchainBuffering;
    VkComponentMapping swapchainComponentMapping;
    int windowWidth;
    int windowHeight;
    bool windowResizable;
    bool windowFullscreen;
    bool vsync;
    int anisotropy;
    VkCullModeFlagBits cullModeMask;
    VkFrontFace vertexWindingDirection;
    // Do not allow this value to be changed at runtime. Will cause memory issues with the amount of semaphors and fences.
    uint32_t maxFramesInFlight;
    double fixedTimeStep; // ex: 1.0 / 50.0
    uint32_t maxPhysicsFrameDelay;
    bool vulkanValidation;
    // Size in pixels of each subtexture on the texture atlas. Minecraft is 16px
    uint32_t subtextureSize;

    char assetPath[256];
    char shaderPath[256];
} Config_t;

void cfg_destroy(void);

Config_t cfg_init(void);