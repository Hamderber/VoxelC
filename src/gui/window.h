#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

void win_pollEvents(const State_t *state);

bool win_shouldClose(const State_t *state);

void win_waitForValidFramebuffer(State_t *state);

VkSurfaceCapabilitiesKHR win_surfaceCapabilitiesGet(const Context_t *context, const Window_t *window);

VkSurfaceFormatKHR win_surfaceFormatsSelect(const Context_t *context, const Window_t *window);

VkPresentModeKHR win_surfacePresentModesSelect(const Context_t *context, const Window_t *window);

void win_create(State_t *state);

void win_destroy(State_t *state);