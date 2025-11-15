#pragma once

#include <stdlib.h>
#include "rendering/buffers/buffers.h"

/// @brief Create the index buffer from the indicies. Adds this data to the existing index buffer
void indexBuffer_createFromData(State_t *pState, uint32_t *pIndices, const uint32_t INDEX_COUNT);

void indexBuffer_createEmpty(State_t *restrict pState, const uint32_t CAPACITY, VkBuffer *restrict pOutBuffer,
                             VkDeviceMemory *restrict pOutMemory);

void indexBuffer_updateFromData_Voxel(State_t *restrict pState, uint32_t *restrict pVertices, uint32_t indexCursor,
                                      VkBuffer buffer);

void indexBuffer_createFromData_Voxel(State_t *pState, uint32_t *pIndices, const uint32_t INDEX_COUNT,
                                      VkBuffer *pOutBuffer, VkDeviceMemory *pOutMemory);

/// @brief Destroy the index buffer
void indexBuffer_destroy(State_t *pState);