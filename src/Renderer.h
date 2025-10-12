#pragma once

#include <string.h>
#include "stb_image.h"
#include "cgltf.h"
#include "rendering/texture.h"
#include "rendering/voxel.h"
#include "rendering/render_pass.h"
#include "rendering/depth.h"
#include "rendering/graphics_pipeline.h"
#include "rendering/buffers/buffers.h"
#include "rendering/model_3d.h"
#include "rendering/buffers/command_buffer.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/buffers/frame_buffer.h"
#include "rendering/atlas_texture.h"
#include "rendering/command_pool.h"
#include "rendering/image.h"

void atlasTextureImageCreate(State_t *state)
{
    int width, height, channels;
    const char *imagePath = RESOURCE_TEXTURE_PATH TEXTURE_ATLAS;
    // Flip the uv vertically to match face implementation
    stbi_set_flip_vertically_on_load(true);

    // Force the image to load with an alpha channel
    stbi_uc *pixels = stbi_load(imagePath, &width, &height, &channels, STBI_rgb_alpha);
    // 4 bytes per pixel (RGBA)
    VkDeviceSize imageSize = width * height * 4;

    logs_log(LOG_INFO, "Atlas PNG: %dx%d px, subtextureSize=%u px", width, height, state->config.subtextureSize);
    state->renderer.atlasWidthInTiles = width / state->config.subtextureSize;
    state->renderer.atlasHeightInTiles = height / state->config.subtextureSize;
    state->renderer.atlasRegionCount = state->renderer.atlasWidthInTiles * state->renderer.atlasHeightInTiles;
    logs_log(LOG_INFO, "The atlas texture has %u regions.", state->renderer.atlasRegionCount);

    logs_logIfError(pixels == NULL,
                    "Failed to load texture %s!", imagePath)

        logs_log(LOG_INFO, "Loaded texture %s", imagePath);

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    bufferCreate(state, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &stagingBuffer, &stagingBufferMemory);

    // Map and copy the data into the staging buffer
    void *data;
    logs_logIfError(vkMapMemory(state->context.device, stagingBufferMemory, 0, imageSize, 0, &data),
                    "Failed to map texture staging buffer memory.")
        memcpy(data, pixels, (size_t)imageSize);
    vkUnmapMemory(state->context.device, stagingBufferMemory);
    // free the image array that was loaded
    stbi_image_free(pixels);

    imageCreate(state, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.atlasTextureImage, &state->renderer.atlasTextureImageMemory);

    // Transition the image for copy
    // Undefined because don't care about original contents of the image before the copy operation
    imageLayoutTransition(state, state->renderer.atlasTextureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    bufferCopyToImage(state, stagingBuffer, state->renderer.atlasTextureImage, (uint32_t)width, (uint32_t)height);

    // Transition the image for sampling
    imageLayoutTransition(state, state->renderer.atlasTextureImage, VK_FORMAT_R8G8B8A8_SRGB,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(state->context.device, stagingBuffer, state->context.pAllocator);
    vkFreeMemory(state->context.device, stagingBufferMemory, state->context.pAllocator);
}

void atlasTextureImageDestroy(State_t *state)
{
    vkDestroyImage(state->context.device, state->renderer.atlasTextureImage, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.atlasTextureImageMemory, state->context.pAllocator);
}

void syncObjectsDestroy(State_t *state)
{
    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.inFlightFences != NULL)
        {
            vkDestroyFence(state->context.device, state->renderer.inFlightFences[i], state->context.pAllocator);
            state->renderer.inFlightFences[i] = VK_NULL_HANDLE;
        }
    }

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.renderFinishedSemaphores != NULL)
        {
            vkDestroySemaphore(state->context.device, state->renderer.renderFinishedSemaphores[i], state->context.pAllocator);
            state->renderer.renderFinishedSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        if (state->renderer.imageAcquiredSemaphores != NULL)
        {
            vkDestroySemaphore(state->context.device, state->renderer.imageAcquiredSemaphores[i], state->context.pAllocator);
            state->renderer.imageAcquiredSemaphores[i] = VK_NULL_HANDLE;
        }
    }

    free(state->renderer.inFlightFences);
    state->renderer.inFlightFences = NULL;
    free(state->renderer.imageAcquiredSemaphores);
    state->renderer.imageAcquiredSemaphores = NULL;
    free(state->renderer.renderFinishedSemaphores);
    state->renderer.renderFinishedSemaphores = NULL;
}

