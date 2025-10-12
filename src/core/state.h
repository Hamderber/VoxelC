#pragma once

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include "core/config.h"
#include "rendering/atlas_texture.h"

typedef struct
{
    // Time since last frame
    double frameTimeDelta;
    // Actual last time (not delta)
    double frameTimeLast;
    double frameTimeTotal;
    double framesPerSecond;
    // Fixed-step physics
    double fixedTimeAccumulated;
} Time_t;

typedef struct
{
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    VkQueue graphicsQueue;
    // This is always null right now so that Vulkan uses its own allocator
    VkAllocationCallbacks *pAllocator;
    /// @brief UINT32_MAX means no family assigned (set to max during creation)
    uint32_t queueFamily;
} Context_t;

typedef struct
{
    // Swapchain
    // https://www.youtube.com/watch?v=nSzQcyQTtRY
    VkSwapchainKHR handle;
    uint32_t imageCount;
    uint32_t imageAcquiredIndex;
    bool recreate;
    VkImage *pImages;
    VkImageView *pImageViews;
    VkFormat format;
    VkColorSpaceKHR colorSpace;
    VkExtent2D imageExtent;
} Swapchain_t;

typedef struct
{
    // Vulkan
    Swapchain_t swapchain;
    VkSurfaceKHR surface;

    // GLFW
    GLFWwindow *pWindow;
    int frameBufferWidth;
    int frameBufferHeight;
} Window_t;

typedef struct
{
    VkPipeline graphicsPipeline;
    VkPipelineLayout pipelineLayout;
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

typedef struct
{
    Config_t config;
    Window_t window;
    Context_t context;
    Renderer_t renderer;
    Time_t time;
} State_t;
