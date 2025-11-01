#pragma region Includes / Defines
#include "core/logs.h"
#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "rendering/shaders.h"
#include "rendering/types/graphicsPipeline_t.h"
#include "core/crash_handler.h"

#define VIEWPORT_COUNT 1
#define SCISSOR_COUNT 1
#define COLOR_BLEND_COUNT 1
#pragma endregion
#pragma endregion
#pragma region Layout
/// @brief Create info for the pipeline layout
static void layout_createInfo_get(State_t *pState, const GraphicsPipeline_t GRAPHICS_PIPELINE, const GraphicsTarget_t TARGET,
                                  VkPipelineLayout *pLayout)
{
    int crashLine = 0;
    do
    {
        if (!pState || !pLayout)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        const VkPushConstantRange PC_RANGE = {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .offset = 0,
            .size = sizeof(Mat4c_t)};

        const VkPipelineLayoutCreateInfo layoutCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &pState->renderer.descriptorSetLayout,
            .pushConstantRangeCount = 1,
            .pPushConstantRanges = &PC_RANGE};

        if (vkCreatePipelineLayout(pState->context.device, &layoutCreateInfo, pState->context.pAllocator, pLayout) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to create a graphics pipeline layout for %s", graphicsPipeline_ToString(GRAPHICS_PIPELINE));
            break;
        }

        switch (GRAPHICS_PIPELINE)
        {
        case GRAPHICS_PIPELINE_FILL:
            switch (TARGET)
            {
            case GRAPHICS_TARGET_MODEL:
                pState->renderer.pipelineLayoutFillModel = *pLayout;
                break;
            case GRAPHICS_TARGET_VOXEL:
                pState->renderer.pipelineLayoutFillVoxel = *pLayout;
                break;
            }
            break;
        case GRAPHICS_PIPELINE_WIREFRAME:
            switch (TARGET)
            {
            case GRAPHICS_TARGET_MODEL:
                pState->renderer.pipelineLayoutWireframeModel = *pLayout;
                break;
            case GRAPHICS_TARGET_VOXEL:
                pState->renderer.pipelineLayoutWireframeVoxel = *pLayout;
                break;
            }
            break;
        default:
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted to create an invalid graphics pipeline layout!");
            break;
        }
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without a graphics pipeline layout!");
}
#pragma endregion
#pragma region Color Blend
/// @brief Create info for the color blend state
static void colorBlendState_createInfo_get(State_t *pState, const GraphicsPipeline_t GRAPHICS_PIPELINE,
                                           VkPipelineColorBlendAttachmentState *pColorBlendAttachmentStates,
                                           VkPipelineColorBlendStateCreateInfo *pCreateInfo)
{
    int crashLine = 0;
    do
    {
        if (!pState || !pColorBlendAttachmentStates || !pCreateInfo)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        pColorBlendAttachmentStates[0] = (VkPipelineColorBlendAttachmentState){
            // Color blending omitted at this time
            .blendEnable = VK_FALSE,
            // Bitwise OR to build the mask of what color bits the blend will write t
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

        VkPipelineColorBlendStateCreateInfo createInfo = (VkPipelineColorBlendStateCreateInfo){
            // This configuration will need to be changed if alpha (transparency) is to be supported in the future.
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = COLOR_BLEND_COUNT,
            .pAttachments = pColorBlendAttachmentStates,
            // Default blend constants applied to all the color blend attachments. When default, this declaration isn't necessary
            // but this is good to include for legibility. The struct doesn't accept the array directly and requires index replacement.
            .blendConstants = {0, 0, 0, 0}};

        switch (GRAPHICS_PIPELINE)
        {
        case GRAPHICS_PIPELINE_FILL:
            // Default
            break;
        case GRAPHICS_PIPELINE_WIREFRAME:
            createInfo.logicOpEnable = VK_TRUE;
            createInfo.logicOp = VK_LOGIC_OP_INVERT;
            break;
        default:
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted to create color blend state for invalid graphics pipeline type!");
        }

        *pCreateInfo = createInfo;
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without color blending information.");
}
#pragma endregion
#pragma region Rasterization
/// @brief Create info for the rasterization state
static void rasterizationState_createInfo_get(State_t *pState, const GraphicsPipeline_t GRAPHICS_PIPELINE,
                                              VkPipelineRasterizationStateCreateInfo *pCreateInfo)
{
    int crashLine = 0;
    do
    {
        if (!pState || !pCreateInfo)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        VkPolygonMode polygonMode;
        switch (GRAPHICS_PIPELINE)
        {
        case GRAPHICS_PIPELINE_FILL:
            polygonMode = VK_POLYGON_MODE_FILL;
            break;
        case GRAPHICS_PIPELINE_WIREFRAME:
            polygonMode = VK_POLYGON_MODE_LINE;
            break;
        default:
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Attempted to create invalid render pipeline type!");
            // Default to avoid unknown behaviour
            polygonMode = VK_POLYGON_MODE_FILL;
            break;
        }

        *pCreateInfo = (VkPipelineRasterizationStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .lineWidth = 1.0F,
            // This one is super duper important. It is convention to have verticies rotate counter-clockwise. So a triangle would have points
            // 0, 1, and 2 at the top, bottom left, and then bottom right. This lets the renderer know which way a triangle is facing. If the
            // renderer is passed a triangle with verticies going clockwise when the front face is assigned as counter-clockwise then that
            // means the triangle is backwards/facing away from the camera reference and can be culled/etc.
            .frontFace = pState->config.vertexWindingDirection,
            // Tells the render pipeline what faces should be culled (hidden). Back but means that back (covered/blocked) bits won't be rendered.
            // This is literally the backface culling that makes Minecraft playable
            .cullMode = pState->config.cullModeMask,
            // Fill is opaque normally rendered object and line is wireframe
            .polygonMode = polygonMode};
    } while (0);

    if (crashLine != 0)
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without the creation info for rasterization.");
}
#pragma endregion
#pragma region Viewport
/// @brief Create info for the viewport state
static void viewportState_createInfo_get(State_t *pState, VkViewport *pViewports, VkRect2D *pScissors,
                                         VkPipelineViewportStateCreateInfo *pCreateInfo)
{
    do
    {
        if (!pState || !pViewports || !pScissors || !pCreateInfo)
        {
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        pViewports[0] = (VkViewport){
            .width = (float)pState->window.swapchain.imageExtent.width,
            .height = (float)pState->window.swapchain.imageExtent.height,
            .maxDepth = 1.0F};

        pScissors[0] = (VkRect2D){
            .extent = pState->window.swapchain.imageExtent};

        *pCreateInfo = (VkPipelineViewportStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = VIEWPORT_COUNT,
            .pViewports = pViewports,
            .scissorCount = SCISSOR_COUNT,
            .pScissors = pScissors,
        };

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a the creation info for the graphics pipeline's viewport.");
}
#pragma endregion
#pragma region Shaders
const char *pSHADER_FUNCTION_ENTRY_POINT = "main";
/// @brief Load the shader for the graphics pipeline, target, and stage
static void shader_load(State_t *pState, const GraphicsPipeline_t PIPELINE, const GraphicsTarget_t TARGET,
                        const ShaderStage_t STAGE, VkShaderModule *pShaderModule)
{
    ShaderBlob_t pVertexBlobs[GRAPHICS_TARGET_COUNT];
    pVertexBlobs[GRAPHICS_TARGET_MODEL] = (ShaderBlob_t){shaderVertCode, shaderVertCodeSize};
    pVertexBlobs[GRAPHICS_TARGET_VOXEL] = (ShaderBlob_t){shaderVoxelVertCode, shaderVoxelVertCodeSize};

    ShaderBlob_t pFragmentBlobs[GRAPHICS_PIPELINE_COUNT];
    pFragmentBlobs[GRAPHICS_PIPELINE_FILL] = (ShaderBlob_t){shaderFillFragCode, shaderFillFragCodeSize};
    pFragmentBlobs[GRAPHICS_PIPELINE_WIREFRAME] = (ShaderBlob_t){shaderWireframeFragCode, shaderWireframeFragCodeSize};

    int crashLine = 0;
    do
    {
        logs_log(LOG_DEBUG, "Loading %s for %s for %s...",
                 shaderStage_ToString(STAGE), graphicsPipeline_ToString(PIPELINE), graphicsTarget_ToString(TARGET));

        if (!pShaderModule)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Recieved an invalid shader module pointer!");
            break;
        }

        VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        };

        ShaderBlob_t blob = {0};

        switch (STAGE)
        {
        case SHADER_STAGE_VERTEX:
            blob = pVertexBlobs[TARGET];
            break;
        case SHADER_STAGE_FRAGMENT:
            blob = pFragmentBlobs[PIPELINE];
            break;
        default:
            crashLine = __LINE__;
            break;
        }

        createInfo.codeSize = blob.codeSize;
        createInfo.pCode = blob.pCODE;

        if (vkCreateShaderModule(pState->context.device, &createInfo, pState->context.pAllocator, pShaderModule) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to create a shader module for %s!", graphicsTarget_ToString(TARGET));
            break;
        }
    } while (0);

    if (crashLine != 0)
    {
        crashHandler_crash_graceful(
            CRASH_LOCATION_LINE(crashLine),
            "The program cannot continue without having shaders to direct GPU actions for image rendering.");
    }
}

static void shaderStage_createInfo_get(State_t *pState, const GraphicsPipeline_t PIPELINE, const GraphicsTarget_t TARGET,
                                       VkPipelineShaderStageCreateInfo *pShaderStages, VkShaderModule *pVertexShaderModule,
                                       VkShaderModule *pFragmentShaderModule)
{
    do
    {
        if (!pState || !pVertexShaderModule || !pFragmentShaderModule || !pShaderStages)
        {
            logs_log(LOG_ERROR, "Recieved an invalid pointer!");
            break;
        }

        shader_load(pState, PIPELINE, TARGET, SHADER_STAGE_VERTEX, pVertexShaderModule);
        pShaderStages[SHADER_STAGE_VERTEX] = (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = *pVertexShaderModule,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            // The entry point of the actual shader code. Default is obviously keeping the function as "main"
            .pName = pSHADER_FUNCTION_ENTRY_POINT,
        };

        shader_load(pState, PIPELINE, TARGET, SHADER_STAGE_FRAGMENT, pFragmentShaderModule);
        pShaderStages[SHADER_STAGE_FRAGMENT] = (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = *pFragmentShaderModule,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pName = pSHADER_FUNCTION_ENTRY_POINT,
        };

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without being able to create the shader stages.");
}
#pragma endregion
#pragma region Create
static void create(State_t *pState, const GraphicsPipeline_t PIPELINE, const GraphicsTarget_t TARGET)
{
    VkShaderModule vertexShaderModule = {0};
    VkShaderModule fragmentShaderModule = {0};
    VkPipelineShaderStageCreateInfo pShaderStages[SHADER_STAGE_COUNT];
    shaderStage_createInfo_get(pState, PIPELINE, TARGET, pShaderStages, &vertexShaderModule, &fragmentShaderModule);

    const VkDynamicState pDYNAMIC_STATES[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(pDYNAMIC_STATES) / sizeof(*pDYNAMIC_STATES),
        .pDynamicStates = pDYNAMIC_STATES};

    VkPipelineVertexInputStateCreateInfo VERTEX_INPUT_STATE = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = NUM_SHADER_VERTEX_BINDING_DESCRIPTIONS,
        .pVertexBindingDescriptions = shaderVertexGetBindingDescription(),
        .vertexAttributeDescriptionCount = NUM_SHADER_VERTEX_ATTRIBUTES,
        .pVertexAttributeDescriptions = shaderVertexGetInputAttributeDescriptions()};

    VkPipelineInputAssemblyStateCreateInfo INPUT_ASSEMBLY_STATE = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE};

    VkPipelineMultisampleStateCreateInfo MULTISAMPLING_STATE = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        // Don't care about multisampling for now
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};

    VkViewport pViewports[VIEWPORT_COUNT];
    VkRect2D pScissors[SCISSOR_COUNT];
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {0};
    viewportState_createInfo_get(pState, pViewports, pScissors, &viewportStateCreateInfo);

    VkPipelineRasterizationStateCreateInfo rasterizationStateCrateInfo = {0};
    rasterizationState_createInfo_get(pState, PIPELINE, &rasterizationStateCrateInfo);

    VkPipelineColorBlendAttachmentState pColorBlendAttachmentStates[COLOR_BLEND_COUNT];
    VkPipelineColorBlendStateCreateInfo colorBlendAttachmentStateCreateInfo = {0};
    colorBlendState_createInfo_get(pState, PIPELINE, pColorBlendAttachmentStates, &colorBlendAttachmentStateCreateInfo);

    VkPipelineLayout pipelineLayout = {0};
    layout_createInfo_get(pState, PIPELINE, TARGET, &pipelineLayout);

    const VkPipelineDepthStencilStateCreateInfo DEPTH_STENCIL = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        // If the depth of new fragments should be compared to the depth buffer to see if they should be discarded
        .depthTestEnable = VK_TRUE,
        // If the new depth of fragments that pass the depth test should actually be written to the depth buffer
        // Only write depth if not wireframe
        .depthWriteEnable = PIPELINE != GRAPHICS_PIPELINE_WIREFRAME,
        // Convention of lower depth = closer so the depth of new fragments should be less (backwards for things like water?)
        .depthCompareOp = VK_COMPARE_OP_LESS,
        // Optional. Allows you to only keep fragments that fall within the specified depth range. Disabled but included for legibility
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0F,
        .maxDepthBounds = 1.0F,
        // Optional. Would require making sure that the format of the depth/stencil image contains a stencil component
        .stencilTestEnable = VK_FALSE};

    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        // layout, pVertexInputState, and pInputAssemblyState all rely on the vertex layout
        .layout = pipelineLayout,
        .pVertexInputState = &VERTEX_INPUT_STATE,
        .pInputAssemblyState = &INPUT_ASSEMBLY_STATE,
        // Get the length of the array by dividing the size of the shaderStages array by the size of the type of the first index of the array
        .stageCount = sizeof(pShaderStages) / sizeof(*pShaderStages),
        .pStages = pShaderStages,
        .pDynamicState = &dynamicStateCreateInfo,
        .pRasterizationState = &rasterizationStateCrateInfo,
        .pMultisampleState = &MULTISAMPLING_STATE,
        .pColorBlendState = &colorBlendAttachmentStateCreateInfo,
        .renderPass = pState->renderer.pRenderPass,
        .pViewportState = &viewportStateCreateInfo,
        .pDepthStencilState = &DEPTH_STENCIL,
    };

    const VkGraphicsPipelineCreateInfo CREATE_INFOS[] = {createInfo};

    int crashLine = 0;
    do
    {
        const uint32_t CREATE_INFO_COUNT = 1;

        switch (PIPELINE)
        {
        case GRAPHICS_PIPELINE_FILL:
            switch (TARGET)
            {
            case GRAPHICS_TARGET_MODEL:
                if (vkCreateGraphicsPipelines(pState->context.device, NULL, CREATE_INFO_COUNT, CREATE_INFOS, pState->context.pAllocator,
                                              &pState->renderer.graphicsPipelineFillModel) != VK_SUCCESS)
                    crashLine = __LINE__;
                break;

            case GRAPHICS_TARGET_VOXEL:
                if (vkCreateGraphicsPipelines(pState->context.device, NULL, CREATE_INFO_COUNT, CREATE_INFOS, pState->context.pAllocator,
                                              &pState->renderer.graphicsPipelineFillVoxel) != VK_SUCCESS)
                    crashLine = __LINE__;
                break;
            }
            break;
        case GRAPHICS_PIPELINE_WIREFRAME:
            switch (TARGET)
            {
            case GRAPHICS_TARGET_MODEL:
                if (vkCreateGraphicsPipelines(pState->context.device, NULL, CREATE_INFO_COUNT, CREATE_INFOS, pState->context.pAllocator,
                                              &pState->renderer.graphicsPipelineWireframeModel) != VK_SUCCESS)
                    crashLine = __LINE__;
                break;

            case GRAPHICS_TARGET_VOXEL:
                if (vkCreateGraphicsPipelines(pState->context.device, NULL, CREATE_INFO_COUNT, CREATE_INFOS, pState->context.pAllocator,
                                              &pState->renderer.graphicsPipelineWireframeVoxel) != VK_SUCCESS)
                    crashLine = __LINE__;
                break;
            }
            break;
        }
    } while (0);

    // Once the render pipeline has been created, the shader information is stored within it. Thus, the shader modules can
    // actually be destroyed now. Do note that this means that if the shaders need to be changed, everything done in this function
    // must be re-performed.
    vkDestroyShaderModule(pState->context.device, fragmentShaderModule, pState->context.pAllocator);
    vkDestroyShaderModule(pState->context.device, vertexShaderModule, pState->context.pAllocator);

    if (crashLine != 0)
    {
        logs_log(LOG_ERROR, "Failed to create the graphics pipeline %s targeting %s!", graphicsPipeline_ToString(PIPELINE), graphicsTarget_ToString(TARGET));
        crashHandler_crash_graceful(CRASH_LOCATION_LINE(crashLine),
                                    "The program cannot continue without a graphics pipeline.");
    }
}

