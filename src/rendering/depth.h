#pragma once

#include "core/types/state_t.h"

VkFormat depth_formatGet(State_t *state);

void depthResourcesCreate(State_t *state);

void depthResourcesDestroy(State_t *state);