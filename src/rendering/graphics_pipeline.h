#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "rendering/types/graphicsPipeline_t.h"

void gp_create(State_t *state, GraphicsPipeline_t graphicsPipeline, GraphicsPipelineTarget_t target);

void gp_destroy(State_t *state, GraphicsPipeline_t graphicsPipeline);