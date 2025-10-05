#pragma once

#include "Toolkit.h"

/* TODO:
   1. Add an additional pipeline that uses the line rendering mode to draw connections between verticies (for showing voxel hitboxes and chunks)
      or empty polygon mode maybe. Maybe polygon for mesh and line for chunks?
   2. Don't forget to change to VK_CULL_MODE_FRONT_BIT in the future
*/

/// @brief The render pass is basically the blueprint for the graphics operation in the graphics pipeline
/// @param state
void renderPassCreate(State_t *state)
{
    VkAttachmentReference colorAttachmentReferences[] = {
        (VkAttachmentReference){
            // This 0 is the output location of the outColor vec4 in the fragment shader. If other outputs are needed, the attachment
            // number would be the same as the output location
            .attachment = 0U,
            // Render target for color output
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };

    state->renderer.renderpassAttachmentCount = sizeof(colorAttachmentReferences) / sizeof(*colorAttachmentReferences);

    VkSubpassDescription subpassDescriptions[] = {
        (VkSubpassDescription){
            // Use for graphics pipeline instead of a compute pipeline
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = state->renderer.renderpassAttachmentCount,
            .pColorAttachments = colorAttachmentReferences,
        },
    };

    VkAttachmentDescription attachmentDescriptions[] = {
        (VkAttachmentDescription){
            .format = state->window.swapchain.format,
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
        },
    };

    VkSubpassDependency dependencies = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        // Destination is the first subpass
        .dstSubpass = 0U,
        // No mask needed at this time
        .srcAccessMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };

    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = sizeof(subpassDescriptions) / sizeof(*subpassDescriptions),
        .pSubpasses = subpassDescriptions,
        .attachmentCount = sizeof(attachmentDescriptions) / sizeof(*attachmentDescriptions),
        .pAttachments = attachmentDescriptions,
        .pDependencies = &dependencies,
    };

    LOG_IF_ERROR(vkCreateRenderPass(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.pRenderPass),
                 "Failed to create the render pass.")
}

void renderPassDestroy(State_t *state)
{
    vkDestroyRenderPass(state->context.device, state->renderer.pRenderPass, state->context.pAllocator);

    state->renderer.renderpassAttachmentCount = 0U;
}

