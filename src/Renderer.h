#pragma once

/* TODO:
   1. Add an additional pipeline that uses the line rendering mode to draw connections between verticies (for showing voxel hitboxes and chunks)
      or empty polygon mode maybe. Maybe polygon for mesh and line for chunks?
   2. Don't forget to change to VK_CULL_MODE_FRONT_BIT in the future
*/

// Rendering
typedef struct
{
    Vec3f_t pos;
    Vec3f_t color;
    Vec2f_t texCoord;
} ShaderVertex_t;

static const uint32_t NUM_SHADER_VERTEX_BINDING_DESCRIPTIONS = 1U;
// A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number
// of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
static inline const VkVertexInputBindingDescription *shaderVertexGetBindingDescription(void)
{
    static const VkVertexInputBindingDescription descriptions[1] = {
        {
            .binding = 0,
            .stride = sizeof(ShaderVertex_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,

        },
    };

    return descriptions;
}

static const uint32_t NUM_SHADER_VERTEX_ATTRIBUTES = 3U;
// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data originating
// from a binding description. We have two attributes, position and color, so we need two attribute description structs.
static inline const VkVertexInputAttributeDescription *shaderVertexGetInputAttributeDescriptions(void)
{
    static const VkVertexInputAttributeDescription descriptions[3] = {
        // Position
        {
            .binding = 0U,
            // The location for the position in the vertex shader
            .location = 0U,
            // Type of data (format is data type so color is same format as pos)
            // a float would be VK_FORMAT_R32_SFLOAT but a vec4 (ex quaternion or rgba) would be VK_FORMAT_R32G32B32A32_SFLOAT
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            // Specifies the number of bytes since the start of the per-vertex data to read from.
            // Position is first so 0
            .offset = 0U,
        },
        // Color
        {
            .binding = 0U,
            // The location for the color in the vertex shader
            .location = 1U,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            // Position is first so sizeof(pos) type to get offset
            .offset = sizeof(Vec2f_t),
        },
        {
            .binding = 0U,
            .location = 2U,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(ShaderVertex_t, texCoord),
        },
    };

    return descriptions;
}

VkFormat depthFormatGet(State_t *state)
{
    VkFormat formats[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    return formatSupportedFind(state, formats, sizeof(formats) / sizeof(*formats), VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

/// @brief The render pass is basically the blueprint for the graphics operation in the graphics pipeline
/// @param state
void renderPassCreate(State_t *state)
{
    VkAttachmentReference colorAttachmentReference = {
        // This 0 is the output location of the outColor vec4 in the fragment shader. If other outputs are needed, the attachment
        // number would be the same as the output location
        .attachment = 0U,
        // Render target for color output
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentReference depthAttachmentReference = {
        // Depth
        .attachment = 1U,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpassDescriptions[] = {
        (VkSubpassDescription){
            // Use for graphics pipeline instead of a compute pipeline
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments = &colorAttachmentReference,
            .pDepthStencilAttachment = &depthAttachmentReference,
        },
    };

    VkAttachmentDescription attachmentDescriptions[] = {
        // Present
        {
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
        // Depth
        {
            .format = depthFormatGet(state),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            // The depth data won't be stored because its not used after drawing
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
    };

    state->renderer.renderpassAttachmentCount = sizeof(attachmentDescriptions) / sizeof(*attachmentDescriptions);

    VkSubpassDependency dependencies[] = {
        (VkSubpassDependency){
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            // Destination is the first subpass
            .dstSubpass = 0U,
            // Wait in the pipeline for the previous external operations to finish before color attachment output
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        },
    };

    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .subpassCount = sizeof(subpassDescriptions) / sizeof(*subpassDescriptions),
        .pSubpasses = subpassDescriptions,
        .attachmentCount = state->renderer.renderpassAttachmentCount,
        .pAttachments = attachmentDescriptions,
        .dependencyCount = sizeof(dependencies) / sizeof(*dependencies),
        .pDependencies = dependencies,
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

    // This configuration will need to be changed if alpha (transparency) is to be supported in the future.
    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = sizeof(colorBlendAttachmentStates) / sizeof(*colorBlendAttachmentStates),
        .pAttachments = colorBlendAttachmentStates,
        // Default blend constants applied to all the color blend attachments. When default, this declaration isn't necessary
        // but this is good to include for legibility. The struct doesn't accept the array directly and requires index replacement.
        .blendConstants = {0, 0, 0, 0},
    };

    const VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &state->renderer.descriptorSetLayout,
    };

    LOG_IF_ERROR(vkCreatePipelineLayout(state->context.device, &layoutCreateInfo, state->context.pAllocator, &state->renderer.pipelineLayout),
                 "Failed to create the pipeline layout.");

    VkPipelineDepthStencilStateCreateInfo depthStencil = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        // If the depth of new fragments should be compared to the depth buffer to see if they should be discarded
        .depthTestEnable = VK_TRUE,
        // If the new depth of fragments that pass the depth test should actually be written to the depth buffer
        .depthWriteEnable = VK_TRUE,
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
        .layout = state->renderer.pipelineLayout,
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
    // Don't care about pipeline cache right now and only need to create 1 pipeline
    LOG_IF_ERROR(vkCreateGraphicsPipelines(state->context.device, NULL, 1U, graphicsPipelineCreateInfos, state->context.pAllocator,
                                           &state->renderer.graphicsPipeline),
                 "Failed to create the graphics pipeline.")

    // Once the render pipeline has been created, the shader information is stored within it. Thus, the shader modules can
    // actually be destroyed now. Do note that this means that if the shaders need to be changed, everything done in this function
    // must be re-performed.
    vkDestroyShaderModule(state->context.device, fragmentShaderModule, state->context.pAllocator);
    vkDestroyShaderModule(state->context.device, vertexShaderModule, state->context.pAllocator);
}

void graphicsPipelineDestroy(State_t *state)
{
    vkDestroyPipeline(state->context.device, state->renderer.graphicsPipeline, state->context.pAllocator);
    vkDestroyPipelineLayout(state->context.device, state->renderer.pipelineLayout, state->context.pAllocator);
}

void framebuffersCreate(State_t *state)
{

    state->renderer.framebufferCount = state->window.swapchain.imageCount;
    state->renderer.pFramebuffers = malloc(sizeof(VkFramebuffer) * state->renderer.framebufferCount);

    LOG_IF_ERROR(!state->renderer.pFramebuffers,
                 "Failed to allocate memory for the framebuffers.")

    for (uint32_t framebufferIndex = 0U; framebufferIndex < state->renderer.framebufferCount; framebufferIndex++)
    {
        // Building the image view here by just attaching the depth image view to the original swapchain image view
        VkImageView attachments[2] = {
            state->window.swapchain.pImageViews[framebufferIndex],
            state->renderer.depthImageView,
        };

        VkFramebufferCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            // Only one image layer for the current swapchain configuration
            .layers = 1U,
            .renderPass = state->renderer.pRenderPass,
            .width = state->window.swapchain.imageExtent.width,
            .height = state->window.swapchain.imageExtent.height,
            // The attachment count must be the same as the amount of attachment descriptions in the renderpass
            .attachmentCount = state->renderer.renderpassAttachmentCount,
            .pAttachments = attachments,
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

void bufferCreate(State_t *state, VkDeviceSize bufferSize, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags propertyFlags,
                  VkBuffer *buffer, VkDeviceMemory *bufferMemory)
{
    VkBufferCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bufferSize,
        .usage = usageFlags,
        // Don't need to share this buffer between queue families right now
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    LOG_IF_ERROR(vkCreateBuffer(state->context.device, &createInfo, state->context.pAllocator, buffer),
                 "Failed to create buffer!")

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(state->context.device, *buffer, &memoryRequirements);

    uint32_t memoryType = memoryTypeGet(state, memoryRequirements.memoryTypeBits, propertyFlags);

    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryType,
    };

    LOG_IF_ERROR(vkAllocateMemory(state->context.device, &allocateInfo, state->context.pAllocator, bufferMemory),
                 "Failed to allocate buffer memory!")

    // No offset required. If there was an offset, it would have to be divisible by memoryRequirements.alignment
    LOG_IF_ERROR(vkBindBufferMemory(state->context.device, *buffer, *bufferMemory, 0),
                 "Failed to bind buffer memory!")
}

VkCommandBuffer commandBufferSingleTimeBegin(State_t *state)
{
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->renderer.commandPool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    LOG_IF_ERROR(vkAllocateCommandBuffers(state->context.device, &allocateInfo, &commandBuffer),
                 "Failed to allocate command buffer!")

    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    LOG_IF_ERROR(vkBeginCommandBuffer(commandBuffer, &beginInfo),
                 "Failed to begin command buffer!")

    return commandBuffer;
}

void commandBufferSingleTimeEnd(State_t *state, VkCommandBuffer commandBuffer)
{
    LOG_IF_ERROR(vkEndCommandBuffer(commandBuffer),
                 "Failed to end command buffer!")

    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    // A fence would allow you to schedule multiple transfers simultaneously and wait for all of them complete,
    // instead of executing one at a time. That may give the driver more opportunities to optimize but is not
    // implemented at this time. Fence is passed as null and we just wait for the transfer queue to be idle right now.
    LOG_IF_ERROR(vkQueueSubmit(state->context.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE),
                 "Failed to submit graphicsQueue!")
    LOG_IF_ERROR(vkQueueWaitIdle(state->context.graphicsQueue),
                 "Failed to wait for graphics queue to idle!")

    vkFreeCommandBuffers(state->context.device, state->renderer.commandPool, 1, &commandBuffer);
}

void bufferCopy(State_t *state, VkBuffer sourceBuffer, VkBuffer destinationBuffer, VkDeviceSize size)
{
    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool = state->renderer.commandPool,
        .commandBufferCount = 1U,
    };

    VkCommandBuffer commandBuffer = commandBufferSingleTimeBegin(state);

    VkBufferCopy copyRegion = {
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    // Only copying one region
    vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);

    commandBufferSingleTimeEnd(state, commandBuffer);
}

void indexBufferCreateFromData(State_t *state, uint16_t *indices, uint32_t indexCount)
{
    VkDeviceSize bufferSize = sizeof(uint16_t) * indexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    bufferCreate(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingMemory);

    void *data;
    LOG_IF_ERROR(vkMapMemory(state->context.device, stagingMemory, 0, bufferSize, 0, &data),
                 "Failed to map index staging buffer memory.")
    memcpy(data, indices, (size_t)bufferSize);
    vkUnmapMemory(state->context.device, stagingMemory);

    bufferCreate(state, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->renderer.indexBuffer, &state->renderer.indexBufferMemory);

    bufferCopy(state, stagingBuffer, state->renderer.indexBuffer, bufferSize);

    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingMemory, state->context.pAllocator);

    logger(LOG_INFO, "Created index buffer (%u indices, %zu bytes).", indexCount, (size_t)bufferSize);
}

void indexBufferDestroy(State_t *state)
{
    vkDestroyBuffer(state->context.device, state->renderer.indexBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.indexBufferMemory, state->context.pAllocator);
}

void vertexBufferCreateFromData(State_t *state, ShaderVertex_t *vertices, uint32_t vertexCount)
{
    VkDeviceSize bufferSize = sizeof(ShaderVertex_t) * vertexCount;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    // Create staging buffer (CPU visible)
    bufferCreate(state, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingMemory);

    void *data;
    LOG_IF_ERROR(vkMapMemory(state->context.device, stagingMemory, 0, bufferSize, 0, &data),
                 "Failed to map vertex staging buffer memory.")
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(state->context.device, stagingMemory);

    // Create actual GPU vertex buffer
    bufferCreate(state, bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 &state->renderer.vertexBuffer, &state->renderer.vertexBufferMemory);

    // Copy from staging to GPU
    bufferCopy(state, stagingBuffer, state->renderer.vertexBuffer, bufferSize);

    // Cleanup staging
    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingMemory, state->context.pAllocator);

    logger(LOG_INFO, "Created vertex buffer (%u vertices, %zu bytes).", vertexCount, (size_t)bufferSize);
}

void vertexBufferDestroy(State_t *state)
{
    vkDestroyBuffer(state->context.device, state->renderer.vertexBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.vertexBufferMemory, state->context.pAllocator);
}

void bufferCopyToImage(State_t *state, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = commandBufferSingleTimeBegin(state);

    VkBufferImageCopy region = {
        // Byte offset
        .bufferOffset = 0,
        // Length and height of how buffer is laid out in memory
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount = 1,
        .imageOffset = {0, 0, 0},
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1,
        },
    };

    // Assuming that the image has already been transitioned to a copy-optimal layout
    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    commandBufferSingleTimeEnd(state, commandBuffer);
}

void imageLayoutTransition(State_t *state, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = commandBufferSingleTimeBegin(state);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        // Can use VK_IMAGE_LAYOUT_UNDEFINED if don't care about existing contents of the image
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        // Used if transferring queue family ownership
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        // Specifies the image that is affected and the specific part of the image. Currently just a non array/mipped image
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .srcAccessMask = 0, // TODO
        .dstAccessMask = 0, // TODO
    };

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        LOG_IF_ERROR(true,
                     "Unsupported layout transition!")
    }

    vkCmdPipelineBarrier(commandBuffer,
                         // Which pipeline stage the operations occur that should happen before the barrier
                         sourceStage,
                         // What pipeline stage in which operations will wait on the barrier
                         destinationStage,
                         0,
                         0, VK_NULL_HANDLE,
                         0, VK_NULL_HANDLE,
                         1, &barrier);

    commandBufferSingleTimeEnd(state, commandBuffer);
}

void imageCreate(State_t *state, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
                 VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory)
{
    VkImageCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        // What type of coordinate system the texels will be accessed by
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = width,
        .extent.height = height,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        // These formats must match or the copy will fail
        .format = format,
        // Must use the same format for the texels as the pixels in the buffer or the copy will fail
        .tiling = tiling,
        // Not usable by the GPU and the very first transition will discard the texels. New copy so safe to discard.
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // Will be used to copy (transfer) but also accessed by the shader (sampled)
        .usage = usage,
        // Only used by 1 queue family
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        // Look into https://vulkan-tutorial.com/Texture_mapping/Images discussion on sparse images (voxel usage)
        .flags = 0,
    };

    LOG_IF_ERROR(vkCreateImage(state->context.device, &createInfo, state->context.pAllocator, image),
                 "Failed to create image!")

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(state->context.device, *image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeGet(state, memoryRequirements.memoryTypeBits, properties),
    };

    LOG_IF_ERROR(vkAllocateMemory(state->context.device, &allocateInfo, state->context.pAllocator, imageMemory),
                 "Failed to allocate memory for texture images!")

    vkBindImageMemory(state->context.device, *image, *imageMemory, 0);
}

