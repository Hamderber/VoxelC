#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

void win_pollEvents(void);

bool win_shouldClose(Window_t *window);

void win_waitForValidFramebuffer(Window_t *window);

VkSurfaceCapabilitiesKHR win_surfaceCapabilitiesGet(const Context_t *context, const Window_t *window);

VkSurfaceFormatKHR win_surfaceFormatsSelect(const Context_t *context, const Window_t *window);

VkPresentModeKHR win_surfacePresentModesSelect(const Context_t *context, const Window_t *window);

void win_create(void *state, Window_t *window, Config_t *config);

void win_destroy(State_t *state);