#pragma once

void rendererCreate(State_t *state)
{
    VkGraphicsPipelineCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    };
    vkCreateGraphicsPipelines(state->context.device, NULL, 1U, &createInfo, state->context.allocator, &state->renderer.graphicsPipeline);
}

void rendererDestroy(State_t *state)
{
    // Placeholder
}