void textureImageCreate(State_t *state)
{
    int width, height, channels;
    const char *imagePath = RESOURCE_TEXTURE_PATH "hello_world.png";
    // Force the image to load with an alpha channel
    stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    // 4 bytes per pixel (RGBA)
    VkDeviceSize imageSize = width * height * 4;

    LOG_IF_ERROR(pixels == NULL,
                 "Failed to load texture %s!", imagePath)

    logger(LOG_INFO, "Loaded texture %s", imagePath);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    bufferCreate(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    // Map and copy the data into the staging buffer
    void *data;
    LOG_IF_ERROR(vkMapMemory(state->context.device, stagingBufferMemory, 0, imageSize, 0, &data),
                 "Failed to map texture staging buffer memory.")
    memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(state->context.device, stagingBufferMemory);
    // free the image array that was loaded
    stbi_image_free(pixels);

    imageCreate(state, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.textureImage, &state->renderer.textureImageMemory);

    // Transition the image for copy
    // Undefined because don't care about original contents of the image before the copy operation
    imageLayoutTransition(state, state->renderer.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    bufferCopyToImage(state, stagingBuffer, state->renderer.textureImage, (uint32_t)width, (uint32_t)height);

    // Transition the image for sampling
    imageLayoutTransition(state, state->renderer.textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingBufferMemory, state->context.pAllocator);
}

void textureImageDestroy(State_t *state)
{
    vkDestroyImage(state->context.device, state->renderer.textureImage, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.textureImageMemory, state->context.pAllocator);
}

void commandBufferAllocate(State_t *state)
{
    // free previous commmand buffers if present
    if (state->renderer.pCommandBuffers != NULL)
    {
        vkFreeCommandBuffers(state->context.device, state->renderer.commandPool,
                             state->config.maxFramesInFlight, state->renderer.pCommandBuffers);
        free(state->renderer.pCommandBuffers);
        state->renderer.pCommandBuffers = NULL;
    }

    state->renderer.pCommandBuffers = malloc(sizeof(VkCommandBuffer) * state->config.maxFramesInFlight);
    LOG_IF_ERROR(state->renderer.pCommandBuffers == NULL,
                 "Failed to allocate memory for command buffers")

    VkCommandBufferAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->renderer.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = state->config.maxFramesInFlight,
    };

    LOG_IF_ERROR(vkAllocateCommandBuffers(state->context.device, &allocateInfo, state->renderer.pCommandBuffers),
                 "Failed to allocate command buffers")
}

void commandBufferRecord(State_t *state)
{
    // Skip this recording frame if the swapchain will be/is being recreated (avoids null pointers)
    if (state->window.swapchain.recreate)
        return;

    VkRect2D renderArea = {
        .extent = state->window.swapchain.imageExtent,
    };

    // Which color values to clear when using the clear operation defined in the attachments of the render pass.
    // Order of clear values must be equal to order of attachments
    VkClearValue clearValues[] = {
        // Clears image but leaves a background color
        {
            // Black
            .color.float32 = {0.0F, 0.0F, 0.0F, 0.0F},
        },
        // Resets the depth stencil
        {
            .depthStencil = {1.0f, 0},
        },
    };

    // Avoid access violations
    if (!state->renderer.pFramebuffers || state->window.swapchain.imageAcquiredIndex >= state->renderer.framebufferCount)
    {
        logger(LOG_WARN, "Skipped an access violation during frame buffer access during commandBufferRecord!");
        return;
    }

    uint32_t frameIndex = state->renderer.currentFrame;
    VkCommandBuffer cmd = state->renderer.pCommandBuffers[frameIndex];

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
    };

    VkResult r = vkResetCommandBuffer(cmd, 0);
    if (r != VK_SUCCESS)
    {
        // Don't just do LOG_IF_ERROR because we want to early exit here
        logger(LOG_ERROR, "vkResetCommandBuffer failed (%d) for frame %u", r, frameIndex);
        return;
    }

    LOG_IF_ERROR(vkBeginCommandBuffer(cmd, &commandBufferBeginInfo),
                 "Failed to begin command buffer (frame %u)", frameIndex);

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
    vkCmdBeginRenderPass(cmd, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Required because the viewport is dynamic (resizeable)
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)state->window.swapchain.imageExtent.width,
        .height = (float)state->window.swapchain.imageExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = state->window.swapchain.imageExtent,
    };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    // Bind the render pipeline to graphics (instead of compute)
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.graphicsPipeline);

    VkBuffer vertexBuffers[] = {state->renderer.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    // 16 limits verticies to 65535 (consider once making own models and having a check?)
    vkCmdBindIndexBuffer(cmd, state->renderer.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    // No offset and 1 descriptor set bound for this frame
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, state->renderer.pipelineLayout,
                            0, 1, &state->renderer.pDescriptorSets[state->renderer.currentFrame], 0, VK_NULL_HANDLE);

    // DRAW ! ! ! ! !
    // Not using instanced rendering so just 1 instance with nothing for the offset
    vkCmdDrawIndexed(cmd, state->renderer.indexCount, 1, 0, 0, 0);

    // Must end the render pass if has begun (obviously)
    vkCmdEndRenderPass(cmd);

    // All errors generated from vkCmd functions will populate here. The vkCmd functions themselves are all void.
    LOG_IF_ERROR(vkEndCommandBuffer(cmd),
                 "Failed to end the command buffer for frame %d.", state->window.swapchain.imageAcquiredIndex)
}