void syncObjectsCreate(State_t *state)
{
    // Destroy the original sync objects if they existed first
    syncObjectsDestroy(state);

    state->renderer.imageAcquiredSemaphores = malloc(sizeof(VkSemaphore) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.imageAcquiredSemaphores == NULL,
                    "Failed to allcoate memory for image acquired semaphors!")

        state->renderer.renderFinishedSemaphores = malloc(sizeof(VkSemaphore) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.renderFinishedSemaphores == NULL,
                    "Failed to allcoate memory for render finished semaphors!")

        state->renderer.inFlightFences = malloc(sizeof(VkFence) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.inFlightFences == NULL,
                    "Failed to allcoate memory for in-flight fences!")

        // GPU operations are async so sync is required to aid in parallel execution
        // Semaphore: (syncronization) action signal for GPU processes. Cannot continue until the relavent semaphore is complete
        // Fence: same above but for CPU
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            // Binary is just signaled/not signaled. Timeline is more states than 2 (0/1) basically.
        };

    // Must start with the fences signaled so something actually renders initially
    VkFenceCreateInfo fenceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        logs_logIfError(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                          &state->renderer.imageAcquiredSemaphores[i]),
                        "Failed to create image acquired semaphore")

            logs_logIfError(vkCreateSemaphore(state->context.device, &semaphoreCreateInfo, state->context.pAllocator,
                                              &state->renderer.renderFinishedSemaphores[i]),
                            "Failed to create render finished semaphore")

                logs_logIfError(vkCreateFence(state->context.device, &fenceCreateInfo, state->context.pAllocator,
                                              &state->renderer.inFlightFences[i]),
                                "Failed to create in-flight fence")
    }

    state->renderer.currentFrame = 0;
}

void descriptorSetLayoutCreate(State_t *state)
{
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
        // Location for the ubo in the shader
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        // Specifices that these descriptors are for the vertex shader to reference
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        // Image sampling not implemented at this time
        .pImmutableSamplers = VK_NULL_HANDLE,
    };

    VkDescriptorSetLayoutBinding samplerBinding = {
        // Location in the shader
        .binding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        // No immutable samplers at this time
        .pImmutableSamplers = VK_NULL_HANDLE,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    VkDescriptorSetLayoutBinding bindings[] = {
        uboLayoutBinding,
        samplerBinding,
    };

    VkDescriptorSetLayoutCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = sizeof(bindings) / sizeof(*bindings),
        .pBindings = bindings,
    };

    logs_logIfError(vkCreateDescriptorSetLayout(state->context.device, &createInfo, state->context.pAllocator,
                                                &state->renderer.descriptorSetLayout),
                    "Failed to create descriptor set layout!")
}

void uniformBuffersCreate(State_t *state)
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject_t);
    state->renderer.pUniformBuffers = malloc(sizeof(VkBuffer) * state->config.maxFramesInFlight);
    state->renderer.pUniformBufferMemories = malloc(sizeof(VkDeviceMemory) * state->config.maxFramesInFlight);
    state->renderer.pUniformBuffersMapped = malloc(sizeof(void *) * state->config.maxFramesInFlight);

    for (size_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        bufferCreate(state, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                     &state->renderer.pUniformBuffers[i], &state->renderer.pUniformBufferMemories[i]);

        logs_logIfError(vkMapMemory(state->context.device, state->renderer.pUniformBufferMemories[i], 0, bufferSize, 0,
                                    &state->renderer.pUniformBuffersMapped[i]),
                        "Failed to map memory for uniform buffer!")
    }
}

