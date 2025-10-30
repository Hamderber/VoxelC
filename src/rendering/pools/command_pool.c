#include <vulkan/vulkan.h>
#include "core/logs.h"
#include "core/types/state_t.h"
#include "core/crash_handler.h"

void commandPool_create(State_t *pState)
{
    do
    {
        const VkCommandPoolCreateFlags CREATE_FLAGS = {
            // Necessary because the command pool will be recaptured every frame
            VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };

        const VkCommandPoolCreateInfo CREATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = pState->context.queueFamily,
            .flags = CREATE_FLAGS,
        };

        if (vkCreateCommandPool(pState->context.device, &CREATE_INFO, pState->context.pAllocator, &pState->renderer.commandPool) != VK_SUCCESS)
        {
            logs_log(LOG_ERROR, "Failed to create the rendering command pool!");
            break;
        }

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a command pool for GPU control.");
}

void commandPool_destroy(State_t *pState)
{
    vkDestroyCommandPool(pState->context.device, pState->renderer.commandPool, pState->context.pAllocator);
}