void commandBufferSubmit(State_t *state)
{
    uint32_t frame = state->renderer.currentFrame;
    VkCommandBuffer cmd = state->renderer.pCommandBuffers[frame];

    VkPipelineStageFlags stageFlags[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1U,
        .pWaitSemaphores = &state->renderer.imageAcquiredSemaphores[frame],
        .pWaitDstStageMask = stageFlags,
        .commandBufferCount = 1U,
        .pCommandBuffers = &cmd,
        .signalSemaphoreCount = 1U,
        .pSignalSemaphores = &state->renderer.renderFinishedSemaphores[frame],
    };

    // Reset the fence *right before* submit (this is the only reset for this frame)
    LOG_IF_ERROR(vkResetFences(state->context.device, 1U, &state->renderer.inFlightFences[frame]),
                 "Failed to reset in-flight fence before submit (frame %u)", frame);

    VkResult r = vkQueueSubmit(state->context.graphicsQueue, 1U, &submitInfo, state->renderer.inFlightFences[frame]);

    LOG_IF_ERROR(r,
                 "Failed to submit graphicsQueue to the command buffer.");
}

void syncObjectsDestroy(State_t *state)
{
    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.inFlightFences != NULL)
        {
            vkDestroyFence(state->context.device, state->renderer.inFlightFences[i], state->context.pAllocator);
            state->renderer.inFlightFences[i] = VK_NULL_HANDLE;
        }
    }

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.renderFinishedSemaphores != NULL)
        {
            vkDestroySemaphore(state->context.device, state->renderer.renderFinishedSemaphores[i], state->context.pAllocator);
            state->renderer.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.imageAcquiredSemaphores != NULL)
        {
            vkDestroySemaphore(state->context.device, state->renderer.imageAcquiredSemaphores[i], state->context.pAllocator);
            state->renderer.imageAcquiredSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    free(state->renderer.inFlightFences);
    state->renderer.inFlightFences = NULL;
    free(state->renderer.imageAcquiredSemaphores);
    state->renderer.imageAcquiredSemaphores = NULL;
    free(state->renderer.renderFinishedSemaphores);
    state->renderer.renderFinishedSemaphores = NULL;
}

