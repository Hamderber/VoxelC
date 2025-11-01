#pragma once

#include <GLFW/glfw3.h>
#include <stdbool.h>
#include "core/types/state_t.h"

/// @brief Toggle window fullscreen
void window_fullscreen_toggle(State_t *pState, bool toggle);

/// @brief Wrapper for polling GLFW events
static inline void window_events_poll(void) { glfwPollEvents(); }

/// @brief Gets the should-window-close flag for the state's GLFW window (thread unsafe)
static inline bool win_shouldClose(const Window_t *pWINDOW) { return glfwWindowShouldClose(pWINDOW->pWindow); }

/// @brief Intentionally hangs while the window is minimized
void window_waitForValidFramebuffer(Window_t *pWindow);

/// @brief Gets Vulkan window surface capabilities
VkSurfaceCapabilitiesKHR window_surfaceCapabilities_get(const Context_t *pCONTEXT, const Window_t *pWINDOW);

/// @brief Selects the best surface format from the available options
VkSurfaceFormatKHR window_surfaceFormats_select(const Context_t *pCONTEXT, const Window_t *pWINDOW);

/// @brief Selects the preferred surface present mode
VkPresentModeKHR window_surfacePresentModes_select(const AppConfig_t *pCONFIG, const Context_t *pCONTEXT, const Window_t *pWINDOW);

/// @brief Creates the GLFW window
void window_init(State_t *pState);

/// @brief Destroys the window, surface, and swapchain
void window_destroy(State_t *pState);