void uniformBuffersDestroy(State_t *state)
{
    for (size_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        vkDestroyBuffer(state->context.device, state->renderer.pUniformBuffers[i], state->context.pAllocator);
        vkUnmapMemory(state->context.device, state->renderer.pUniformBufferMemories[i]);
        vkFreeMemory(state->context.device, state->renderer.pUniformBufferMemories[i], state->context.pAllocator);
        state->renderer.pUniformBuffersMapped[i] = NULL;
    }

    free(state->renderer.pUniformBuffers);
    state->renderer.pUniformBuffers = NULL;
    free(state->renderer.pUniformBufferMemories);
    state->renderer.pUniformBufferMemories = NULL;
}

void updateUniformBuffer(State_t *state)
{
    // For animation we want the total time so that the shaders can rotate/etc objects and display them where they should be
    // instead of having stutter with frames.

    float rotateDegreesY = 45.0F;
    float rotateDegreesX = 45.0F;
    float rotateDegreesZ = 45.0F;

    Quaternion_t qYaw = cm_quatAngleAxis(cm_deg2radf(rotateDegreesY) * (float)state->time.frameTimeTotal, Y_AXIS);
    Quaternion_t qPitch = cm_quatAngleAxis(cm_deg2radf(rotateDegreesX) * (float)state->time.frameTimeTotal, X_AXIS);
    Quaternion_t qRoll = cm_quatAngleAxis(cm_deg2radf(rotateDegreesZ) * (float)state->time.frameTimeTotal, Z_AXIS);
    Quaternion_t qTemp = cm_quatMultiply(qYaw, qPitch);
    Quaternion_t qCombined = cm_quatMultiply(qTemp, qRoll);
    Mat4c_t model = cm_quat2mat(qCombined);

    UniformBufferObject_t ubo = {
        .model = model,
        .view = cm_lookAt((Vec3f_t){0.0F, 3.0F, -3.0F}, // camera position
                          VEC3_ZERO,                    // look at origin
                          UP),                          // up = +Y
        .projection = cm_perspective(cm_deg2radf(45.0F),
                                     state->window.swapchain.imageExtent.width / (float)state->window.swapchain.imageExtent.height,
                                     0.1F, 10.0F),
    };

    memcpy(state->renderer.pUniformBuffersMapped[state->renderer.currentFrame], &ubo, sizeof(ubo));
}

void descriptorPoolCreate(State_t *state)
{
    VkDescriptorPoolSize poolSizes[] = {
        {
            // ubo
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = state->config.maxFramesInFlight,
        },
        {
            // sampler
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = state->config.maxFramesInFlight,
        },
    };

    uint32_t numPools = sizeof(poolSizes) / sizeof(*poolSizes);

    VkDescriptorPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = numPools,
        .pPoolSizes = poolSizes,
        .maxSets = state->config.maxFramesInFlight,
    };

    logs_logIfError(vkCreateDescriptorPool(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.descriptorPool),
                    "Failed to create descriptor pool!")
}

void descriptorPoolDestroy(State_t *state)
{
    vkDestroyDescriptorPool(state->context.device, state->renderer.descriptorPool, state->context.pAllocator);
}

