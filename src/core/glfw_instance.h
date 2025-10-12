#pragma once

#include <GLFW/glfw3.h>

void glfwi_logInfo(void);

void glfwi_framebufferSizeCallback(GLFWwindow *window, int frameBufferWidth, int frameBufferHeight);

void glfwi_init(void);
