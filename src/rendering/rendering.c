#pragma region Includes
#include <string.h>
#include "stb_image.h"
#include "core/logs.h"
#include "cgltf.h"
#include "main.h"
#include "rendering/texture.h"
#include "rendering/render_pass.h"
#include "rendering/depth.h"
#include "rendering/graphics_pipeline.h"
#include "rendering/buffers/buffers.h"
#include "rendering/model_3d.h"
#include "rendering/buffers/command_buffer.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/buffers/frame_buffer.h"
#include "rendering/atlas_texture.h"
#include "rendering/pools/command_pool.h"
#include "rendering/image.h"
#include "rendering/buffers/uniform_buffer.h"
#include "rendering/pools/descriptor_pool.h"
#include "rendering/syncObjects.h"
#include "gui/window.h"
#include "gui/swapchain.h"
#include "gui/mouse.h"
#include "rendering/types/graphicsPipeline_t.h"
#include "input/input.h"
#include "events/eventBus.h"
#include "events/eventTypes.h"
#include "input/types/inputActionQuery_t.h"
#include "core/crash_handler.h"
#include "rendering/types/renderModel_t.h"
#include "scene/scene.h"
#include "rendering/model_3d.h"
#pragma endregion
#pragma region Wireframe
/// @brief Toggle wireframe pipeline
static void wireframe_toggle(State_t *pState)
{
    if (!pState->context.physicalDeviceEnabledFeatures.fillModeNonSolid)
    {
        logs_log(LOG_WARN, "Attempted to toggle wireframe visiblity but the device doesn't support that feature.");
        pState->renderer.activeGraphicsPipeline = GRAPHICS_PIPELINE_MODEL_FILL;
        return;
    }

    GraphicsPipeline_t current = pState->renderer.activeGraphicsPipeline;
    GraphicsPipeline_t target = current == GRAPHICS_PIPELINE_MODEL_FILL ? GRAPHICS_PIPELINE_WIREFRAME : GRAPHICS_PIPELINE_MODEL_FILL;

    pState->renderer.activeGraphicsPipeline = target;
}

EventResult_t rendering_wireframe_onTogglePress(State_t *pState, Event_t *pEvent, void *pCtx)
{
    pCtx;
    if (pEvent == NULL)
        return EVENT_RESULT_ERROR;

    const InputActionQuery_t pQUERY[] = {
        {.mapping = INPUT_ACTION_WIREFRAME_TOGGLE,
         .actionCtx = CTX_INPUT_ACTION_START}};

    InputAction_t pQueryResult[sizeof pQUERY / sizeof pQUERY[0]];
    const size_t SIZE = input_inputAction_matchQuery(pEvent, pQUERY, sizeof pQUERY / sizeof pQUERY[0], pQueryResult);

    if (SIZE > 0)
        for (size_t i = 0; i < SIZE; i++)
        {
            const InputAction_t ACTION = pQueryResult[i];

            switch (ACTION.action)
            {
            case INPUT_ACTION_WIREFRAME_TOGGLE:
                logs_log(LOG_DEBUG, "Wireframe toggle (pressed)");
                wireframe_toggle(pState);
                return EVENT_RESULT_CONSUME;
                break;
            }
        }

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Presentation
void rendering_present(State_t *pState)
{
    uniformBuffer_update(pState);

    swapchain_image_acquireNext(pState);

    commandBuffer_record(pState);
    commandBuffer_submit(pState);

    swapchain_image_present(pState);
}
#pragma endregion
#pragma region Models
/// @brief Destroy UBO setup
static void models_destroy(State_t *pState)
{
    uniformBuffers_destroy(pState);
    indexBuffer_destroy(pState);
    vertexBuffer_destroy(pState);
}
#pragma endregion
#pragma region Create
void rendering_recreate(State_t *pState)
{
    // Make sure the GPU is idle. This could be a queue wait plus fence if more wait accuracy is needed
    vkDeviceWaitIdle(pState->context.device);
    window_waitForValidFramebuffer(&pState->window);

    // Reset mouse input so that the camera does't jerk when the window is resized
    mouse_inputReset(pState);

    depthResources_destroy(pState);
    framebuffers_destroy(pState);

    swapchain_create(pState);
    depthResources_create(pState);
    framebuffers_create(pState);

    pState->window.swapchain.recreate = false;
}

void rendering_create(State_t *pState)
{
    // Must exist before anything that references it
    renderpass_create(pState);

    // Must be created before the descriptor pool
    descriptorSet_layout_create(pState);

    // Create all graphics pipelines and set the active (default) one
    pState->renderer.activeGraphicsPipeline = GRAPHICS_PIPELINE_MODEL_FILL;
    graphicsPipeline_createAll(pState);

    // Needed for all staging/copies and one-time commands
    commandPool_create(pState);

    // Voxel texture atlas
    atlasTexture_create(pState);

    // Per-frame resources that descriptors will point at
    uniformBuffers_create(pState);

    // Depth & framebuffers must happen before recording
    // creates depth image + view
    depthResources_create(pState);
    // needs swapchain views + depth view + render pass
    framebuffers_create(pState);

    // Descriptor pool/sets AFTER UBO + atlas view + sampler exist
    descriptorPool_create(pState);

    // Command buffers and sync last
    commandBuffer_create(pState);
    syncObjects_create(pState);
}
#pragma endregion
#pragma region Destroy
void rendering_destroy(State_t *pState)
{
    // The GPU could be working on stuff for the renderer in parallel, meaning the renderer could be
    // destroyed while the GPU is still working. We should wait for the GPU to finish its current tasks.
    if (vkQueueWaitIdle(pState->context.graphicsQueue) != VK_SUCCESS)
    {
        logs_log(LOG_ERROR, "Failed to wait for the Vulkan graphics queue to be idle.");
        crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue due to a fatal CPU/GPU desync.");
    }

    // Stop GPU use first
    syncObjects_destroy(pState);

    // Descriptors before destroying underlying resources
    descriptorPool_destroy(pState);

    // Framebuffer graph
    framebuffers_destroy(pState);
    depthResources_destroy(pState);

    models_destroy(pState);

    atlasTexture_destroy(pState);

    // Command pool after any single-time buffers etc. are destroyed
    commandPool_destroy(pState);

    // Pipeline objects last
    graphicsPipeline_destroyAll(pState);
    renderpass_destroy(pState);
}
#pragma endregion
