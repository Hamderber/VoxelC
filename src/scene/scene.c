#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/types/scene_t.h"
#include "rendering/model_3d.h"
#include "rendering/types/renderModel_t.h"

// Grow strategy: x2, starting from 4
static inline uint32_t scene_next_capacity(uint32_t current)
{
    return current ? (current << 1) : 4u;
}

void scene_modelCreate(Scene_t *scene, RenderModel_t *mdl)
{
    if (!scene)
        return;

    if (scene->modelCount >= scene->modelCapacity)
    {
        uint32_t newCap = scene_next_capacity(scene->modelCapacity);
        void *newMem = realloc(scene->models, sizeof(RenderModel_t *) * newCap);
        if (!newMem)
            return;
        scene->models = newMem;
        scene->modelCapacity = newCap;
    }

    scene->models[scene->modelCount++] = mdl; // just store pointer
    logs_log(LOG_DEBUG, "scene_modelCreate: model added (count=%u, cap=%u)",
             scene->modelCount, scene->modelCapacity);
}

void scene_modelDestroy(State_t *state, RenderModel_t *mdl)
{
    if (!state || !mdl)
        return;

    const VkDevice device = state->context.device;
    const VkAllocationCallbacks *alloc = state->context.pAllocator;

    // vertex and index buffers and handled by the renderer destruction

    mdl->indexCount = 0;

    if (mdl->textureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(device, mdl->textureSampler, alloc);
        mdl->textureSampler = VK_NULL_HANDLE;
    }
    if (mdl->textureView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(device, mdl->textureView, alloc);
        mdl->textureView = VK_NULL_HANDLE;
    }
    if (mdl->textureImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(device, mdl->textureImage, alloc);
        mdl->textureImage = VK_NULL_HANDLE;
    }
    if (mdl->textureMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device, mdl->textureMemory, alloc);
        mdl->textureMemory = VK_NULL_HANDLE;
    }

    if (mdl->descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device, mdl->descriptorPool, alloc);
        mdl->descriptorPool = VK_NULL_HANDLE;
    }

    if (mdl->pDescriptorSets)
    {
        free(mdl->pDescriptorSets);
        mdl->pDescriptorSets = NULL;
    }

    memset(&mdl->modelMatrix, 0, sizeof(mdl->modelMatrix));

    free(mdl);
}

void scene_destroy(State_t *state)
{
    if (!state || !state->scene.models)
        return;

    vkDeviceWaitIdle(state->context.device);

    size_t count = state->scene.modelCount;
    for (size_t i = 0; i < count; ++i)
    {
        if (!state->scene.models[i])
            continue;

        scene_modelDestroy(state, state->scene.models[i]);
    }

    free(state->scene.models);
    state->scene.models = NULL;
    state->scene.modelCapacity = 0;
    state->scene.modelCount = 0;
}

void scene_model_createAll(State_t *pState)
{
    RenderModel_t *mdl = m3d_load(pState,
                                  MODEL_PATH "complex_test.glb",
                                  RESOURCE_TEXTURE_PATH "complex_test.png");

    mdl->modelMatrix = cmath_mat_setTranslation(MAT4_IDENTITY, VEC3_LEFT);

    scene_modelCreate(&pState->scene, mdl);
}