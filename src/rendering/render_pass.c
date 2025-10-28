#pragma region Includes/Defs
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "rendering/depth.h"
#include "core/crash_handler.h"

#define SUBPASS_COUNT 1
#define DEPENDENCY_COUNT 1
#define ATTACHMENT_COUNT 2
#pragma endregion
#pragma region Attachments
/// @brief Create the attachment descs
static void attachmentDescs_create(State_t *pState, VkAttachmentDescription *pAttachmentDescs)
{
    // Present
    pAttachmentDescs[0] = (VkAttachmentDescription){
        .format = pState->window.swapchain.format,
        // No MSAA at this time
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // We don't know what the original layout will be
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // Tells Vulkan to transition the image layout to presentation source for presenting to the screen
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        // The load operation will be clear (clear image initially)
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        // We want to store the results of this render
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    };

    // Depth
    pAttachmentDescs[1] = (VkAttachmentDescription){
        .format = depth_formatGet(pState),
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        // The depth data won't be stored because its not used after drawing
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    pState->renderer.renderpassAttachmentCount = ATTACHMENT_COUNT;
}
#pragma endregion
#pragma region Subpass
/// @brief Create the subpass' dependencies
static void subpassDependencies_create(VkSubpassDependency *pSubpassDependencies)
{
    pSubpassDependencies[0] = (VkSubpassDependency){
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        // Destination is the first subpass
        .dstSubpass = 0,
        // Wait in the pipeline for the previous external operations to finish before color attachment output
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };
}
#pragma endregion
#pragma region Const/Dest-ructors
void renderpass_create(State_t *pState)
{
    do
    {
        VkSubpassDescription pSubpassDescs[SUBPASS_COUNT] = {0};
        pSubpassDescs[0] =
            (VkSubpassDescription){
                // Use for graphics pipeline instead of a compute pipeline
                .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
                .colorAttachmentCount = 1,
                .pColorAttachments = &(VkAttachmentReference){
                    .attachment = 0,
                    // Render target for color output
                    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                },
                .pDepthStencilAttachment = &(VkAttachmentReference){
                    .attachment = 1,
                    .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                },
            };

        VkSubpassDependency pSubpassDependencies[DEPENDENCY_COUNT] = {0};
        subpassDependencies_create(pSubpassDependencies);

        VkAttachmentDescription pAttachmentDescs[ATTACHMENT_COUNT] = {0};
        attachmentDescs_create(pState, pAttachmentDescs);

        const VkRenderPassCreateInfo CREATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .subpassCount = SUBPASS_COUNT,
            .pSubpasses = pSubpassDescs,
            .dependencyCount = DEPENDENCY_COUNT,
            .pDependencies = pSubpassDependencies,
            .attachmentCount = pState->renderer.renderpassAttachmentCount,
            .pAttachments = pAttachmentDescs,
        };

        if (vkCreateRenderPass(pState->context.device, &CREATE_INFO, pState->context.pAllocator, &pState->renderer.pRenderPass) != VK_SUCCESS)
        {
            logs_log(LOG_ERROR, "Failed to create the Vulkan render pass!");
            break;
        }

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a render pass to use for image presentation!");
};

void renderpass_destroy(State_t *pState)
{
    vkDestroyRenderPass(pState->context.device, pState->renderer.pRenderPass, pState->context.pAllocator);

    pState->renderer.renderpassAttachmentCount = 0;
}
#pragma endregion
#pragma region Undefs
#undef SUBPASS_COUNT
#undef DEPENDENCY_COUNT
#undef ATTACHMENT_COUNT
#pragma endregion