#pragma region Includes
#pragma once
#include <vulkan/vulkan.h>
#include "core/logs.h"
#pragma endregion
#pragma region Wrappers
// #define VK_WRAPPER_DEBUG
static inline VkResult vkMapMemory_wrapper(const char *restrict pDebugMessage, VkDevice device, VkDeviceMemory memory, VkDeviceSize offset,
                                           VkDeviceSize size, VkMemoryMapFlags flags, void **ppData)
{
#ifdef VK_WRAPPER_DEBUG
    logs_log(LOG_DEBUG, "(vkMapMemory) %s", pDebugMessage);
#endif
    pDebugMessage;
    return vkMapMemory(device, memory, offset, size, flags, ppData);
}
#pragma endregion
#pragma region Undefines
#undef VK_WRAPPER_DEBUG
#pragma endregion