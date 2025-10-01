#pragma once

/* TODO:
   1. Add an additional pipeline that uses the line rendering mode to draw connections between verticies (for showing voxel hitboxes and chunks)
      or empty polygon mode maybe. Maybe polygon for mesh and line for chunks?
   2. Don't forget to change to VK_CULL_MODE_FRONT_BIT in the future
*/

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
void rendererCreate(State_t *state)
{
    // This is literally just a hardcoded copy-paste of the files generated from shaders.bat
    // This way is nice for not having to ship the shaders as separate files with the exe. But this is also
    // a huge pain in the ass for debugging shaders because I have to recompile and re-copy/paste every time.
    const uint32_t vertexShaderCode[] = {0x07230203, 0x00010000, 0x000d000b, 0x00000036,
                                         0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                                         0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
                                         0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                                         0x0008000f, 0x00000000, 0x00000004, 0x6e69616d,
                                         0x00000000, 0x00000022, 0x00000026, 0x00000031,
                                         0x00030003, 0x00000002, 0x000001cc, 0x000a0004,
                                         0x475f4c47, 0x4c474f4f, 0x70635f45, 0x74735f70,
                                         0x5f656c79, 0x656e696c, 0x7269645f, 0x69746365,
                                         0x00006576, 0x00080004, 0x475f4c47, 0x4c474f4f,
                                         0x6e695f45, 0x64756c63, 0x69645f65, 0x74636572,
                                         0x00657669, 0x00040005, 0x00000004, 0x6e69616d,
                                         0x00000000, 0x00050005, 0x0000000c, 0x69736f70,
                                         0x6e6f6974, 0x00000073, 0x00040005, 0x00000017,
                                         0x6f6c6f63, 0x00007372, 0x00060005, 0x00000020,
                                         0x505f6c67, 0x65567265, 0x78657472, 0x00000000,
                                         0x00060006, 0x00000020, 0x00000000, 0x505f6c67,
                                         0x7469736f, 0x006e6f69, 0x00070006, 0x00000020,
                                         0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953,
                                         0x00000000, 0x00070006, 0x00000020, 0x00000002,
                                         0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e,
                                         0x00070006, 0x00000020, 0x00000003, 0x435f6c67,
                                         0x446c6c75, 0x61747369, 0x0065636e, 0x00030005,
                                         0x00000022, 0x00000000, 0x00060005, 0x00000026,
                                         0x565f6c67, 0x65747265, 0x646e4978, 0x00007865,
                                         0x00050005, 0x00000031, 0x67617266, 0x6f6c6f43,
                                         0x00000072, 0x00030047, 0x00000020, 0x00000002,
                                         0x00050048, 0x00000020, 0x00000000, 0x0000000b,
                                         0x00000000, 0x00050048, 0x00000020, 0x00000001,
                                         0x0000000b, 0x00000001, 0x00050048, 0x00000020,
                                         0x00000002, 0x0000000b, 0x00000003, 0x00050048,
                                         0x00000020, 0x00000003, 0x0000000b, 0x00000004,
                                         0x00040047, 0x00000026, 0x0000000b, 0x0000002a,
                                         0x00040047, 0x00000031, 0x0000001e, 0x00000000,
                                         0x00020013, 0x00000002, 0x00030021, 0x00000003,
                                         0x00000002, 0x00030016, 0x00000006, 0x00000020,
                                         0x00040017, 0x00000007, 0x00000006, 0x00000002,
                                         0x00040015, 0x00000008, 0x00000020, 0x00000000,
                                         0x0004002b, 0x00000008, 0x00000009, 0x00000003,
                                         0x0004001c, 0x0000000a, 0x00000007, 0x00000009,
                                         0x00040020, 0x0000000b, 0x00000006, 0x0000000a,
                                         0x0004003b, 0x0000000b, 0x0000000c, 0x00000006,
                                         0x0004002b, 0x00000006, 0x0000000d, 0x00000000,
                                         0x0004002b, 0x00000006, 0x0000000e, 0xbf000000,
                                         0x0005002c, 0x00000007, 0x0000000f, 0x0000000d,
                                         0x0000000e, 0x0004002b, 0x00000006, 0x00000010,
                                         0x3f000000, 0x0005002c, 0x00000007, 0x00000011,
                                         0x00000010, 0x00000010, 0x0005002c, 0x00000007,
                                         0x00000012, 0x0000000e, 0x00000010, 0x0006002c,
                                         0x0000000a, 0x00000013, 0x0000000f, 0x00000011,
                                         0x00000012, 0x00040017, 0x00000014, 0x00000006,
                                         0x00000003, 0x0004001c, 0x00000015, 0x00000014,
                                         0x00000009, 0x00040020, 0x00000016, 0x00000006,
                                         0x00000015, 0x0004003b, 0x00000016, 0x00000017,
                                         0x00000006, 0x0004002b, 0x00000006, 0x00000018,
                                         0x3f800000, 0x0006002c, 0x00000014, 0x00000019,
                                         0x00000018, 0x0000000d, 0x0000000d, 0x0006002c,
                                         0x00000014, 0x0000001a, 0x0000000d, 0x00000018,
                                         0x0000000d, 0x0006002c, 0x00000014, 0x0000001b,
                                         0x0000000d, 0x0000000d, 0x00000018, 0x0006002c,
                                         0x00000015, 0x0000001c, 0x00000019, 0x0000001a,
                                         0x0000001b, 0x00040017, 0x0000001d, 0x00000006,
                                         0x00000004, 0x0004002b, 0x00000008, 0x0000001e,
                                         0x00000001, 0x0004001c, 0x0000001f, 0x00000006,
                                         0x0000001e, 0x0006001e, 0x00000020, 0x0000001d,
                                         0x00000006, 0x0000001f, 0x0000001f, 0x00040020,
                                         0x00000021, 0x00000003, 0x00000020, 0x0004003b,
                                         0x00000021, 0x00000022, 0x00000003, 0x00040015,
                                         0x00000023, 0x00000020, 0x00000001, 0x0004002b,
                                         0x00000023, 0x00000024, 0x00000000, 0x00040020,
                                         0x00000025, 0x00000001, 0x00000023, 0x0004003b,
                                         0x00000025, 0x00000026, 0x00000001, 0x00040020,
                                         0x00000028, 0x00000006, 0x00000007, 0x00040020,
                                         0x0000002e, 0x00000003, 0x0000001d, 0x00040020,
                                         0x00000030, 0x00000003, 0x00000014, 0x0004003b,
                                         0x00000030, 0x00000031, 0x00000003, 0x00040020,
                                         0x00000033, 0x00000006, 0x00000014, 0x00050036,
                                         0x00000002, 0x00000004, 0x00000000, 0x00000003,
                                         0x000200f8, 0x00000005, 0x0003003e, 0x0000000c,
                                         0x00000013, 0x0003003e, 0x00000017, 0x0000001c,
                                         0x0004003d, 0x00000023, 0x00000027, 0x00000026,
                                         0x00050041, 0x00000028, 0x00000029, 0x0000000c,
                                         0x00000027, 0x0004003d, 0x00000007, 0x0000002a,
                                         0x00000029, 0x00050051, 0x00000006, 0x0000002b,
                                         0x0000002a, 0x00000000, 0x00050051, 0x00000006,
                                         0x0000002c, 0x0000002a, 0x00000001, 0x00070050,
                                         0x0000001d, 0x0000002d, 0x0000002b, 0x0000002c,
                                         0x0000000d, 0x00000018, 0x00050041, 0x0000002e,
                                         0x0000002f, 0x00000022, 0x00000024, 0x0003003e,
                                         0x0000002f, 0x0000002d, 0x0004003d, 0x00000023,
                                         0x00000032, 0x00000026, 0x00050041, 0x00000033,
                                         0x00000034, 0x00000017, 0x00000032, 0x0004003d,
                                         0x00000014, 0x00000035, 0x00000034, 0x0003003e,
                                         0x00000031, 0x00000035, 0x000100fd, 0x00010038};

    const uint32_t fragmentShaderCode[] = {0x07230203, 0x00010000, 0x000d000b, 0x00000013,
                                           0x00000000, 0x00020011, 0x00000001, 0x0006000b,
                                           0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e,
                                           0x00000000, 0x0003000e, 0x00000000, 0x00000001,
                                           0x0007000f, 0x00000004, 0x00000004, 0x6e69616d,
                                           0x00000000, 0x00000009, 0x0000000c, 0x00030010,
                                           0x00000004, 0x00000007, 0x00030003, 0x00000002,
                                           0x000001cc, 0x000a0004, 0x475f4c47, 0x4c474f4f,
                                           0x70635f45, 0x74735f70, 0x5f656c79, 0x656e696c,
                                           0x7269645f, 0x69746365, 0x00006576, 0x00080004,
                                           0x475f4c47, 0x4c474f4f, 0x6e695f45, 0x64756c63,
                                           0x69645f65, 0x74636572, 0x00657669, 0x00040005,
                                           0x00000004, 0x6e69616d, 0x00000000, 0x00050005,
                                           0x00000009, 0x4374756f, 0x726f6c6f, 0x00000000,
                                           0x00050005, 0x0000000c, 0x67617266, 0x6f6c6f43,
                                           0x00000072, 0x00040047, 0x00000009, 0x0000001e,
                                           0x00000000, 0x00040047, 0x0000000c, 0x0000001e,
                                           0x00000000, 0x00020013, 0x00000002, 0x00030021,
                                           0x00000003, 0x00000002, 0x00030016, 0x00000006,
                                           0x00000020, 0x00040017, 0x00000007, 0x00000006,
                                           0x00000004, 0x00040020, 0x00000008, 0x00000003,
                                           0x00000007, 0x0004003b, 0x00000008, 0x00000009,
                                           0x00000003, 0x00040017, 0x0000000a, 0x00000006,
                                           0x00000003, 0x00040020, 0x0000000b, 0x00000001,
                                           0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c,
                                           0x00000001, 0x0004002b, 0x00000006, 0x0000000e,
                                           0x3f800000, 0x00050036, 0x00000002, 0x00000004,
                                           0x00000000, 0x00000003, 0x000200f8, 0x00000005,
                                           0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c,
                                           0x00050051, 0x00000006, 0x0000000f, 0x0000000d,
                                           0x00000000, 0x00050051, 0x00000006, 0x00000010,
                                           0x0000000d, 0x00000001, 0x00050051, 0x00000006,
                                           0x00000011, 0x0000000d, 0x00000002, 0x00070050,
                                           0x00000007, 0x00000012, 0x0000000f, 0x00000010,
                                           0x00000011, 0x0000000e, 0x0003003e, 0x00000009,
                                           0x00000012, 0x000100fd, 0x00010038};

    const char *shaderEntryFunctionName = "main";

    logger(LOG_INFO, "Creating shaders...");

    VkShaderModuleCreateInfo vertexShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        // Need to pass how big the code actuallly is...
        .codeSize = sizeof(vertexShaderCode),
        // ... For it to correctly allocate storage for it here.
        .pCode = vertexShaderCode,
    };
    VkShaderModule vertexShaderModule;
    LOG_IF_ERROR(vkCreateShaderModule(state->context.device, &vertexShaderModuleCreateInfo, state->context.pAllocator, &vertexShaderModule),
                 "Couldn't create the vertex shader module.")

    VkShaderModuleCreateInfo fragmentShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(fragmentShaderCode),
        .pCode = fragmentShaderCode,
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
        }};

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
        // Tells the render pipeline what faces should be drawn. Front but means that back (covered/blocked) bits won't be rendered. This is
        // literally the backface culling that makes Minecraft playable

        // .cullMode = VK_CULL_MODE_FRONT_BIT, enable this when performance is needed and meshing and stuff is known to work correctly

        .cullMode = VK_CULL_MODE_FRONT_AND_BACK, // Both purely for debugging to make sure meshing and whatnot works.
        // Fill is opaque normally rendered object and line is wireframe
        .polygonMode = VK_POLYGON_MODE_FILL,
    };

    VkPipelineMultisampleStateCreateInfo multisamplingState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        // Don't care about multisampling for now
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        . // https://youtu.be/6hFJvlYbtF0?si=1R0f-dthm0ZV-uDq&t=8143
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        // Get the length of the array by dividing the size of the shaderStages array by the size of the type of the first index of the array
        .stageCount = sizeof(shaderStages) / sizeof(*shaderStages),
        .pStages = &shaderStages,
        .pDynamicState = &dynamicState,
        .pVertexInputState = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisamplingState,
        .pColorBlendState = &colorBlendState};
    // Don't care about pipeline cache right now and only need to create 1 pipeline
    LOG_IF_ERROR(vkCreateGraphicsPipelines(state->context.device, NULL, 1U, &graphicsPipelineCreateInfo, state->context.pAllocator,
                                           &state->renderer.graphicsPipeline),
                 "Failed to create the graphics pipeline.")

    // Once the render pipeline has been created, the shader information is stored within it. Thus, the shader modules can
    // actually be destroyed now. Do note that this means that if the shaders need to be changed, everything done in this function
    // must be re-performed.
    vkDestroyShaderModule(state->context.device, fragmentShaderModule, state->context.pAllocator);
    vkDestroyShaderModule(state->context.device, vertexShaderModule, state->context.pAllocator);
}

void rendererDestroy(State_t *state)
{
    vkDestroyPipeline(state->context.device, state->renderer.graphicsPipeline, state->context.pAllocator);
}
