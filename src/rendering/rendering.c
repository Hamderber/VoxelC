#pragma region Includes
#include <string.h>
#include "stb_image.h"
#include "cgltf.h"
#include "main.h"
#include "rendering/texture.h"
#include "rendering/voxel.h"
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
        pState->renderer.activeGraphicsPipeline = GRAPHICS_PIPELINE_FILL;
        return;
    }

    GraphicsPipeline_t current = pState->renderer.activeGraphicsPipeline;
    GraphicsPipeline_t target = current == GRAPHICS_PIPELINE_FILL ? GRAPHICS_PIPELINE_WIREFRAME : GRAPHICS_PIPELINE_FILL;

    pState->renderer.activeGraphicsPipeline = target;
}

EventResult_t rendering_wireframe_onTogglePress(State_t *pState, Event_t *pEvent, void *pCtx)
{
    pCtx;
    if (pEvent == NULL)
        return EVENT_RESULT_ERROR;

    const InputActionQuery_t pQUERY[] = {
        {
            .mapping = INPUT_ACTION_WIREFRAME_TOGGLE,
            .actionCtx = CTX_INPUT_ACTION_START,
        },
    };

    InputAction_t pQUERY_RESULT[sizeof pQUERY / sizeof pQUERY[0]];
    const size_t SIZE = input_inputAction_matchQuery(pEvent, pQUERY, sizeof pQUERY / sizeof pQUERY[0], pQUERY_RESULT);

    if (SIZE > 0)
        for (size_t i = 0; i < SIZE; i++)
        {
            const InputAction_t ACTION = pQUERY_RESULT[i];

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
    updateUniformBuffer(pState);

    swapchain_image_acquireNext(pState);

    commandBufferRecord(pState);

    commandBufferSubmit(pState);

    swapchain_image_present(pState);
}
#pragma endregion
#pragma region Models
/// @brief Destroy UBO setup
static void models_destroy(State_t *pState)
{
    uniformBuffersDestroy(pState);
    indexBufferDestroy(pState);
    vertexBufferDestroy(pState);
}
#pragma endregion
#pragma region Atlas Texture
/// @brief Creates the atlas texture
static void atlasTexture_create(State_t *pState)
{
    // Atlas resources FIRST (image -> view -> sampler -> regions)
    atlasTextureImageCreate(pState);
    atlasTextureViewImageCreate(pState);
    tex_samplerCreate(pState);

    // Build a black gutter around each tile of the atlas. Gutter prevents oversampling during aniosotropic filtering
    const uint32_t TILE_PX = pState->config.subtextureSize;
    const uint32_t GUTTER_PX = pState->config.atlasGutterPx;
    const uint32_t TILES_X = pState->renderer.atlasWidthInTiles;
    const uint32_t TILES_Y = pState->renderer.atlasHeightInTiles;
    const uint32_t STRIDE = TILE_PX + 2 * GUTTER_PX;

    pState->renderer.pAtlasRegions = atlasCreate(
        pState->renderer.pAtlasRegions,
        pState->renderer.atlasRegionCount,
        TILES_X, TILES_Y,
        TILES_X * STRIDE, TILES_Y * STRIDE,
        TILE_PX, GUTTER_PX);
}

/// @brief Destroys the atlas texture
static void atlasTexture_destroy(State_t *pState)
{
    tex_samplerDestroy(pState);
    atlasTextureImageViewDestroy(&pState->context, &pState->renderer);
    atlasTextureImageDestroy(pState);
    atlasDestroy(pState->renderer.pAtlasRegions);
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

    depthResourcesDestroy(pState);
    framebuffersDestroy(pState);

    swapchain_create(pState);
    depthResourcesCreate(pState);
    framebuffersCreate(pState);

    pState->window.swapchain.recreate = false;
}

void rendering_create(State_t *pState)
{
    // Must exist before anything that references it
    renderpass_create(pState);
    descriptorSetLayoutCreate(pState);
    // Create all graphics pipelines and set the active (default) one
    pState->renderer.activeGraphicsPipeline = GRAPHICS_PIPELINE_FILL;
    gp_create(pState, GRAPHICS_PIPELINE_FILL, GRAPHICS_PIPELINE_TARGET_MODEL);
    gp_create(pState, GRAPHICS_PIPELINE_WIREFRAME, GRAPHICS_PIPELINE_TARGET_MODEL);

    // Needed for all staging/copies and one-time commands
    commandPoolCreate(pState);

    atlasTexture_create(pState);

    // Per-frame resources that descriptors will point at
    // UBOs (needed before descriptorSetsCreate)
    uniformBuffersCreate(pState);

    // Depth & framebuffers must happen before recording
    // creates depth image + view
    depthResourcesCreate(pState);
    // needs swapchain views + depth view + render pass
    framebuffersCreate(pState);

    // Descriptor pool/sets AFTER UBO + atlas view + sampler exist
    descriptorPoolCreate(pState);
    // writes UBO & combined image sampler
    descriptorSetsCreate(pState);

    // Command buffers and sync last
    commandBufferAllocate(pState);
    syncObjectsCreate(pState);
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
        crashHandler_crash_graceful("The program cannot continue due to a fatal CPU/GPU desync.");
    }

    // Stop GPU use first
    syncObjectsDestroy(pState);

    // Descriptors before destroying underlying resources
    descriptorSetsDestroy(pState);
    descriptorPoolDestroy(pState);

    // Framebuffer graph
    framebuffersDestroy(pState);
    depthResourcesDestroy(pState);

    models_destroy(pState);

    atlasTexture_destroy(pState);

    // Command pool after any single-time buffers etc. are destroyed
    commandPoolDestroy(pState);

    // Pipeline objects last
    // Destroy all graphics pipeline types
    gp_destroy(pState, GRAPHICS_PIPELINE_FILL);
    gp_destroy(pState, GRAPHICS_PIPELINE_WIREFRAME);
    renderpass_destroy(pState);
}
#pragma endregion