void syncObjectsCreate(State_t *state)
{
    // Destroy the original sync objects if they existed first
    syncObjectsDestroy(state);

    state->renderer.imageAcquiredSemaphores = malloc(sizeof(VkSemaphore) * state->config.maxFramesInFlight);
    LOG_IF_ERROR(state->renderer.imageAcquiredSemaphores == NULL,
                 "Failed to allcoate memory for image acquired semaphors!")

    state->renderer.renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * state->config.maxFramesInFlight);
    LOG_IF_ERROR(state->renderer.renderFinishedSemaphores == NULL,
                 "Failed to allcoate memory for render finished semaphors!")

    state->renderer.inFlightFences = malloc(sizeof(VkFence) * state->config.maxFramesInFlight);
    LOG_IF_ERROR(state->renderer.inFlightFences == NULL,
                 "Failed to allcoate memory for in-flight fences!")

    // GPU operations are async so sync is required to aid in parallel execution
    // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
    // Fence: same above but for CPU
    VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        // Binary is just signaled/not signaled. Timeline is more states than 2 (0/1) basically.
    };

    // Must start with the fences signaled so something actually renders initially
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        LOG_IF_ERROR(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                       &state->renderer.imageAcquiredSemaphores[i]),
                     "Failed to create image acquired semaphore")

        LOG_IF_ERROR(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                       &state->renderer.renderFinishedSemaphores[i]),
                     "Failed to create render finished semaphore")

        LOG_IF_ERROR(vkCreateFence(state->context.device, &fenceCreateInfo, state->context.pAllocator,
                                   &state->renderer.inFlightFences[i]),
                     "Failed to create in-flight fence")
    }

    state->renderer.currentFrame = 0;
}