/// @brief Create the voxels' graphics pipelines
static void voxel_create(State_t *pState)
{
    create(pState, GRAPHICS_PIPELINE_FILL, GRAPHICS_TARGET_VOXEL);
    create(pState, GRAPHICS_PIPELINE_WIREFRAME, GRAPHICS_TARGET_VOXEL);
}

/// @brief Create the models' graphics pipelines
static void models_create(State_t *pState)
{
    create(pState, GRAPHICS_PIPELINE_FILL, GRAPHICS_TARGET_MODEL);
    create(pState, GRAPHICS_PIPELINE_WIREFRAME, GRAPHICS_TARGET_MODEL);
}

void graphicsPipeline_createAll(State_t *pState)
{
    voxel_create(pState);
    models_create(pState);
}
#pragma endregion
#pragma region Destroy
void graphicsPipeline_destroyAll(State_t *pState)
{
    vkDestroyPipeline(pState->context.device, pState->renderer.graphicsPipelineFillModel, pState->context.pAllocator);
    vkDestroyPipelineLayout(pState->context.device, pState->renderer.pipelineLayoutFillModel, pState->context.pAllocator);

    vkDestroyPipeline(pState->context.device, pState->renderer.graphicsPipelineFillVoxel, pState->context.pAllocator);
    vkDestroyPipelineLayout(pState->context.device, pState->renderer.pipelineLayoutFillVoxel, pState->context.pAllocator);

    vkDestroyPipeline(pState->context.device, pState->renderer.graphicsPipelineWireframeModel, pState->context.pAllocator);
    vkDestroyPipelineLayout(pState->context.device, pState->renderer.pipelineLayoutWireframeModel, pState->context.pAllocator);

    vkDestroyPipeline(pState->context.device, pState->renderer.graphicsPipelineWireframeVoxel, pState->context.pAllocator);
    vkDestroyPipelineLayout(pState->context.device, pState->renderer.pipelineLayoutWireframeVoxel, pState->context.pAllocator);
}
#pragma endregion
#pragma region Undefines
#undef VIEWPORT_COUNT
#undef SCISSOR_COUNT
#undef COLOR_BLEND_COUNT
#pragma endregion