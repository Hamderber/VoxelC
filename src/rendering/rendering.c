#pragma once

#include <string.h>
#include "stb_image.h"
#include "cgltf.h"
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

void rend_wireframeToggle(State_t *state)
{
    if (!state->context.physicalDeviceEnabledFeatures.fillModeNonSolid)
    {
        logs_log(LOG_WARN, "Attempted to toggle wireframe visiblity but the device doesn't support that feature.");
        state->renderer.activeGraphicsPipeline = GRAPHICS_PIPELINE_FILL;
        return;
    }

    // is the active graphics pipeline fill? If yes set to wirefame otherwise (that means it is already wireframe)
    // set it to fill
    state->renderer.activeGraphicsPipeline =
        state->renderer.activeGraphicsPipeline ==
                GRAPHICS_PIPELINE_FILL
            ? GRAPHICS_PIPELINE_WIREFRAME
            : GRAPHICS_PIPELINE_FILL;
}

EventResult_t rend_onWireframeTogglePress(struct State_t *state, Event_t *event, void *ctx)
{
    ctx = NULL;
    if (event == NULL)
    {
        return EVENT_RESULT_ERROR;
    }

    if (event->type == EVENT_TYPE_INPUT_MAPPED && event->data.inputMapped != NULL)
    {
        for (size_t i = 0; i < event->data.inputMapped->actionCount; i++)
        {
            if (event->data.inputMapped->inputActions[i].actionState == CTX_INPUT_ACTION_START)
            {
                switch (event->data.inputMapped->inputActions[i].action)
                {
                case INPUT_ACTION_WIREFRAME_TOGGLE:
                    logs_log(LOG_DEBUG, "Wireframe toggle (pressed)");
                    rend_wireframeToggle(state);
                    return EVENT_RESULT_CONSUME;
                    break;
                }
            }
        }
    }

    return EVENT_RESULT_PASS;
}

void rend_recreate(State_t *state)
{
    // Make sure the GPU is idle. This could be a queue wait plus fence if more wait accuracy is needed
    vkDeviceWaitIdle(state->context.device);
    window_waitForValidFramebuffer(&state->window);

    // Reset mouse input so that the camera does't jerk when the window is resized
    mouse_inputReset(state);

    depthResourcesDestroy(state);
    framebuffersDestroy(state);

    swapchain_create(state);
    depthResourcesCreate(state);
    framebuffersCreate(state);

    logs_log(LOG_DEBUG, "Re-created the swapchain.");
    state->window.swapchain.recreate = false;
}

void rend_present(State_t *state)
{
    updateUniformBuffer(state);
    sc_imageAcquireNext(state);
    commandBufferRecord(state);
    commandBufferSubmit(state);
    sc_imagePresent(state);
}

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
void rend_create(State_t *state)
{
    // Must exist before anything that references it
    rp_create(state);
    descriptorSetLayoutCreate(state);
    // Create all graphics pipelines and set the active (default) one
    state->renderer.activeGraphicsPipeline = GRAPHICS_PIPELINE_FILL;
    gp_create(state, GRAPHICS_PIPELINE_FILL, GRAPHICS_PIPELINE_TARGET_MODEL);
    gp_create(state, GRAPHICS_PIPELINE_WIREFRAME, GRAPHICS_PIPELINE_TARGET_MODEL);

    // Needed for all staging/copies and one-time commands
    commandPoolCreate(state);

    // // so m3d_load can remap UVs using pAtlasRegions
    // // loads atlas.png, creates VkImage + memory
    // atlasTextureImageCreate(state);
    // // view for the atlas image
    // atlasTextureViewImageCreate(state);
    // // sampler used by descriptors
    // tex_samplerCreate(state);
    // // call the dynamic atlas region builder *after* you know width/height
    // // fills renderer.pAtlasRegions[0..N)
    // state->renderer.pAtlasRegions = atlasCreate(state->renderer.pAtlasRegions, state->renderer.atlasRegionCount,
    //                                             state->renderer.atlasWidthInTiles, state->renderer.atlasHeightInTiles);

    // Atlas resources FIRST (image -> view -> sampler -> regions)
    atlasTextureImageCreate(state);
    atlasTextureViewImageCreate(state);
    tex_samplerCreate(state);

    // Build gutter-aware regions. We know the tilePx and gutterPx here.
    const uint32_t tilePx = state->config.subtextureSize;
    const uint32_t gutterPx = state->config.atlasGutterPx; // SAME value used above
    const uint32_t tilesX = state->renderer.atlasWidthInTiles;
    const uint32_t tilesY = state->renderer.atlasHeightInTiles;
    const uint32_t stride = tilePx + 2 * gutterPx;

    state->renderer.pAtlasRegions = atlasCreate(
        state->renderer.pAtlasRegions,
        state->renderer.atlasRegionCount,
        tilesX, tilesY,
        tilesX * stride, tilesY * stride,
        tilePx, gutterPx);

    // Model upload that may use pAtlasRegions for UV remap
    // builds vertex/index buffers
    // m3d_load(state);

    // Per-frame resources that descriptors will point at
    // UBOs (needed before descriptorSetsCreate)
    uniformBuffersCreate(state);

    // Depth & framebuffers must happen before recording
    // creates depth image + view
    depthResourcesCreate(state);
    // needs swapchain views + depth view + render pass
    framebuffersCreate(state);

    // Descriptor pool/sets AFTER UBO + atlas view + sampler exist
    descriptorPoolCreate(state);
    // writes UBO & combined image sampler
    descriptorSetsCreate(state);

    // Command buffers and sync last
    commandBufferAllocate(state);
    syncObjectsCreate(state);
}

void rend_destroy(State_t *state)
{
    // The GPU could be working on stuff for the renderer in parallel, meaning the renderer could be
    // destroyed while the GPU is still working. We should wait for the GPU to finish its current tasks.
    logs_logIfError(vkQueueWaitIdle(state->context.graphicsQueue),
                    "Failed to wait for the Vulkan graphicsQueue to be idle.");
    // Stop GPU use first
    syncObjectsDestroy(state);

    // Descriptors before destroying underlying resources
    descriptorSetsDestroy(state);
    descriptorPoolDestroy(state);

    // Framebuffer graph
    framebuffersDestroy(state);
    depthResourcesDestroy(state);

    // Per-model buffers
    uniformBuffersDestroy(state);
    indexBufferDestroy(state);
    vertexBufferDestroy(state);
    m3d_destroy(state);

    // Atlas GPU resources
    tex_samplerDestroy(state);
    atlasTextureImageViewDestroy(&state->context, &state->renderer);
    atlasTextureImageDestroy(state);
    // frees pAtlasRegions (heap)
    // AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles
    atlasDestroy(state->renderer.pAtlasRegions);

    // Command pool after any single-time buffers etc. are destroyed
    commandPoolDestroy(state);

    // Pipeline objects last
    // Destroy all graphics pipeline types
    gp_destroy(state, GRAPHICS_PIPELINE_FILL);
    gp_destroy(state, GRAPHICS_PIPELINE_WIREFRAME);
    rp_destroy(state);
}
