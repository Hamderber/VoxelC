#pragma once
#include <vulkan/vulkan.h>
#include "collection/dynamicStack_t.h"

typedef struct PendingBufferDestroy_t
{
    VkBuffer buffer;
    VkDeviceMemory memory;
} PendingBufferDestroy_t;

void renderGC_pushGarbage(const uint32_t FRAME_INDEX, VkBuffer buffer, VkDeviceMemory memory);

void renderGC_flushGarbage(State_t *pState, const uint32_t FRAME_INDEX);

void renderGC_init(State_t *pState);

void renderGC_destroy(State_t *pState);