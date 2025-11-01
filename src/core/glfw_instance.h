#pragma once

#include <GLFW/glfw3.h>

/// @brief This is requried even if window-is-resizable is hard-coded to false! This is because the user can
/// always minimize the window, which also causes a frame buffer size change.
void glfwInstance_framebuffer_sizeCallback(GLFWwindow *pWindow, int frameBufferWidth, int frameBufferHeight);

/// @brief Initalizes the GLFW instance.
void glfwInstance_init(void);
