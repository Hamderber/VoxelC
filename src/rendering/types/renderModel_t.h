#pragma once

#include <stdint.h>
#include <vulkan/vulkan.h>
#include "cmath/cmath.h"

typedef struct
{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexMemory;
    uint32_t indexCount;

    // Per-frame descriptor set (one per swapchain image) so we can point to the shared UBO + this model's texture
    VkDescriptorSet *pDescriptorSets; // size = renderer.maxFramesInFlight
    VkDescriptorPool descriptorPool;  // per-model pool

    // Texture owned by this model (from /res/textures)
    VkImage textureImage;
    VkDeviceMemory textureMemory;
    VkImageView textureView;
    VkSampler textureSampler;

    // world transform (set externally)
    Mat4c_t modelMatrix;
} RenderModel_t;
