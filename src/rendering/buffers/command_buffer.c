#pragma region Includes
#include "core/logs.h"
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <inttypes.h>
#include "core/types/state_t.h"
#include "rendering/types/graphicsPipeline_t.h"
#include "rendering/types/renderChunk_t.h"
#include "rendering/types/renderModel_t.h"
#include "core/crash_handler.h"
#include "rendering/types/graphicsPipeline_t.h"
#include "rendering/chunk/chunkRendering.h"
#include "scene/scene.h"
#pragma endregion
#pragma region Binding
static void pipeline_bind(State_t *pState, VkCommandBuffer *pCmd, VkPipelineLayout *pPipelineLayout, const GraphicsTarget_t TARGET)
{
    if (!pState || !pCmd || !pPipelineLayout)
        return;

    // Bind the render pipeline to the active graphics pipeline (instead of compute)
    VkPipeline pPipelines[GRAPHICS_PIPELINE_COUNT];
    VkPipelineLayout pPipelineLayouts[GRAPHICS_PIPELINE_COUNT];
    switch (pState->renderer.activeGraphicsPipeline)
    {
    case GRAPHICS_PIPELINE_VOXEL_FILL:
    case GRAPHICS_PIPELINE_MODEL_FILL:
        pPipelines[GRAPHICS_TARGET_MODEL] = pState->renderer.graphicsPipelineFillModel;
        pPipelineLayouts[GRAPHICS_TARGET_MODEL] = pState->renderer.pipelineLayoutFillModel;
        pPipelines[GRAPHICS_TARGET_VOXEL] = pState->renderer.graphicsPipelineFillVoxel;
        pPipelineLayouts[GRAPHICS_TARGET_VOXEL] = pState->renderer.pipelineLayoutFillVoxel;
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        pPipelines[GRAPHICS_TARGET_MODEL] = pState->renderer.graphicsPipelineWireframeModel;
        pPipelineLayouts[GRAPHICS_TARGET_MODEL] = pState->renderer.pipelineLayoutWireframeModel;
        pPipelines[GRAPHICS_TARGET_VOXEL] = pState->renderer.graphicsPipelineWireframeVoxel;
        pPipelineLayouts[GRAPHICS_TARGET_VOXEL] = pState->renderer.pipelineLayoutWireframeVoxel;
        break;
    }

    *pPipelineLayout = pPipelineLayouts[TARGET];
    vkCmdBindPipeline(*pCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pPipelines[TARGET]);

    const uint32_t FIRST_DESC_SET = 0;
    const uint32_t DESC_SET_COUNT = 1;
    const uint32_t DYNAMIC_OFFSET_COUNT = 0;
    const uint32_t *pDYNAMIC_OFFSETS = VK_NULL_HANDLE;
    vkCmdBindDescriptorSets(*pCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, *pPipelineLayout,
                            FIRST_DESC_SET, DESC_SET_COUNT, &pState->renderer.pDescriptorSets[pState->renderer.currentFrame],
                            DYNAMIC_OFFSET_COUNT, pDYNAMIC_OFFSETS);
}
#pragma endregion
#pragma region Record
void commandBuffer_record(State_t *pState)
{
    // Skip this recording frame if the swapchain will be/is being recreated (avoids null pointers)
    if (pState->window.swapchain.recreate)
        return;

    const VkRect2D RENDER_AREA = {.extent = pState->window.swapchain.imageExtent};

    // Which color values to clear when using the clear operation defined in the attachments of the render pass.
    // Order of clear values must be equal to order of attachments
    const VkClearValue pCLEAR_VALUES[] = {
        // Clears image but leaves a background color (black)
        {.color.float32 = {0.0F, 0.0F, 0.0F, 0.0F}},
        // Resets the depth stencil
        {.depthStencil = {1.0F, 0}}};

    const uint32_t CLEAR_VALUE_COUNT = sizeof(pCLEAR_VALUES) / sizeof(*pCLEAR_VALUES);

    const VkCommandBufferBeginInfo CMD_BEGIN_INFO = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0};

    const uint32_t FRAME_INDEX = pState->renderer.currentFrame;

    int crashLine = 0;
    do
    {
        // Avoid access violations
        if (!pState->renderer.pFramebuffers || pState->window.swapchain.imageAcquiredIndex >= pState->renderer.framebufferCount)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted access violation during frame buffer access while recording the command buffer!");
            break;
        }

        VkCommandBuffer cmd = pState->renderer.pCommandBuffers[FRAME_INDEX];
        const uint32_t FLAGS = 0;
        if (vkResetCommandBuffer(cmd, FLAGS) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to reset the command buffer for frame %" PRIu32 "!", FRAME_INDEX);
            break;
        }

        if (vkBeginCommandBuffer(cmd, &CMD_BEGIN_INFO) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to begin command buffer for frame %" PRIu32 "!", FRAME_INDEX);
            break;
        }

        // ALL vkCmd functions (commands) MUST go between the begin and end command buffer functions (obviously)
        const VkRenderPassBeginInfo RP_BEGIN_INFO = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = pState->renderer.pRenderPass,
            .framebuffer = pState->renderer.pFramebuffers[pState->window.swapchain.imageAcquiredIndex],
            .renderArea = RENDER_AREA,
            .clearValueCount = CLEAR_VALUE_COUNT,
            .pClearValues = pCLEAR_VALUES,
        };