void descriptorSetLayoutCreate(State_t *state)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        // Location for the ubo in the shader
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        // Specifices that these descriptors are for the vertex shader to reference
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        // Image sampling not implemented at this time
        .pImmutableSamplers = VK_NULL_HANDLE,
    };

    VkDescriptorSetLayoutBinding samplerBinding = {
        // Location in the shader
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        // No immutable samplers at this time
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding bindings[] = {
        uboLayoutBinding,
        samplerBinding,
    };

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof(bindings) / sizeof(*bindings),
        .pBindings = bindings,
    };

    LOG_IF_ERROR(vkCreateDescriptorSetLayout(state->context.device, &createInfo, state->context.pAllocator,
                                             &state->renderer.descriptorSetLayout),
                 "Failed to create descriptor set layout!")
}

void uniformBuffersCreate(State_t *state)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject_t);
    state->renderer.pUniformBuffers = malloc(sizeof(VkBuffer) * state->config.maxFramesInFlight);
    state->renderer.pUniformBufferMemories = malloc(sizeof(VkDeviceMemory) * state->config.maxFramesInFlight);
    state->renderer.pUniformBuffersMapped = malloc(sizeof(void *) * state->config.maxFramesInFlight);

    for (size_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        bufferCreate(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &state->renderer.pUniformBuffers[i], &state->renderer.pUniformBufferMemories[i]);

        LOG_IF_ERROR(vkMapMemory(state->context.device, state->renderer.pUniformBufferMemories[i], 0, bufferSize, 0,
                                 &state->renderer.pUniformBuffersMapped[i]),
                     "Failed to map memory for uniform buffer!")
    }
}

