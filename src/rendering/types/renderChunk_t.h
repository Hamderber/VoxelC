#pragma once

#include <vulkan/vulkan.h>
#include "cmath/cmath.h"

typedef struct RenderChunk_t
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t indexCount;
    Mat4c_t modelMatrix;
} RenderChunk_t;