#pragma region Render Pass Begin
        // If secondary command buffers are used, use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
        vkCmdBeginRenderPass(cmd, &RP_BEGIN_INFO, VK_SUBPASS_CONTENTS_INLINE);

        // Required because the viewport is dynamic (resizeable)
        const VkViewport VIEWPORT = {
            .x = 0.0F,
            .y = 0.0F,
            .width = (float)pState->window.swapchain.imageExtent.width,
            .height = (float)pState->window.swapchain.imageExtent.height,
            .minDepth = 0.0F,
            .maxDepth = 1.0F};
        const uint32_t FIRST_VIEWPORT = 0;
        const uint32_t VIEWPORT_COUNT = 1;
        vkCmdSetViewport(cmd, FIRST_VIEWPORT, VIEWPORT_COUNT, &VIEWPORT);

        const VkRect2D SCISSOR = {
            .offset = {0, 0},
            .extent = pState->window.swapchain.imageExtent};
        const uint32_t FIRST_SCISSOR = 0;
        const uint32_t SCISSOR_COUNT = 1;
        vkCmdSetScissor(cmd, FIRST_SCISSOR, SCISSOR_COUNT, &SCISSOR);

        // DRAW ! ! ! ! !
        {
            // Models
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
            pipeline_bind(pState, &cmd, &pipelineLayout, GRAPHICS_TARGET_MODEL);
            scene_drawModels(pState, &cmd, &pipelineLayout);
        }

        {
            // Voxel
            VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
            pipeline_bind(pState, &cmd, &pipelineLayout, GRAPHICS_TARGET_VOXEL);
            chunk_drawChunks(pState, &cmd, &pipelineLayout);
        }

#pragma endregion
#pragma region Render Pass End
        // Must end the render pass if has begun (obviously)
        vkCmdEndRenderPass(cmd);
