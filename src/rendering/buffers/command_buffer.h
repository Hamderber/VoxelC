#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"

void commandBufferAllocate(State_t *state);

void commandBufferRecord(State_t *state);

void commandBufferSubmit(State_t *state);

VkCommandBuffer commandBufferSingleTimeBegin(State_t *state);

void commandBufferSingleTimeEnd(State_t *state, VkCommandBuffer commandBuffer);