void uniformBuffersDestroy(State_t *state)
{
    for (size_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        vkDestroyBuffer(state->context.device, state->renderer.pUniformBuffers[i], state->context.pAllocator);
        vkUnmapMemory(state->context.device, state->renderer.pUniformBufferMemories[i]);
        vkFreeMemory(state->context.device, state->renderer.pUniformBufferMemories[i], state->context.pAllocator);
        state->renderer.pUniformBuffersMapped[i] = NULL;
    }

    free(state->renderer.pUniformBuffers);
    state->renderer.pUniformBuffers = NULL;
    free(state->renderer.pUniformBufferMemories);
    state->renderer.pUniformBufferMemories = NULL;
}

void updateUniformBuffer(State_t *state)
{
    // For animation we want the total time so that the shaders can rotate/etc objects and display them where they should be
    // instead of having stutter with frames.

    Quaternion_t qYaw = la_quatAngleAxis(la_deg2radf(90.0F) * (float)state->time.frameTimeTotal, Y_AXIS);
    Quaternion_t qPitch = la_quatAngleAxis(la_deg2radf(45.0F) * (float)state->time.frameTimeTotal, X_AXIS);
    Quaternion_t qCombined = la_quatMultiply(qYaw, qPitch);
    Mat4c_t model = la_quat2mat(qCombined);

    UniformBufferObject_t ubo = {
        .model = model,
        .view = la_lookAt((Vec3f_t){2.0F, 2.0F, 2.0F}, VEC3_ONE, FORWARD),
        .projection = la_perspective(la_deg2radf(45.0F),
                                     state->window.swapchain.imageExtent.width / (float)state->window.swapchain.imageExtent.height,
                                     0.1F, 10.0F),
    };

    memcpy(state->renderer.pUniformBuffersMapped[state->renderer.currentFrame], &ubo, sizeof(ubo));
}

void descriptorPoolCreate(State_t *state)
{
    VkDescriptorPoolSize poolSizes[] = {
        {
            // ubo
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = state->config.maxFramesInFlight,
        },
        {
            // sampler
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = state->config.maxFramesInFlight,
        },
    };

    uint32_t numPools = sizeof(poolSizes) / sizeof(*poolSizes);

    VkDescriptorPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = numPools,
        .pPoolSizes = poolSizes,
        .maxSets = state->config.maxFramesInFlight,
    };

    LOG_IF_ERROR(vkCreateDescriptorPool(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.descriptorPool),
                 "Failed to create descriptor pool!")
}

void descriptorPoolDestroy(State_t *state)
{
    vkDestroyDescriptorPool(state->context.device, state->renderer.descriptorPool, state->context.pAllocator);
}

void descriptorSetsCreate(State_t *state)
{
    VkDescriptorSetLayout *layouts = malloc(sizeof(VkDescriptorSetLayout) * state->config.maxFramesInFlight);
    LOG_IF_ERROR(layouts == NULL,
                 "Failed to allocate memory for descriptor set layouts!")
    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        layouts[i] = state->renderer.descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state->renderer.descriptorPool,
        .descriptorSetCount = state->config.maxFramesInFlight,
        .pSetLayouts = layouts,
    };

    state->renderer.pDescriptorSets = malloc(sizeof(VkDescriptorSet) * state->config.maxFramesInFlight);
    LOG_IF_ERROR(state->renderer.pDescriptorSets == NULL,
                 "Failed to allocate memory for descriptor sets!")

    LOG_IF_ERROR(vkAllocateDescriptorSets(state->context.device, &allocateInfo, state->renderer.pDescriptorSets),
                 "Failed to allocate descriptor sets!")

    free(layouts);

    // Populate descriptors

    for (uint32_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = state->renderer.pUniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject_t),
        };

        VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = state->renderer.textureImageView,
            .sampler = state->renderer.textureSampler,
        };

        VkWriteDescriptorSet descriptorWrites[] = {
            {
                // ubo
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = state->renderer.pDescriptorSets[i],
                // location in the vertex shader
                .dstBinding = 0,
                // the descriptors can be arrays but thats not implemented at this time so 0 is the first "index"
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &bufferInfo,
            },
            {
                // sampler
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = state->renderer.pDescriptorSets[i],
                // location in the vertex shader
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &imageInfo,
            },
        };

        // Dont copy so 0 copies and destination null
        vkUpdateDescriptorSets(state->context.device, sizeof(descriptorWrites) / sizeof(*descriptorWrites), descriptorWrites,
                               0, VK_NULL_HANDLE);
    }
}

