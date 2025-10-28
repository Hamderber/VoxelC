#pragma region Includes
#include <vulkan/vulkan.h>
#include "core/logs.h"
#include <stdlib.h>
#include <string.h>
#include "core/types/state_t.h"
#include "rendering/types/uniformBufferObject_t.h"
#include "core/crash_handler.h"
#pragma endregion
#pragma region Layout Binding
void descriptorSet_layout_create(State_t *pState)
{
    do
    {
        const VkDescriptorSetLayoutBinding UBO_LAYOUT_BINDING = {
            // Location for the ubo in the shader
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            // Specifices that these descriptors are for the vertex shader to reference
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            // Image sampling not implemented at this time
            .pImmutableSamplers = VK_NULL_HANDLE,
        };

        const VkDescriptorSetLayoutBinding SAMPLER_BINDING = {
            // Location in the shader
            .binding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            // No immutable samplers at this time
            .pImmutableSamplers = VK_NULL_HANDLE,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        };

        const VkDescriptorSetLayoutBinding pBINDINGS[] = {
            UBO_LAYOUT_BINDING,
            SAMPLER_BINDING,
        };

        const VkDescriptorSetLayoutCreateInfo CREATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = sizeof(pBINDINGS) / sizeof(*pBINDINGS),
            .pBindings = pBINDINGS,
        };

        if (vkCreateDescriptorSetLayout(pState->context.device, &CREATE_INFO, pState->context.pAllocator,
                                        &pState->renderer.descriptorSetLayout) != VK_SUCCESS)
        {
            logs_log(LOG_ERROR, "Failed to create the Vulkan descriptor set layout!");
            break;
        }

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a descriptor set to describe the Vulkan image presentation.");
}
#pragma endregion
#pragma region Sets Create/Dest
/// @brief Allocates the state's descriptor sets with their descriptor set layouts
static void sets_allocate(State_t *pState)
{
    VkDescriptorSetLayout *pLayouts = NULL;
    int crashLine = 0;
    do
    {
        pLayouts = malloc(sizeof(VkDescriptorSetLayout) * pState->config.maxFramesInFlight);
        if (!pLayouts)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for Vulkan descriptor set layouts!");
            break;
        }

        for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
            pLayouts[i] = pState->renderer.descriptorSetLayout;

        const VkDescriptorSetAllocateInfo ALLOCATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pState->renderer.descriptorPool,
            .descriptorSetCount = pState->config.maxFramesInFlight,
            .pSetLayouts = pLayouts,
        };

        pState->renderer.pDescriptorSets = malloc(sizeof(VkDescriptorSet) * pState->config.maxFramesInFlight);

        if (!pState->renderer.pDescriptorSets ||
            vkAllocateDescriptorSets(pState->context.device, &ALLOCATE_INFO, pState->renderer.pDescriptorSets) != VK_SUCCESS)
        {
            crashLine = __LINE__;
            logs_log(LOG_ERROR, "Failed to allocate memory for Vulkan descriptor sets!");
            break;
        }

    } while (0);

    free(pLayouts);

    if (crashLine != 0)
    {
        free(pState->renderer.pDescriptorSets);
        crashHandler_crash_graceful(
            CRASH_LOCATION_LINE(crashLine),
            "The program cannot continue without the Vulkan descriptor sets being allocated for use in image presentation.");
    }
}

/// @brief Populates/creates the descriptor sets
static void sets_populate(State_t *pState)
{
    const VkDescriptorImageInfo IMAGE_INFO = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = pState->renderer.atlasTextureImageView,
        .sampler = pState->renderer.textureSampler,
    };

    for (uint32_t i = 0; i < pState->config.maxFramesInFlight; i++)
    {
        const VkDescriptorBufferInfo BUFFER_INFO = {
            .buffer = pState->renderer.pUniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject_t),
        };

        const VkWriteDescriptorSet DESCRIPTOR_WRITES[] = {
            // ubo
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = pState->renderer.pDescriptorSets[i],
                // location in the vertex shader
                .dstBinding = 0,
                // the descriptors can be arrays but thats not implemented at this time so 0 is the first "index"
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo = &BUFFER_INFO,
            },
            // sampler
            {
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = pState->renderer.pDescriptorSets[i],
                // location in the vertex shader
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo = &IMAGE_INFO,
            },
        };

        uint32_t numCopies = 0;
        VkCopyDescriptorSet *pDestination = VK_NULL_HANDLE;
        vkUpdateDescriptorSets(pState->context.device, sizeof(DESCRIPTOR_WRITES) / sizeof(*DESCRIPTOR_WRITES), DESCRIPTOR_WRITES,
                               numCopies, pDestination);
    }
}

void sets_create(State_t *pState)
{
    sets_allocate(pState);
    sets_populate(pState);
}

void sets_destroy(State_t *pState)
{
    // The descriptor set itself is freed by Vulkan when the descriptor pool is freed
    vkDestroyDescriptorSetLayout(pState->context.device, pState->renderer.descriptorSetLayout, pState->context.pAllocator);
}
#pragma endregion
#pragma region Pool Create/Dest
void descriptorPool_create(State_t *pState)
{
    do
    {
        const VkDescriptorPoolSize pPOOL_SIZES[] = {
            {
                // ubo
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = pState->config.maxFramesInFlight,
            },
            {
                // sampler
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = pState->config.maxFramesInFlight,
            },
        };

        uint32_t numPools = sizeof(pPOOL_SIZES) / sizeof(*pPOOL_SIZES);

        const VkDescriptorPoolCreateInfo CREATE_INFO = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .poolSizeCount = numPools,
            .pPoolSizes = pPOOL_SIZES,
            .maxSets = pState->config.maxFramesInFlight,
        };

        if (vkCreateDescriptorPool(pState->context.device, &CREATE_INFO, pState->context.pAllocator,
                                   &pState->renderer.descriptorPool) != VK_SUCCESS)
        {
            logs_log(LOG_ERROR, "Failed to create the Vulkan descriptor pool!");
            break;
        }

        sets_create(pState);

        return;
    } while (0);

    crashHandler_crash_graceful(CRASH_LOCATION, "The program cannot continue without a Vulkan descriptor pool for image presentation.");
}

void descriptorPool_destroy(State_t *pState)
{
    sets_destroy(pState);
    vkDestroyDescriptorPool(pState->context.device, pState->renderer.descriptorPool, pState->context.pAllocator);
}
#pragma endregion