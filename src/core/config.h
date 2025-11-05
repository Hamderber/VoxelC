#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include "rendering/types/swapchainBuffering_t.h"

typedef struct
{
    const char *pAPPLICATION_NAME;
    const char *pENGINE_NAME;
    const char *pWINDOW_TITLE;
    uint32_t vkAPIVersion;
    SwapchainBuffering_t swapchainBuffering;
    VkComponentMapping swapchainComponentMapping;
    int windowWidth;
    int windowHeight;
    bool windowResizable;
    bool windowFullscreen;
    float cameraFOV;
    float cameraFarClippingPlane;
    float cameraNearClippingPlane;
    uint32_t chunkRenderDistance;
    bool vsync;
    int anisotropy;
    bool resetCursorOnMenuExit;
    VkCullModeFlagBits cullModeMask;
    VkFrontFace vertexWindingDirection;
    // Do not allow this value to be changed at runtime. Will cause memory issues with the amount of semaphors and fences.
    uint32_t maxFramesInFlight;
    double fixedTimeStep; // ex: 1.0 / 50.0
    uint32_t maxPhysicsFrameDelay;
    bool vulkanValidation;
    // Size in pixels of each subtexture on the texture atlas. Minecraft is 16px
    uint32_t subtextureSize;
    double mouseSensitivity;
    uint32_t atlasPaddingPx;
} AppConfig_t;

struct State_t;
/// @brief Loads configs and creates the directory and/or configs not found
void config_init(struct State_t *pState);
