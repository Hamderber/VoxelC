#pragma once

#include "core/types/swapchain_t.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "input/types/input_t.h"

typedef struct
{
    // Vulkan
    Swapchain_t swapchain;
    VkSurfaceKHR surface;

    // GLFW
    GLFWwindow *pWindow;
    int frameBufferWidth;
    int frameBufferHeight;

    // Input
    Input_t input;
} Window_t;