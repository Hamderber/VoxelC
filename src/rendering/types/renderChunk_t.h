#pragma once

#include <vulkan/vulkan.h>
#include "cmath/cmath.h"
#include <stdbool.h>

typedef struct RenderChunk_t
{
    bool queuedForRemesh;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    // Capacity of verticies
    uint32_t vertexCapacity;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    // Capacity of indicies
    uint32_t indexCapacity;
    // How many indicies to draw for the current frame
    uint32_t indexCount;
    Mat4c_t modelMatrix;
} RenderChunk_t;