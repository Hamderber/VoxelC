#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "core/types/state_t.h"

void win_pollEvents(void);

bool win_shouldClose(Window_t *window);

void win_waitForValidFramebuffer(Window_t *window);

VkSurfaceCapabilitiesKHR win_surfaceCapabilitiesGet(const Context_t *context, const Window_t *window);

VkSurfaceFormatKHR win_surfaceFormatsSelect(const Context_t *context, const Window_t *window);

VkPresentModeKHR win_surfacePresentModesSelect(const AppConfig_t *config, const Context_t *context, const Window_t *window);

void win_create(State_t *state);

void win_destroy(State_t *state);