void descriptorSetsDestroy(State_t *state)
{
    // The descriptor set itself is freed by Vulkan when the descriptor pool is freed
    vkDestroyDescriptorSetLayout(state->context.device, state->renderer.descriptorSetLayout, state->context.pAllocator);
}

VkImageView imageViewCreate(State_t *state, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{

    VkImageViewCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange.aspectMask = aspectFlags,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
        .components = state->config.swapchainComponentMapping,
    };

    VkImageView textureImageView;
    LOG_IF_ERROR(vkCreateImageView(state->context.device, &createInfo, state->context.pAllocator, &textureImageView),
                 "Failed to create image view!")

    return textureImageView;
}

void textureViewImageCreate(State_t *state)
{
    // Written this way to support looping in the future
    state->renderer.textureImageView = imageViewCreate(state, state->renderer.textureImage,
                                                       VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void textureImageViewDestroy(State_t *state)
{
    vkDestroyImageView(state->context.device, state->renderer.textureImageView, state->context.pAllocator);
}

void textureSamplerCreate(State_t *state)
{
    float af = (float)state->renderer.anisotropicFilteringOptions[state->renderer.anisotropicFilteringOptionsCount - 1];
    VkSamplerCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        // Filters specify how to interpolate texels that are magnified or minified
        // Mag = oversampling and Min = undersampling
        // See https://vulkan-tutorial.com/Texture_mapping/Image_view_and_sampler
        // Linear for larger textures (interpolation)
        // .magFilter = VK_FILTER_LINEAR,
        // .minFilter = VK_FILTER_LINEAR,
        // Nearest for small textures (pixel art)
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        // Addressing can be defined per-axis and for some reason its UVW instead of XYZ (texture space convention)
        // Repeat is the most commonly used (floors, etc.) for tiling
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = af,
        // Which color is returned when sampling beyond the image with clamp to border addressing mode. It is possible to
        // return black, white or transparent in either float or int formats
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        // Which coordinate system you want to use to address texels in an image. If this field is VK_TRUE, then you can simply
        // use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed
        // using the [0, 1) range on all axes. Real-world applications almost always use normalized coordinates, because then
        // it's possible to use textures of varying resolutions with the exact same coordinates
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        // Mipmapping not implemneted at this time
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    logger(LOG_WARN, "Anisotropy is hard-coded to select the highest available (%.fx) at this time.", af);

    LOG_IF_ERROR(vkCreateSampler(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.textureSampler),
                 "Failed to create texture sampler!")
}

void textureSamplerDestroy(State_t *state)
{
    vkDestroySampler(state->context.device, state->renderer.textureSampler, state->context.pAllocator);
}

bool formatHasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void depthResourcesCreate(State_t *state)
{
    VkFormat depthFormat = depthFormatGet(state);

    imageCreate(state, state->window.swapchain.imageExtent.width, state->window.swapchain.imageExtent.height,
                depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.depthImage, &state->renderer.depthImageMemory);

    state->renderer.depthImageView = imageViewCreate(state, state->renderer.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    // Don't need to explicitly transition the layout of the image to a depth attachment because taken care of in the render pass
}

void depthResourcesDestroy(State_t *state)
{
    vkDestroyImageView(state->context.device, state->renderer.depthImageView, state->context.pAllocator);
    vkDestroyImage(state->context.device, state->renderer.depthImage, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.depthImageMemory, state->context.pAllocator);
}

void modelLoad(State_t *state)
{
    const char *path = MODEL_PATH "hello_world.glb";

    cgltf_options options = {0};
    cgltf_data *data = NULL;

    // Parse the glTF model file
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success)
    {
        logger(LOG_ERROR, "Failed to parse glTF file: %s", path);
        return;
    }

    // Load external or embedded buffer data (needed for vertices, indices, etc.)
    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        logger(LOG_ERROR, "Failed to load buffers for: %s", path);
        cgltf_free(data);
        return;
    }

    // Validate structure (optional, but useful for debugging)
    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        logger(LOG_ERROR, "Invalid glTF data in: %s", path);
        cgltf_free(data);
        return;
    }

    logger(LOG_INFO, "Loaded model: %s", path);
    logger(LOG_INFO, "Meshes: %zu", data->meshes_count);
    logger(LOG_INFO, "Materials: %zu", data->materials_count);
    logger(LOG_INFO, "Nodes: %zu", data->nodes_count);

    // For now, only load the first mesh and its first primitive
    if (data->meshes_count == 0)
    {
        logger(LOG_ERROR, "Model has no meshes! %s", path);
        cgltf_free(data);
        return;
    }

    cgltf_mesh *mesh = &data->meshes[0];
    if (mesh->primitives_count == 0)
    {
        logger(LOG_ERROR, "Mesh has no primitives! %s", path);
        cgltf_free(data);
        return;
    }

    cgltf_primitive *prim = &mesh->primitives[0];

    logger(LOG_INFO, "Loading mesh: %s (%zu primitives)", mesh->name ? mesh->name : "(unnamed)", mesh->primitives_count);

    // Extract the accessors for position, texcoord, and optional color
    cgltf_accessor *posAccessor = NULL;
    cgltf_accessor *uvAccessor = NULL;
    for (size_t k = 0; k < prim->attributes_count; k++)
    {
        cgltf_attribute *attr = &prim->attributes[k];
        if (attr->type == cgltf_attribute_type_position)
            posAccessor = attr->data;
        else if (attr->type == cgltf_attribute_type_texcoord)
            uvAccessor = attr->data;
    }

    if (posAccessor == NULL)
    {
        logger(LOG_ERROR, "Model missing position attribute! %s", path);
        cgltf_free(data);
        return;
    }

    size_t vertexCount = posAccessor->count;
    ShaderVertex_t *vertices = malloc(sizeof(ShaderVertex_t) * vertexCount);
    LOG_IF_ERROR(vertices == NULL, "Failed to allocate vertex array for model: %s", path);

    float tmp[4];
    for (size_t v = 0; v < vertexCount; v++)
    {
        // Positions
        cgltf_accessor_read_float(posAccessor, v, tmp, 3);
        vertices[v].pos = (Vec3f_t){tmp[0], tmp[1], tmp[2]};

        // Texture coordinates
        if (uvAccessor)
        {
            cgltf_accessor_read_float(uvAccessor, v, tmp, 2);
            // Flip V coordinate to match Vulkan convention
            vertices[v].texCoord = (Vec2f_t){tmp[0], 1.0f - tmp[1]};
        }
        else
        {
            vertices[v].texCoord = (Vec2f_t){0.0f, 0.0f};
        }

        // Default white color for now (can be replaced if color attributes are added)
        vertices[v].color = WHITE;
    }

    // Load indices if they exist
    uint16_t *indices = NULL;
    size_t indexCount = 0;
    if (prim->indices)
    {
        cgltf_accessor *indexAccessor = prim->indices;
        indexCount = indexAccessor->count;
        indices = malloc(sizeof(uint16_t) * indexCount);
        LOG_IF_ERROR(indices == NULL, "Failed to allocate index array for model: %s", path);

        for (size_t i = 0; i < indexCount; i++)
        {
            indices[i] = (uint16_t)cgltf_accessor_read_index(indexAccessor, i);
        }
    }
    else
    {
        // Fallback: sequential indices
        indexCount = vertexCount;
        indices = malloc(sizeof(uint16_t) * indexCount);
        LOG_IF_ERROR(indices == NULL, "Failed to allocate fallback indices for model: %s", path);

        for (size_t i = 0; i < indexCount; i++)
        {
            indices[i] = (uint16_t)i;
        }
    }

    logger(LOG_INFO, "Model vertex count: %zu", vertexCount);
    logger(LOG_INFO, "Model index count: %zu", indexCount);

    // Upload to GPU
    vertexBufferCreateFromData(state, vertices, (uint32_t)vertexCount);
    indexBufferCreateFromData(state, indices, (uint32_t)indexCount);

    // Store index count in renderer for drawing
    state->renderer.indexCount = (uint32_t)indexCount;

    // Cleanup CPU-side memory
    free(vertices);
    free(indices);
    cgltf_free(data);

    logger(LOG_INFO, "Model successfully uploaded to GPU: %s", path);
}

