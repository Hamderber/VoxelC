#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "rendering/types/renderModel_t.h"

static VkDescriptorSet allocateDescriptorSet(VkDevice device, VkDescriptorPool pool, VkDescriptorSetLayout layout);

static bool modelDescriptorPoolCreate(State_t *state, RenderModel_t *model);

bool modelDescriptorSetsCreate(State_t *state, RenderModel_t *model);

void modelDescriptorSetsDestroy(State_t *state, RenderModel_t *model);

RenderModel_t *m3d_load(State_t *state, const char *glbPath, const char *texturePath);