void graphicsPipelineCreate(State_t *state)
{
    const char *shaderEntryFunctionName = "main";

    logger(LOG_INFO, "Loading shaders...");

    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        // Need to pass how big the code actuallly is...
        .codeSize = shaderVertCodeSize,
        // ... For it to correctly allocate storage for it here.
        .pCode = shaderVertCode,
    };
    VkShaderModule vertexShaderModule;
    LOG_IF_ERROR(vkCreateShaderModule(state->context.device, &vertexShaderModuleCreateInfo, state->context.pAllocator, &vertexShaderModule),
                 "Couldn't create the vertex shader module.")

    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderFragCodeSize,
        .pCode = shaderFragCode,
    };
    VkShaderModule fragmentShaderModule;
    LOG_IF_ERROR(vkCreateShaderModule(state->context.device, &fragmentShaderModuleCreateInfo, state->context.pAllocator, &fragmentShaderModule),
                 "Couldn't create the fragment shader module.")

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
        // No need to change defaults right now
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

    VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .lineWidth = 1.0F,
        // This one is super duper important. It is convention to have verticies rotate counter-clockwise. So a triangle would have points
        // 0, 1, and 2 at the top, bottom left, and then bottom right. This lets the renderer know which way a triangle is facing. If the
        // renderer is passed a triangle with verticies going clockwise when the front face is assigned as counter-clockwise then that
        // means the triangle is backwards/facing away from the camera reference and can be culled/etc.
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        // Tells the render pipeline what faces should be culled (hidden). Back but means that back (covered/blocked) bits won't be rendered.
        // This is literally the backface culling that makes Minecraft playable

        // .cullMode = VK_CULL_MODE_BACK_BIT, enable this when performance is needed and meshing and stuff is known to work. (backface culling)

        .cullMode = VK_CULL_MODE_NONE, // Both purely for debugging to make sure meshing and whatnot works.
        // Fill is opaque normally rendered object and line is wireframe
        .polygonMode = VK_POLYGON_MODE_FILL,
    };

    VkPipelineMultisampleStateCreateInfo multisamplingState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        // Don't care about multisampling for now
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
        // Color blending omitted at this time
        (VkPipelineColorBlendAttachmentState){
            // Bitwise OR to build the mask of what color bits the blend will write to
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        }};

    // Default blend constants applied to all the color blend attachments. When default, this declaration isn't necessary
    // but this is good to include for legibility. The struct doesn't accept the array directly and requires index replacement.
    float blendConstants[4] = {0.0F, 0.0F, 0.0F, 0.0F};

    // This configuration will need to be changed if alpha (transparency) is to be supported in the future.
    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = sizeof(colorBlendAttachmentStates) / sizeof(*colorBlendAttachmentStates),
        .pAttachments = colorBlendAttachmentStates,
        .blendConstants[0] = blendConstants[0],
        .blendConstants[1] = blendConstants[1],
        .blendConstants[2] = blendConstants[2],
        .blendConstants[3] = blendConstants[3],
    };

    VkDescriptorSetLayout descriptorSetLayouts[] = {
        (VkDescriptorSetLayout){
            0}, // Default
    };

    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = sizeof(descriptorSetLayouts) / sizeof(*descriptorSetLayouts),
        .pSetLayouts = descriptorSetLayouts};

    LOG_IF_ERROR(vkCreatePipelineLayout(state->context.device, &layoutCreateInfo, state->context.pAllocator, &state->renderer.pPipelineLayout),
                 "Failed to create the pipeline layout.");

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        // layout, pVertexInputState, and pInputAssemblyState all rely on the vertex layout
        .layout = state->renderer.pPipelineLayout,
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
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfos[] = {
        graphicsPipelineCreateInfo,
    };
    // Don't care about pipeline cache right now and only need to create 1 pipeline
    LOG_IF_ERROR(vkCreateGraphicsPipelines(state->context.device, NULL, 1U, graphicsPipelineCreateInfos, state->context.pAllocator,
                                           &state->renderer.pGraphicsPipeline),
                 "Failed to create the graphics pipeline.")

    // Once the render pipeline has been created, the shader information is stored within it. Thus, the shader modules can
    // actually be destroyed now. Do note that this means that if the shaders need to be changed, everything done in this function
    // must be re-performed.
    vkDestroyShaderModule(state->context.device, fragmentShaderModule, state->context.pAllocator);
    vkDestroyShaderModule(state->context.device, vertexShaderModule, state->context.pAllocator);
}

void graphicsPipelineDestroy(State_t *state)
{
    vkDestroyPipeline(state->context.device, state->renderer.pGraphicsPipeline, state->context.pAllocator);
    vkDestroyPipelineLayout(state->context.device, state->renderer.pPipelineLayout, state->context.pAllocator);
}

void framebuffersCreate(State_t *state)
{

    state->renderer.framebufferCount = state->window.swapchain.imageCount;
    state->renderer.pFramebuffers = malloc(sizeof(VkFramebuffer) * state->renderer.framebufferCount);

    LOG_IF_ERROR(!state->renderer.pFramebuffers,
                 "Failed to allocate memory for the framebuffers.")

    for (uint32_t framebufferIndex = 0U; framebufferIndex < state->renderer.framebufferCount; framebufferIndex++)
    {
        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            // Only one image layer for the current swapchain configuration
            .layers = 1U,
            .renderPass = state->renderer.pRenderPass,
            .width = state->window.swapchain.imageExtent.width,
            .height = state->window.swapchain.imageExtent.height,
            // The attachment count must be the same as the amount of attachment descriptions in the renderpass
            .attachmentCount = state->renderer.renderpassAttachmentCount,
            .pAttachments = &state->window.swapchain.pImageViews[framebufferIndex],
        };

        LOG_IF_ERROR(vkCreateFramebuffer(state->context.device, &createInfo, state->context.pAllocator,
                                         &state->renderer.pFramebuffers[framebufferIndex]),
                     "Failed to create frame buffer %d.", framebufferIndex)
    }
}

