#pragma region Includes
#pragma once
#include <vulkan/vulkan.h>
#include "core/logs.h"
#pragma endregion
#pragma region Wrappers
VkResult vkMapMemory_wrapper(const char *restrict pDebugMessage, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset,
                             VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
    logs_log(LOG_DEBUG, "(vkMapMemory) %s", pDebugMessage);
    return vkMapMemory(device, memory, offset, size, flags, ppData);
}
#pragma endregion