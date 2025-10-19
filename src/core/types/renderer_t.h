#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include "core/config.h"
#include "core/types/atlasRegion_t.h"
#include "rendering/types/graphicsPipeline_t.h"

typedef struct
{
    // Normal rendering
    VkPipeline graphicsPipelineFill;
    VkPipelineLayout pipelineLayoutFill;

    // Vertex wireframe rendering
    VkPipeline graphicsPipelineWireframe;
    VkPipelineLayout pipelineLayoutWireframe;

    GraphicsPipeline_t activeGraphicsPipeline;

    uint32_t renderpassAttachmentCount;
    VkRenderPass pRenderPass;
    uint32_t framebufferCount;
    VkFramebuffer *pFramebuffers;
    VkCommandPool commandPool;
    VkCommandBuffer *pCommandBuffers;
    VkSemaphore *imageAcquiredSemaphores;
    VkSemaphore *renderFinishedSemaphores;
    VkFence *inFlightFences;
    VkFence *imagesInFlight;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    VkDescriptorSetLayout descriptorSetLayout;
    VkBuffer *pUniformBuffers;
    VkDeviceMemory *pUniformBufferMemories;
    // Array of pointers that Vulkan uses to access uniform buffers and their memory
    void **pUniformBuffersMapped;
    uint32_t currentFrame;
    VkDescriptorPool descriptorPool;
    VkDescriptorSet *pDescriptorSets;
    // Change these to arrays once more than one texture is loaded
    VkImage atlasTextureImage;
    VkDeviceMemory atlasTextureImageMemory;
    VkImageView atlasTextureImageView;
    uint32_t anisotropicFilteringOptionsCount;
    AnisotropicFilteringOptions_t *anisotropicFilteringOptions;
    VkSampler textureSampler;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;
    uint32_t modelIndexCount;
    uint32_t atlasRegionCount;
    uint32_t atlasWidthInTiles;
    uint32_t atlasHeightInTiles;
    AtlasRegion_t *pAtlasRegions;
} Renderer_t;