void framebuffersDestroy(State_t *state)
{
    // There is a hypothtical situation where the cause of detroying framebuffers is due to changing the amount of swapchain
    // images. In such a case, directly relying on state->window.swapchain.imageCount would be insufficient and cause a memory leak.
    for (uint32_t i = 0; i < state->renderer.framebufferCount; i++)
    {
        vkDestroyFramebuffer(state->context.device, state->renderer.pFramebuffers[i], state->context.pAllocator);
    }

    free(state->renderer.pFramebuffers);

    state->renderer.pFramebuffers = NULL;
    state->renderer.framebufferCount = 0U;
}

void commandPoolCreate(State_t *state)
{
    VkCommandPoolCreateFlags createFlags = {
        // Necessary because the command pool will be recaptured every frame
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };

    VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = state->context.queueFamily,
        .flags = createFlags,
    };

    LOG_IF_ERROR(vkCreateCommandPool(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.commandPool),
                 "Failed to create command pool.")
}

void commandPoolDestroy(State_t *state)
{
    vkDestroyCommandPool(state->context.device, state->renderer.commandPool, state->context.pAllocator);
}

void commandBufferAllocate(State_t *state)
{
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandBufferCount = 1U,
        .commandPool = state->renderer.commandPool,
        // Primary goes directly to the GPU and secondary goes to the GPU when called through primary
        // Kind of like how main() is the entry point and then main can call other funcitons
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    };

    // The associated destroy (deallocation) isn't required because Vulkan does it automatically
    LOG_IF_ERROR(vkAllocateCommandBuffers(state->context.device, &allocateInfo, &state->renderer.commandBuffer),
                 "Failed to allocate command buffer")
}

void commandBufferRecord(State_t *state)
{
    // Skip this recording frame if the swapchain will be/is being recreated (avoids null pointers)
    if (state->window.swapchain.recreate)
        return;

    // Because the command pool was created with VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, the command buffer
    // doesn't need to be reset here manually. If the command buffer has begun, DO NOT begin it again.

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };

    VkRect2D renderArea = {
        .extent = state->window.swapchain.imageExtent,
    };

    // Which color values to clear when using the clear operation defined in the attachments of the render pass.
    VkClearValue clearValues[] = {
        // Ex: clears image but leaves a background color
        (VkClearValue){
            // Black
            .color = 0,
        },
    };

    LOG_IF_ERROR(vkBeginCommandBuffer(state->renderer.commandBuffer, &commandBufferBeginInfo),
                 "Failed to begin the command buffer for frame %d.", state->window.swapchain.imageAcquiredIndex)

    // Avoid access violations
    if (!state->renderer.pFramebuffers || state->window.swapchain.imageAcquiredIndex >= state->renderer.framebufferCount)
    {
        logger(LOG_WARN, "Skipped an access violation during frame buffer access during commandBufferRecord!");
        return;
    }

    // ALL vkCmd functions (commands) MUST go between the begin and end command buffer functions (obviously)
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->renderer.pRenderPass,
        .framebuffer = state->renderer.pFramebuffers[state->window.swapchain.imageAcquiredIndex],
        .renderArea = renderArea,
        .clearValueCount = sizeof(clearValues) / sizeof(*clearValues),
        .pClearValues = clearValues,
    };

    // If secondary command buffers are used, use VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS
    vkCmdBeginRenderPass(state->renderer.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Required because the viewport is dynamic (resizeable)
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)state->window.swapchain.imageExtent.width,
        .height = (float)state->window.swapchain.imageExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(state->renderer.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = state->window.swapchain.imageExtent,
    };
    vkCmdSetScissor(state->renderer.commandBuffer, 0, 1, &scissor);

    // Access violation here when resizing the window

    // Bind the render pipeline to graphics (instead of compute)
    vkCmdBindPipeline(state->renderer.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pGraphicsPipeline);

    // DRAW ! ! ! ! !
    // currently hardcoded the vertex data in the (also hardcoded) shaders
    vkCmdDraw(state->renderer.commandBuffer, 3U, 1U, 0U, 0U);

    // Must end the render pass if has begun (obviously)
    vkCmdEndRenderPass(state->renderer.commandBuffer);

    // All errors generated from vkCmd functions will populate here. The vkCmd functions themselves are all void.
    LOG_IF_ERROR(vkEndCommandBuffer(state->renderer.commandBuffer),
                 "Failed to end the command buffer for frame %d.", state->window.swapchain.imageAcquiredIndex)
}

