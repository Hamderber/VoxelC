#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"

/// @brief Record the command buffer for the current frame
void commandBuffer_record(State_t *pState);

/// @brief Submit the command buffer for the current frame
void commandBuffer_submit(State_t *pState);

/// @brief One-shot begin of the command buffer
VkCommandBuffer commandBuffer_singleTime_start(State_t *pState);

/// @brief One-shot end of the command buffer
void commandBuffer_singleTime_end(State_t *pState, VkCommandBuffer commandBuffer);

/// @brief Create the command buffers
void commandBuffer_create(State_t *pState);