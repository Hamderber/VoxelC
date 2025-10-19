#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "cmath/cmath.h"
#include "core/types/swapchain_t.h"

typedef struct
{
    // Vulkan
    Swapchain_t swapchain;
    VkSurfaceKHR surface;

    // GLFW
    GLFWwindow *pWindow;
    int frameBufferWidth;
    int frameBufferHeight;
    int widthPrev;
    int heightPrev;
    int prevX;
    int prevY;
    bool fullscreen;
} Window_t;