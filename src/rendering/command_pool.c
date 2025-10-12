#include <vulkan/vulkan.h>
#include "core/types/state_t.h"

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

    logs_logIfError(vkCreateCommandPool(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.commandPool),
                    "Failed to create command pool.")
}

void commandPoolDestroy(State_t *state)
{
    vkDestroyCommandPool(state->context.device, state->renderer.commandPool, state->context.pAllocator);
}