void descriptorSetsCreate(State_t *state)
{
    VkDescriptorSetLayout *layouts = malloc(sizeof(VkDescriptorSetLayout) * state->config.maxFramesInFlight);
    logs_logIfError(layouts == NULL,
                    "Failed to allocate memory for descriptor set layouts!") for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
    {
        layouts[i] = state->renderer.descriptorSetLayout;
    }

    VkDescriptorSetAllocateInfo allocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state->renderer.descriptorPool,
        .descriptorSetCount = state->config.maxFramesInFlight,
        .pSetLayouts = layouts,
    };

    state->renderer.pDescriptorSets = malloc(sizeof(VkDescriptorSet) * state->config.maxFramesInFlight);
    logs_logIfError(state->renderer.pDescriptorSets == NULL,
                    "Failed to allocate memory for descriptor sets!")

        logs_logIfError(vkAllocateDescriptorSets(state->context.device, &allocateInfo, state->renderer.pDescriptorSets),
                        "Failed to allocate descriptor sets!")

            free(layouts);

    // Populate descriptors

    for (uint32_t i = 0; i < state->config.maxFramesInFlight; i++)
    {
        VkDescriptorBufferInfo bufferInfo = {
            .buffer = state->renderer.pUniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject_t),
        };

        VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = state->renderer.atlasTextureImageView,
            .sampler = state->renderer.textureSampler,
        };

        VkWriteDescriptorSet descriptorWrites[] = {
            {
                // ubo
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = state->renderer.pDescriptorSets[i],
                // location in the vertex shader
                .dstBinding = 0,
                // the descriptors can be arrays but thats not implemented at this time so 0 is the first "index"
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &bufferInfo,
            },
            {
                // sampler
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = state->renderer.pDescriptorSets[i],
                // location in the vertex shader
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &imageInfo,
            },
        };

        // Dont copy so 0 copies and destination null
        vkUpdateDescriptorSets(state->context.device, sizeof(descriptorWrites) / sizeof(*descriptorWrites), descriptorWrites,
                               0, VK_NULL_HANDLE);
    }
}

void descriptorSetsDestroy(State_t *state)
{
    // The descriptor set itself is freed by Vulkan when the descriptor pool is freed
    vkDestroyDescriptorSetLayout(state->context.device, state->renderer.descriptorSetLayout, state->context.pAllocator);
}