void modelDestroy(State_t *state)
{
    // Placeholder. Model destruction currently handled by buffer destruction
}

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
void rendererCreate(State_t *state)
{
    renderPassCreate(state);
    descriptorSetLayoutCreate(state);
    graphicsPipelineCreate(state);
    commandPoolCreate(state);
    modelLoad(state);
    // vertexBufferCreate(state);
    // indexBufferCreate(state);
    uniformBuffersCreate(state);
    depthResourcesCreate(state);
    framebuffersCreate(state);
    textureImageCreate(state);
    textureViewImageCreate(state);
    textureSamplerCreate(state);
    descriptorPoolCreate(state);
    descriptorSetsCreate(state);
    commandBufferAllocate(state);
    syncObjectsCreate(state);
}

void rendererDestroy(State_t *state)
{
    // The GPU could be working on stuff for the renderer in parallel, meaning the renderer could be
    // destroyed while the GPU is still working. We should wait for the GPU to finish its current tasks.
    LOG_IF_ERROR(vkQueueWaitIdle(state->context.graphicsQueue),
                 "Failed to wait for the Vulkan graphicsQueue to be idle.")
    syncObjectsDestroy(state);
    textureSamplerDestroy(state);
    textureImageViewDestroy(state);
    textureImageDestroy(state);
    descriptorSetsDestroy(state);
    descriptorPoolDestroy(state);
    framebuffersDestroy(state);
    depthResourcesDestroy(state);
    uniformBuffersDestroy(state);
    indexBufferDestroy(state);
    vertexBufferDestroy(state);
    modelDestroy(state);
    commandPoolDestroy(state);
    graphicsPipelineDestroy(state);
    renderPassDestroy(state);
}
