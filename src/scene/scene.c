#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/types/scene_t.h"
#include "rendering/model_3d.h"
#include "rendering/types/renderModel_t.h"
#include "scene/SceneModelInstance_t.h"
#include "core/random.h"

void scene_drawModels(State_t *pState, VkCommandBuffer *pCmd, VkPipelineLayout *pPipelineLayout)
{
    for (uint32_t i = 0; i < pState->scene.instCount; ++i)
    {
        SceneModelInstance_t m = pState->scene.pModelInstances[i];

        // access violation
        vkCmdBindDescriptorSets(*pCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pPipelineLayout,
                                0, 1, &m.pModel->pDescriptorSets[pState->renderer.currentFrame],
                                0, NULL);

        VkBuffer modelVBs[] = {m.pModel->vertexBuffer};
        VkDeviceSize offs[] = {0};
        vkCmdBindVertexBuffers(*pCmd, 0, 1, modelVBs, offs);
        // 16 limits verticies to 65535 (consider once making own models and having a check?)
        vkCmdBindIndexBuffer(*pCmd, m.pModel->indexBuffer, 0, VK_INDEX_TYPE_UINT16);

        vkCmdPushConstants(*pCmd, *pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)sizeof(Mat4c_t), &m.modelMatrix);
        // // Not using instanced rendering so just 1 instance with nothing for the offset
        vkCmdDrawIndexed(*pCmd, m.pModel->indexCount, 1, 0, 0, 0);
    }
}

// Grow strategy: x2, starting from 4
static inline uint32_t scene_next_capacity(uint32_t current)
{
    return current ? (current << 1) : 4u;
}

void scene_modelAdd(Scene_t *pScene, RenderModel_t *pModel, Mat4c_t matrix)
{
    if (!pScene)
        return;

    if (pScene->instCount >= pScene->instCap)
    {
        uint32_t newCap = scene_next_capacity(pScene->instCap);
        void *pNewMem = realloc(pScene->pModelInstances, sizeof(SceneModelInstance_t) * newCap);
        if (!pNewMem)
            return;
        pScene->pModelInstances = pNewMem;
        pScene->instCap = newCap;
    }

    pScene->pModelInstances[pScene->instCount++] = (SceneModelInstance_t){
        .modelMatrix = matrix,
        .pModel = pModel};
}

void scene_modelCreate(Scene_t *pScene, RenderModel_t *pMdl)
{
    if (!pScene)
        return;

    if (pScene->modelCount >= pScene->modelCap)
    {
        uint32_t newCap = scene_next_capacity(pScene->modelCap);
        void *pNewMem = realloc(pScene->ppModels, sizeof(RenderModel_t *) * newCap);
        if (!pNewMem)
            return;
        pScene->ppModels = pNewMem;
        pScene->modelCap = newCap;
    }

    pScene->ppModels[pScene->modelCount++] = pMdl; // just store pointer
    logs_log(LOG_DEBUG, "scene_modelCreate: model added (count=%u, cap=%u)",
             pScene->modelCount, pScene->modelCap);
}

void scene_modelDestroy(State_t *pState, RenderModel_t *pMdl)
{
    if (!pState || !pMdl)
        return;

    const VkDevice DEVICE = pState->context.device;
    const VkAllocationCallbacks *pAllocator = pState->context.pAllocator;

    // vertex and index buffers and handled by the renderer destruction

    pMdl->indexCount = 0;

    if (pMdl->textureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(DEVICE, pMdl->textureSampler, pAllocator);
        pMdl->textureSampler = VK_NULL_HANDLE;
    }

    if (pMdl->textureView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(DEVICE, pMdl->textureView, pAllocator);
        pMdl->textureView = VK_NULL_HANDLE;
    }

    if (pMdl->textureImage != VK_NULL_HANDLE)
    {
        vkDestroyImage(DEVICE, pMdl->textureImage, pAllocator);
        pMdl->textureImage = VK_NULL_HANDLE;
    }

    if (pMdl->textureMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(DEVICE, pMdl->textureMemory, pAllocator);
        pMdl->textureMemory = VK_NULL_HANDLE;
    }

    if (pMdl->descriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(DEVICE, pMdl->descriptorPool, pAllocator);
        pMdl->descriptorPool = VK_NULL_HANDLE;
    }

    if (pMdl->pDescriptorSets)
    {
        free(pMdl->pDescriptorSets);
        pMdl->pDescriptorSets = NULL;
    }

    memset(&pMdl->modelMatrix, 0, sizeof(pMdl->modelMatrix));
    free(pMdl);
}

void scene_destroy(State_t *state)
{
    if (!state || !state->scene.ppModels)
        return;

    vkDeviceWaitIdle(state->context.device);

    size_t count = state->scene.modelCount;
    for (size_t i = 0; i < count; ++i)
    {
        if (!state->scene.ppModels[i])
            continue;

        scene_modelDestroy(state, state->scene.ppModels[i]);
    }

    free(state->scene.ppModels);
    state->scene.ppModels = NULL;
    state->scene.modelCap = 0;
    state->scene.modelCount = 0;
}

void scene_debug_rotateAllRandom(Scene_t *pScene, float rad)
{
    for (uint32_t i = 0; i < pScene->instCount; i++)
    {
        Mat4c_t mat = cmath_mat_rotate(pScene->pModelInstances[i].modelMatrix, rad, VEC3_UP);
        pScene->pModelInstances[i].modelMatrix = mat;
    }
}

void scene_model_createAll(State_t *pState)
{
    RenderModel_t *mdl = m3d_load(pState,
                                  MODEL_PATH "complex_test.glb",
                                  RESOURCE_TEXTURE_PATH "complex_test.png");

    scene_modelCreate(&pState->scene, mdl);

    for (int i = 0; i < 250; i++)
        scene_modelAdd(&pState->scene, mdl,
                       cmath_mat_setTranslation(random_mat_rot(), random_vec3f(-50.0F, 50.0F, -50.0F, 50.0F, -50.0F, 50.0F)));
}