void atlasTextureViewImageCreate(State_t *state)
{
    // Written this way to support looping in the future
    state->renderer.atlasTextureImageView = imageViewCreate(state, state->renderer.atlasTextureImage,
                                                            VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void atlasTextureImageViewDestroy(State_t *state)
{
    vkDestroyImageView(state->context.device, state->renderer.atlasTextureImageView, state->context.pAllocator);
}

void textureSamplerCreate(State_t *state)
{
    float af = (float)state->renderer.anisotropicFilteringOptions[state->renderer.anisotropicFilteringOptionsCount - 1];
    VkSamplerCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        // Filters specify how to interpolate texels that are magnified or minified
        // Mag = oversampling and Min = undersampling
        // See https://vulkan-tutorial.com/Texture_mapping/Image_view_and_sampler
        // Linear for larger textures (interpolation)
        // .magFilter = VK_FILTER_LINEAR,
        // .minFilter = VK_FILTER_LINEAR,
        // Nearest for small textures (pixel art)
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        // Addressing can be defined per-axis and for some reason its UVW instead of XYZ (texture space convention)
        // Repeat is the most commonly used (floors, etc.) for tiling
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = af,
        // Which color is returned when sampling beyond the image with clamp to border addressing mode. It is possible to
        // return black, white or transparent in either float or int formats
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        // Which coordinate system you want to use to address texels in an image. If this field is VK_TRUE, then you can simply
        // use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed
        // using the [0, 1) range on all axes. Real-world applications almost always use normalized coordinates, because then
        // it's possible to use textures of varying resolutions with the exact same coordinates
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        // Mipmapping not implemneted at this time
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .mipLodBias = 0.0f,
        .minLod = 0.0f,
        .maxLod = 0.0f,
    };

    logs_log(LOG_WARN, "Anisotropy is hard-coded to select the highest available (%.fx) at this time.", af);

    logs_logIfError(vkCreateSampler(state->context.device, &createInfo, state->context.pAllocator, &state->renderer.textureSampler),
                    "Failed to create texture sampler!")
}

void textureSamplerDestroy(State_t *state)
{
    vkDestroySampler(state->context.device, state->renderer.textureSampler, state->context.pAllocator);
}

bool formatHasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void depthResourcesCreate(State_t *state)
{
    VkFormat depthFormat = depth_formatGet(state);

    imageCreate(state, state->window.swapchain.imageExtent.width, state->window.swapchain.imageExtent.height,
                depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &state->renderer.depthImage, &state->renderer.depthImageMemory);

    state->renderer.depthImageView = imageViewCreate(state, state->renderer.depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    // Don't need to explicitly transition the layout of the image to a depth attachment because taken care of in the render pass
}

void depthResourcesDestroy(State_t *state)
{
    vkDestroyImageView(state->context.device, state->renderer.depthImageView, state->context.pAllocator);
    vkDestroyImage(state->context.device, state->renderer.depthImage, state->context.pAllocator);
    vkFreeMemory(state->context.device, state->renderer.depthImageMemory, state->context.pAllocator);
}

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
void rendererCreate(State_t *state)
{
    // Must exist before anything that references it
    rp_create(state);
    descriptorSetLayoutCreate(state);
    gp_create(state);

    // Needed for all staging/copies and one-time commands
    commandPoolCreate(state);

    // Atlas resources FIRST (image -> view -> sampler -> regions)
    // so m3d_load can remap UVs using pAtlasRegions
    // loads atlas.png, creates VkImage + memory
    atlasTextureImageCreate(state);
    // view for the atlas image
    atlasTextureViewImageCreate(state);
    // sampler used by descriptors
    textureSamplerCreate(state);
    // call the dynamic atlas region builder *after* you know width/height
    // fills renderer.pAtlasRegions[0..N)
    state->renderer.pAtlasRegions = atlasCreate(state->renderer.pAtlasRegions, state->renderer.atlasRegionCount,
                                                state->renderer.atlasWidthInTiles, state->renderer.atlasHeightInTiles);

    // Model upload that may use pAtlasRegions for UV remap
    // builds vertex/index buffers
    m3d_load(state);

    // Per-frame resources that descriptors will point at
    // UBOs (needed before descriptorSetsCreate)
    uniformBuffersCreate(state);

    // Depth & framebuffers must happen before recording
    // creates depth image + view
    depthResourcesCreate(state);
    // needs swapchain views + depth view + render pass
    framebuffersCreate(state);

    // Descriptor pool/sets AFTER UBO + atlas view + sampler exist
    descriptorPoolCreate(state);
    // writes UBO & combined image sampler
    descriptorSetsCreate(state);

    // Command buffers and sync last
    commandBufferAllocate(state);
    syncObjectsCreate(state);
}

void rendererDestroy(State_t *state)
{
    // The GPU could be working on stuff for the renderer in parallel, meaning the renderer could be
    // destroyed while the GPU is still working. We should wait for the GPU to finish its current tasks.
    logs_logIfError(vkQueueWaitIdle(state->context.graphicsQueue),
                    "Failed to wait for the Vulkan graphicsQueue to be idle.")
        // Stop GPU use first
        syncObjectsDestroy(state);

    // Descriptors before destroying underlying resources
    descriptorSetsDestroy(state);
    descriptorPoolDestroy(state);

    // Framebuffer graph
    framebuffersDestroy(state);
    depthResourcesDestroy(state);

    // Per-model buffers
    uniformBuffersDestroy(state);
    indexBufferDestroy(state);
    vertexBufferDestroy(state);
    m3d_destroy(state);

    // Atlas GPU resources
    textureSamplerDestroy(state);
    atlasTextureImageViewDestroy(state);
    atlasTextureImageDestroy(state);
    // frees pAtlasRegions (heap)
    // AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles
    atlasDestroy(state->renderer.pAtlasRegions);

    // Command pool after any single-time buffers etc. are destroyed
    commandPoolDestroy(state);

    // Pipeline objects last
    gp_destroy(state);
    rp_destroy(state);
}
