#pragma once
#include <vulkan/vulkan.h>
#include "collection/dynamicStack_t.h"

typedef struct PendingBufferDestroy_t
{
    VkBuffer buffer;
    VkDeviceMemory memory;
} PendingBufferDestroy_t;

void renderGC_updateFrame(const uint32_t FRAME_INDEX);

void renderGC_pushGarbage(VkBuffer buffer, VkDeviceMemory memory);

void renderGC_flushGarbage(const uint32_t FRAME_INDEX, const bool FLUSH_ALL);

/// @brief Caches the pointer to the pState that persists for app lifetime
void renderGC_init(VkDevice device, uint32_t maxFramesInFlightCfg, VkAllocationCallbacks *pAllocatorCtx);

void renderGC_destroy(void);