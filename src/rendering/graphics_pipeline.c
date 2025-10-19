#pragma once

#include "core/logs.h"
#include <vulkan/vulkan.h>
#include "core/types/state_t.h"
#include "rendering/shaders.h"
#include "rendering/types/graphicsPipeline_t.h"

/// @brief Creates the graphics pipeline
/// @param state
void gp_create(State_t *state, GraphicsPipeline_t graphicsPipeline)
{
    const char *shaderEntryFunctionName = "main";

    logs_log(LOG_DEBUG, "Loading shaders for render pipeline %d...", (int)graphicsPipeline);

    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        // Need to pass how big the code actuallly is...
        .codeSize = shaderVertCodeSize,
        // ... For it to correctly allocate storage for it here.
        .pCode = shaderVertCode,
    };
    VkShaderModule vertexShaderModule;
    logs_logIfError(vkCreateShaderModule(state->context.device, &vertexShaderModuleCreateInfo, state->context.pAllocator, &vertexShaderModule),
                    "Couldn't create the vertex shader module.");

    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo;
    switch (graphicsPipeline)
    {
    case GRAPHICS_PIPELINE_FILL:
        fragmentShaderModuleCreateInfo = (VkShaderModuleCreateInfo){
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderFillFragCodeSize,
            .pCode = shaderFillFragCode,
        };
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        fragmentShaderModuleCreateInfo = (VkShaderModuleCreateInfo){
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = shaderWireframeFragCodeSize,
            .pCode = shaderWireframeFragCode,
        };
        break;
    default:
        logs_log(LOG_ERROR, "Attempted to create a fragment shader for an invalid graphics pipeline type! None was created.");
        break;
    }

    VkShaderModule fragmentShaderModule;
    logs_logIfError(vkCreateShaderModule(state->context.device, &fragmentShaderModuleCreateInfo, state->context.pAllocator, &fragmentShaderModule),
                    "Couldn't create the fragment shader module for graphics pipeline %d.", (int)graphicsPipeline);

    // Because these indexes are hardcoded, they can be referenced by definitions (SHADER_STAGE_xxx)
    VkPipelineShaderStageCreateInfo shaderStages[] = {
        // Stage 0 (vertex)
        (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = vertexShaderModule,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            // The entry point of the actual shader code. Default is obviously keeping the function as "main"
            .pName = shaderEntryFunctionName},
        // Stage 1 (fragment)
        (VkPipelineShaderStageCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .module = fragmentShaderModule,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pName = shaderEntryFunctionName},
    };

    // Dynamic states are the things that can be changed at runtime without requiring a recreation of the render pipeline (ex: window/viewport size)
    // https://registry.khronos.org/vulkan/specs/latest/man/html/VkDynamicState.html
    const VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        // The amount of scissors used to divide the pipeline into smaller rectangles
        VK_DYNAMIC_STATE_SCISSOR,
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynamicStates) / sizeof(*dynamicStates),
        .pDynamicStates = dynamicStates};

    VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = NUM_SHADER_VERTEX_BINDING_DESCRIPTIONS,
        .pVertexBindingDescriptions = shaderVertexGetBindingDescription(),
        .vertexAttributeDescriptionCount = NUM_SHADER_VERTEX_ATTRIBUTES,
        .pVertexAttributeDescriptions = shaderVertexGetInputAttributeDescriptions(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        // The type of primitives used for rendering. The most common is triangles as defined by three verticies.
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        // Passing a special index to a verticy will inform the GPU that the mesh wants to be split at that point.
        // Unnecessary for now.
        .primitiveRestartEnable = VK_FALSE};

    VkViewport viewports[] = {
        (VkViewport){
            // Origin (default is 0 but explicitly saying this is helpful to see)
            .x = 0,
            .y = 0,
            .width = (float)state->window.swapchain.imageExtent.width,
            .height = (float)state->window.swapchain.imageExtent.height,
            .maxDepth = 1.0F}};

    VkRect2D scissors[] = {
        (VkRect2D){
            .extent = state->window.swapchain.imageExtent}};

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = sizeof(viewports) / sizeof(*viewports),
        .pViewports = viewports,
        .scissorCount = sizeof(scissors) / sizeof(*scissors),
        .pScissors = scissors,
    };

    VkPolygonMode polygonMode;

    switch (graphicsPipeline)
    {
    case GRAPHICS_PIPELINE_FILL:
        polygonMode = VK_POLYGON_MODE_FILL;
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        polygonMode = VK_POLYGON_MODE_LINE;
        break;
    default:
        logs_log(LOG_ERROR, "Attempted to create invalid render pipeline type! Defaulting to fill.");
        polygonMode = VK_POLYGON_MODE_FILL;
        break;
    }

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0F,
        // This one is super duper important. It is convention to have verticies rotate counter-clockwise. So a triangle would have points
        // 0, 1, and 2 at the top, bottom left, and then bottom right. This lets the renderer know which way a triangle is facing. If the
        // renderer is passed a triangle with verticies going clockwise when the front face is assigned as counter-clockwise then that
        // means the triangle is backwards/facing away from the camera reference and can be culled/etc.
        .frontFace = state->config.vertexWindingDirection,
        // Tells the render pipeline what faces should be culled (hidden). Back but means that back (covered/blocked) bits won't be rendered.
        // This is literally the backface culling that makes Minecraft playable
        .cullMode = state->config.cullModeMask,
        // Fill is opaque normally rendered object and line is wireframe
        .polygonMode = polygonMode,
    };

    VkPipelineMultisampleStateCreateInfo multisamplingState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        // Don't care about multisampling for now
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
        (VkPipelineColorBlendAttachmentState){
            // Color blending omitted at this time
            .blendEnable = VK_FALSE,
            // Bitwise OR to build the mask of what color bits the blend will write t
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        }};

    VkPipelineColorBlendStateCreateInfo colorBlendState;
    switch (graphicsPipeline)
    {
    case GRAPHICS_PIPELINE_FILL:
        // This configuration will need to be changed if alpha (transparency) is to be supported in the future.
        colorBlendState = (VkPipelineColorBlendStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = sizeof(colorBlendAttachmentStates) / sizeof(*colorBlendAttachmentStates),
            .pAttachments = colorBlendAttachmentStates,
            // Default blend constants applied to all the color blend attachments. When default, this declaration isn't necessary
            // but this is good to include for legibility. The struct doesn't accept the array directly and requires index replacement.
            .blendConstants = {0, 0, 0, 0},
        };
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        colorBlendState = (VkPipelineColorBlendStateCreateInfo){
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .attachmentCount = sizeof(colorBlendAttachmentStates) / sizeof(*colorBlendAttachmentStates),
            .pAttachments = colorBlendAttachmentStates,
            .logicOpEnable = VK_TRUE,
            .logicOp = VK_LOGIC_OP_INVERT,
            .blendConstants = {0, 0, 0, 0},
        };
        break;
    default:
        logs_log(LOG_ERROR, "Attempted to create color blend state for invalid graphics pipeline type! No fill was created.");
    }

    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state->renderer.descriptorSetLayout,
    };

    VkPipelineLayout pipelineLayout;
    switch (graphicsPipeline)
    {
    case GRAPHICS_PIPELINE_FILL:
        logs_logIfError(vkCreatePipelineLayout(state->context.device, &layoutCreateInfo, state->context.pAllocator,
                                               &pipelineLayout),
                        "Failed to create the fill pipeline layout.");
        state->renderer.pipelineLayoutFill = pipelineLayout;
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        logs_logIfError(vkCreatePipelineLayout(state->context.device, &layoutCreateInfo, state->context.pAllocator,
                                               &pipelineLayout),
                        "Failed to create the wireframe pipeline layout.");
        state->renderer.pipelineLayoutWireframe = pipelineLayout;
        break;
    default:
        logs_log(LOG_ERROR, "Attempted to create invalid render pipeline type's layout! Defaulting to fill's.");
        logs_logIfError(vkCreatePipelineLayout(state->context.device, &layoutCreateInfo, state->context.pAllocator,
                                               &pipelineLayout),
                        "Failed to create the fill pipeline layout.");
        state->renderer.pipelineLayoutFill = pipelineLayout;
        break;
    }

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        // If the depth of new fragments should be compared to the depth buffer to see if they should be discarded
        .depthTestEnable = VK_TRUE,
        // If the new depth of fragments that pass the depth test should actually be written to the depth buffer
        // Only write depth if not wireframe
        .depthWriteEnable = graphicsPipeline != GRAPHICS_PIPELINE_WIREFRAME,
        // Convention of lower depth = closer so the depth of new fragments should be less (backwards for things like water?)
        .depthCompareOp = VK_COMPARE_OP_LESS,
        // Optional. Allows you to only keep fragments that fall within the specified depth range. Disabled but included for legibility
        .depthBoundsTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 1.0f,
        // Optional. Would require making sure that the format of the depth/stencil image contains a stencil component
        .stencilTestEnable = VK_FALSE,
        // .front = {},
        // .back = {},
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        // layout, pVertexInputState, and pInputAssemblyState all rely on the vertex layout
        .layout = pipelineLayout,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        // Get the length of the array by dividing the size of the shaderStages array by the size of the type of the first index of the array
        .stageCount = sizeof(shaderStages) / sizeof(*shaderStages),
        .pStages = shaderStages,
        .pDynamicState = &dynamicState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisamplingState,
        .pColorBlendState = &colorBlendState,
        .renderPass = state->renderer.pRenderPass,
        .pViewportState = &viewportStateCreateInfo,
        .pDepthStencilState = &depthStencil,
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfos[] = {
        graphicsPipelineCreateInfo,
    };
    // Don't care about pipeline cache right now

    switch (graphicsPipeline)
    {
    case GRAPHICS_PIPELINE_FILL:
        logs_logIfError(vkCreateGraphicsPipelines(state->context.device, NULL, 1U, graphicsPipelineCreateInfos, state->context.pAllocator,
                                                  &state->renderer.graphicsPipelineFill),
                        "Failed to create the graphics pipeline fill.");
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        logs_logIfError(vkCreateGraphicsPipelines(state->context.device, NULL, 1U, graphicsPipelineCreateInfos, state->context.pAllocator,
                                                  &state->renderer.graphicsPipelineWireframe),
                        "Failed to create the graphics pipeline wireframe.");
        break;
    default:
        logs_log(LOG_ERROR, "Attempted to create invalid graphics pipeline type! Defaulting to fill's.");
        logs_logIfError(vkCreateGraphicsPipelines(state->context.device, NULL, 1U, graphicsPipelineCreateInfos, state->context.pAllocator,
                                                  &state->renderer.graphicsPipelineFill),
                        "Failed to create the graphics pipeline.");
        break;
    }

    // Once the render pipeline has been created, the shader information is stored within it. Thus, the shader modules can
    // actually be destroyed now. Do note that this means that if the shaders need to be changed, everything done in this function
    // must be re-performed.
    vkDestroyShaderModule(state->context.device, fragmentShaderModule, state->context.pAllocator);
    vkDestroyShaderModule(state->context.device, vertexShaderModule, state->context.pAllocator);
}

/// @brief Destroys the graphics pipeline
/// @param state
void gp_destroy(State_t *state, GraphicsPipeline_t graphicsPipeline)
{
    switch (graphicsPipeline)
    {
    case GRAPHICS_PIPELINE_FILL:
        vkDestroyPipeline(state->context.device, state->renderer.graphicsPipelineFill, state->context.pAllocator);
        vkDestroyPipelineLayout(state->context.device, state->renderer.pipelineLayoutFill, state->context.pAllocator);
        break;
    case GRAPHICS_PIPELINE_WIREFRAME:
        vkDestroyPipeline(state->context.device, state->renderer.graphicsPipelineWireframe, state->context.pAllocator);
        vkDestroyPipelineLayout(state->context.device, state->renderer.pipelineLayoutWireframe, state->context.pAllocator);
        break;
    default:
        logs_log(LOG_ERROR, "Attempted to destroy invalid render pipeline type!");
        break;
    }
}