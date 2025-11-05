#pragma once

#include <stdlib.h>
#include "rendering/buffers/buffers.h"

/// @brief Create the index buffer from the indicies. Adds this data to the existing index buffer
void indexBuffer_createFromData(State_t *pState, uint32_t *pIndices, const uint32_t INDEX_COUNT);

/// @brief Destroy the index buffer
void indexBuffer_destroy(State_t *pState);