#pragma endregion

        // All errors generated from vkCmd functions will populate here. The vkCmd functions themselves are all void.
        if (vkEndCommandBuffer(cmd) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "An error occured during execution of the command buffer for frame %" PRIu32 "!",
                     FRAME_INDEX);
        }
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue with a failed command buffer.");
}
#pragma endregion
#pragma region Submit
void commandBuffer_submit(State_t *pState)
{
    const uint32_t FRAME = pState->renderer.currentFrame;
    const VkCommandBuffer CMD = pState->renderer.pCommandBuffers[FRAME];
    const VkPipelineStageFlags pSTAGE_FLAGS[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    const VkSubmitInfo SUBMIT_INFO = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &pState->renderer.pImageAcquiredSemaphores[FRAME],
        .pWaitDstStageMask = pSTAGE_FLAGS,
        .commandBufferCount = 1,
        .pCommandBuffers = &CMD,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &pState->renderer.pRenderFinishedSemaphores[FRAME],
    };

    int crashLine = 0;
    do
    {
        if (vkResetFences(pState->context.device, 1, &pState->renderer.pInFlightFences[FRAME]) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to reset in-flight fence before submitting the command buffer for frame %" PRIu32 "!", FRAME);
            break;
        }

        if (vkQueueSubmit(pState->context.graphicsQueue, 1, &SUBMIT_INFO, pState->renderer.pInFlightFences[FRAME]) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to submit graphics queue to the command buffer!");
            break;
        }
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without successfully submitting the command buffer.");
}
#pragma endregion
#pragma region Single-time Cmd
const uint32_t COMMAND_BUFFER_COUNT = 1;
VkCommandBuffer commandBuffer_singleTime_start(State_t *pState)
{
    VkCommandBuffer commandBuffer;

    int crashLine = 0;
    do
    {
        const VkCommandBufferAllocateInfo ALLOCATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandPool = pState->renderer.commandPool,
            .commandBufferCount = COMMAND_BUFFER_COUNT,
        };

        const VkCommandBufferBeginInfo BEGIN_INFO = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        if (vkAllocateCommandBuffers(pState->context.device, &ALLOCATE_INFO, &commandBuffer) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate the command buffer!");
            break;
        }

        if (vkBeginCommandBuffer(commandBuffer, &BEGIN_INFO) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to begin the command buffer!");
            break;
        }
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without successful command buffer writes.");

    return commandBuffer;
}

void commandBuffer_singleTime_end(State_t *pState, VkCommandBuffer commandBuffer)
{
    int crashLine = 0;
    do
    {
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to end the command buffer!");
            break;
        }

        const VkSubmitInfo SUBMIT_INFO = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = COMMAND_BUFFER_COUNT,
            .pCommandBuffers = &commandBuffer,
        };

        // A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete,
        // instead of executing one at a time. That may give the driver more opportunities to optimize but is not
        // implemented at this time. Fence is passed as null and we just wait for the transfer queue to be idle right now.
        // CPU bottleneck
        VkFence fence = VK_NULL_HANDLE;
        if (vkQueueSubmit(pState->context.graphicsQueue, 1, &SUBMIT_INFO, fence) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to submit the graphics queue!");
            break;
        }

        if (vkQueueWaitIdle(pState->context.graphicsQueue) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to wait for graphics queue to idle!");
            break;
        }

        vkFreeCommandBuffers(pState->context.device, pState->renderer.commandPool, COMMAND_BUFFER_COUNT, &commandBuffer);
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without successful command buffer writing.");
}
#pragma endregion
#pragma region Create
void commandBuffer_create(State_t *pState)
{
    int crashLine = 0;
    do
    {
        // free previous commmand buffers if present
        if (pState->renderer.pCommandBuffers != NULL)
        {
            vkFreeCommandBuffers(pState->context.device, pState->renderer.commandPool,
                                 pState->config.maxFramesInFlight, pState->renderer.pCommandBuffers);
            free(pState->renderer.pCommandBuffers);
            pState->renderer.pCommandBuffers = NULL;
        }

        pState->renderer.pCommandBuffers = malloc(sizeof(VkCommandBuffer) * pState->config.maxFramesInFlight);
        if (!pState->renderer.pCommandBuffers)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for command buffers!");
            break;
        }

        const VkCommandBufferAllocateInfo ALLOCATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pState->renderer.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = pState->config.maxFramesInFlight,
        };

        if (vkAllocateCommandBuffers(pState->context.device, &ALLOCATE_INFO, pState->renderer.pCommandBuffers) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for command buffers!");
            break;
        }
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without a command buffer for GPU control.");
}
#pragma endregion