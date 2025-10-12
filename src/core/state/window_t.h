#pragma once

#include "swapchain_t.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

typedef struct
{
    // Vulkan
    Swapchain_t swapchain;
    VkSurfaceKHR surface;

    // GLFW
    GLFWwindow *pWindow;
    int frameBufferWidth;
    int frameBufferHeight;
} Window_t;