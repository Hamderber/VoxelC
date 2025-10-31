#pragma once

#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "core/types/scene_t.h"
#include "rendering/types/renderModel_t.h"

void scene_debug_rotateAllRandom(Scene_t *pScene, float rad);
void scene_drawModels(State_t *pState, VkCommandBuffer *pCmd, VkPipelineLayout *pPipelineLayout);
void scene_modelCreate(Scene_t *scene, RenderModel_t *mdl);
void scene_destroy(State_t *state);
void scene_model_createAll(State_t *pState);