void commandBufferSubmit(State_t *state)
{
    VkPipelineStageFlags stageFlags[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };

    VkSubmitInfo submitInfos[] = {
        (VkSubmitInfo){
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            // Only one command buffer at this time (no array stored)
            .commandBufferCount = 1U,
            .pCommandBuffers = &state->renderer.commandBuffer,
            .waitSemaphoreCount = 1U,
            // Wait until the image is acquired
            .pWaitSemaphores = &state->renderer.imageAcquiredSemaphore,
            .signalSemaphoreCount = 1U,
            // Signal when the render is finished
            .pSignalSemaphores = &state->renderer.renderFinishedSemaphore,
            .pWaitDstStageMask = stageFlags,
        },
    };

    // Just one command submission right now
    // Signals the fence for when finished rendering
    LOG_IF_ERROR(vkQueueSubmit(state->context.queue, 1U, submitInfos, state->renderer.inFlightFence),
                 "Failed to submit queue to the command buffer.")
}

void syncObjectsCreate(State_t *state)
{
    // GPU operations are async so sync is required to aid in parallel execution
    // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
    // Fence: same above but for CPU
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        // Binary is just signaled/not signaled. Timeline is more states than 2 (0/1) basically.
    };

    LOG_IF_ERROR(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                   &state->renderer.imageAcquiredSemaphore),
                 "Failed to create image acquired semaphore")

    LOG_IF_ERROR(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                   &state->renderer.renderFinishedSemaphore),
                 "Failed to create render finished semaphore")

    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    };

    LOG_IF_ERROR(vkCreateFence(state->context.device, &fenceCreateInfo, state->context.pAllocator,
                               &state->renderer.inFlightFence),
                 "Failed to create in-flight fence")
}

void syncObjectsDestroy(State_t *state)
{
    vkDestroyFence(state->context.device, state->renderer.inFlightFence, state->context.pAllocator);
    vkDestroySemaphore(state->context.device, state->renderer.renderFinishedSemaphore, state->context.pAllocator);
    vkDestroySemaphore(state->context.device, state->renderer.imageAcquiredSemaphore, state->context.pAllocator);
}

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
void rendererCreate(State_t *state)
{
    renderPassCreate(state);
    graphicsPipelineCreate(state);
    framebuffersCreate(state);
    commandPoolCreate(state);
    commandBufferAllocate(state);
    syncObjectsCreate(state);
}

void rendererDestroy(State_t *state)
{
    // The GPU could be working on stuff for the renderer in parallel, meaning the renderer could be
    // destroyed while the GPU is still working. We should wait for the GPU to finish its current tasks.
    LOG_IF_ERROR(vkQueueWaitIdle(state->context.queue),
                 "Failed to wait for the Vulkan queue to be idle.")
    syncObjectsDestroy(state);
    commandPoolDestroy(state);
    framebuffersDestroy(state);
    graphicsPipelineDestroy(state);
    renderPassDestroy(state);
}
