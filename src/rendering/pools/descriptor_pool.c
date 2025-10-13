#include <vulkan/vulkan.h>
#include "core/logs.h"
#include <stdlib.h>
#include <string.h>
#include "core/types/state_t.h"
#include "rendering/types/uniformBufferObject_t.h"

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
                    "Failed to create descriptor set layout!");
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
                    "Failed to create descriptor pool!");
}

void descriptorPoolDestroy(State_t *state)
{
    vkDestroyDescriptorPool(state->context.device, state->renderer.descriptorPool, state->context.pAllocator);
}

void descriptorSetsCreate(State_t *state)
{
    VkDescriptorSetLayout *layouts = malloc(sizeof(VkDescriptorSetLayout) * state->config.maxFramesInFlight);
    logs_logIfError(layouts == NULL,
                    "Failed to allocate memory for descriptor set layouts!");
    for (uint32_t i = 0U; i < state->config.maxFramesInFlight; i++)
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
                    "Failed to allocate memory for descriptor sets!");

    logs_logIfError(vkAllocateDescriptorSets(state->context.device, &allocateInfo, state->renderer.pDescriptorSets),
                    "Failed to allocate descriptor sets!");

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
