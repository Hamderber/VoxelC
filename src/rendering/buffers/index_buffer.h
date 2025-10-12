#pragma once

#include <stdlib.h>
#include "rendering/buffers/buffers.h"

void indexBufferCreateFromData(State_t *state, uint16_t *indices, uint32_t indexCount);

void indexBufferDestroy(State_t *state);