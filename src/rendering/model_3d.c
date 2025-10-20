#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include <vulkan/vulkan.h>
#include "cmath/cmath.h"
#include "rendering/uvs.h"
#include "rendering/texture.h"
#include "rendering/voxel.h"
#include "rendering/buffers/buffers.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/types/uniformBufferObject_t.h"
#include "rendering/types/faceTexture_t.h"
#include "main.h"
#include "cgltf.h"
#include "texture.h"

static VkDescriptorSet allocateDescriptorSet(VkDevice device,
                                             VkDescriptorPool pool,
                                             VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    VkDescriptorSet set = VK_NULL_HANDLE;
    VkResult r = vkAllocateDescriptorSets(device, &ai, &set);
    if (r != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return set;
}

static bool modelDescriptorPoolCreate(State_t *state, RenderModel_t *model)
{
    const uint32_t frames = state->config.maxFramesInFlight;

    // We need 1 UBO + 1 sampler per set → multiply by frames
    VkDescriptorPoolSize sizes[] = {
        {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = frames},
        {.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = frames},
    };

    VkDescriptorPoolCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = frames,
        .poolSizeCount = (uint32_t)(sizeof sizes / sizeof *sizes),
        .pPoolSizes = sizes,
    };

    return vkCreateDescriptorPool(state->context.device, &ci, state->context.pAllocator,
                                  &model->descriptorPool) == VK_SUCCESS;
}

bool modelDescriptorSetsCreate(State_t *state, RenderModel_t *model)
{
    if (!modelDescriptorPoolCreate(state, model))
    {
        logs_log(LOG_ERROR, "Model descriptor pool create failed");
        return false;
    }

    const uint32_t frames = state->config.maxFramesInFlight;
    model->pDescriptorSets = (VkDescriptorSet *)calloc(frames, sizeof *model->pDescriptorSets);
    if (!model->pDescriptorSets)
        return false;

    for (uint32_t f = 0; f < frames; ++f)
    {
        VkDescriptorSet set = allocateDescriptorSet(state->context.device,
                                                    model->descriptorPool,
                                                    state->renderer.descriptorSetLayout);
        if (set == VK_NULL_HANDLE)
        {
            logs_log(LOG_ERROR, "Model descriptor set alloc failed (frame %u)", f);
            return false;
        }
        model->pDescriptorSets[f] = set;

        // Binding 0 = UBO (shared per-frame UBO you already update)
        VkDescriptorBufferInfo uboInfo = {
            .buffer = state->renderer.pUniformBuffers[f],
            .offset = 0,
            .range = sizeof(UniformBufferObject_t),
        };
        VkWriteDescriptorSet writeUBO = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .pBufferInfo = &uboInfo,
        };

        // Binding 1 = this model’s texture
        VkDescriptorImageInfo imgInfo = {
            .sampler = model->textureSampler,
            .imageView = model->textureView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        VkWriteDescriptorSet writeImg = {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = set,
            .dstBinding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .pImageInfo = &imgInfo,
        };

        VkWriteDescriptorSet writes[2] = {writeUBO, writeImg};
        vkUpdateDescriptorSets(state->context.device, 2, writes, 0, NULL);
    }
    return true;
}

void modelDescriptorSetsDestroy(State_t *state, RenderModel_t *model)
{
    if (model->pDescriptorSets)
    {
        free(model->pDescriptorSets);
        model->pDescriptorSets = NULL;
    }
    if (model->descriptorPool)
    {
        vkDestroyDescriptorPool(state->context.device, model->descriptorPool, state->context.pAllocator);
        model->descriptorPool = VK_NULL_HANDLE;
    }
}

RenderModel_t *m3d_load(State_t *state, const char *glbPath, const char *texturePath)
{
    cgltf_options options = {0};
    cgltf_data *data = NULL;

    // Parse + load + validate
    if (cgltf_parse_file(&options, glbPath, &data) != cgltf_result_success)
    {
        logs_log(LOG_ERROR, "cgltf_parse_file failed: %s", glbPath);
        return NULL;
    }
    if (cgltf_load_buffers(&options, data, glbPath) != cgltf_result_success)
    {
        logs_log(LOG_ERROR, "cgltf_load_buffers failed: %s", glbPath);
        cgltf_free(data);
        return NULL;
    }
    if (cgltf_validate(data) != cgltf_result_success)
    {
        logs_log(LOG_ERROR, "cgltf_validate failed: %s", glbPath);
        cgltf_free(data);
        return NULL;
    }

    if (data->meshes_count == 0 || data->meshes[0].primitives_count == 0)
    {
        logs_log(LOG_ERROR, "Model missing mesh/primitive: %s", glbPath);
        cgltf_free(data);
        return NULL;
    }

    cgltf_primitive *prim = &data->meshes[0].primitives[0];

    // Accessors
    cgltf_accessor *posAccessor = NULL, *uvAccessor = NULL, *colAccessor = NULL;
    for (size_t k = 0; k < prim->attributes_count; ++k)
    {
        cgltf_attribute *attr = &prim->attributes[k];
        if (attr->type == cgltf_attribute_type_position)
            posAccessor = attr->data;
        else if (attr->type == cgltf_attribute_type_texcoord)
            uvAccessor = attr->data;
        else if (attr->type == cgltf_attribute_type_color)
            colAccessor = attr->data;
    }
    if (!posAccessor)
    {
        logs_log(LOG_ERROR, "Position attribute missing: %s", glbPath);
        cgltf_free(data);
        return NULL;
    }

    // Vertices
    const size_t vertexCount = posAccessor->count;
    ShaderVertex_t *vertices = (ShaderVertex_t *)malloc(sizeof(ShaderVertex_t) * vertexCount);
    if (!vertices)
    {
        cgltf_free(data);
        return NULL;
    }

    float tmp[4];
    for (size_t v = 0; v < vertexCount; ++v)
    {
        cgltf_accessor_read_float(posAccessor, v, tmp, 3);
        vertices[v].pos = (Vec3f_t){tmp[0], tmp[1], tmp[2]};

        if (uvAccessor)
        {
            cgltf_accessor_read_float(uvAccessor, v, tmp, 2);
            vertices[v].texCoord = (Vec2f_t){tmp[0], 1.0f - tmp[1]}; // flip V
        }
        else
        {
            vertices[v].texCoord = (Vec2f_t){0.0f, 0.0f};
        }

        if (colAccessor)
        {
            cgltf_accessor_read_float(colAccessor, v, tmp, 3);
            vertices[v].color = (Vec3f_t){tmp[0], tmp[1], tmp[2]};
        }
        else
        {
            vertices[v].color = COLOR_WHITE;
        }

        // Models do NOT use atlas indexing; keep 0 so the attribute is well-defined
        vertices[v].atlasIndex = 0u;
    }

    // Indices (ensure they fit UINT16 if your pipeline uses VK_INDEX_TYPE_UINT16)
    uint16_t *indices = NULL;
    size_t indexCount = 0;
    if (prim->indices)
    {
        cgltf_accessor *ia = prim->indices;
        indexCount = ia->count;

        // Validate index range here (65535)

        indices = (uint16_t *)malloc(sizeof(uint16_t) * indexCount);
        if (!indices)
        {
            free(vertices);
            cgltf_free(data);
            return NULL;
        }

        for (size_t i = 0; i < indexCount; ++i)
        {
            indices[i] = (uint16_t)cgltf_accessor_read_index(ia, i);
        }
    }
    else
    {
        indexCount = vertexCount;
        indices = (uint16_t *)malloc(sizeof(uint16_t) * indexCount);
        if (!indices)
        {
            free(vertices);
            cgltf_free(data);
            return NULL;
        }
        for (size_t i = 0; i < indexCount; ++i)
            indices[i] = (uint16_t)i;
    }

    // Create the GPU buffers for this model
    RenderModel_t *model = (RenderModel_t *)calloc(1, sizeof(RenderModel_t));
    if (!model)
    {
        free(vertices);
        free(indices);
        cgltf_free(data);
        return NULL;
    }

    vertexBufferCreateFromData(state, vertices, (uint32_t)vertexCount);
    indexBufferCreateFromData(state, indices, (uint32_t)indexCount);

    // Take ownership of the buffers created by the renderer helpers
    model->vertexBuffer = state->renderer.vertexBuffer;
    model->vertexMemory = state->renderer.vertexBufferMemory;
    model->indexBuffer = state->renderer.indexBuffer;
    model->indexMemory = state->renderer.indexBufferMemory;
    model->indexCount = (uint32_t)indexCount;

    // Load the model's texture from /res/textures (no atlas)
    if (!texture2DCreateFromFile(state, texturePath, &model->textureImage, &model->textureMemory,
                                 &model->textureView, &model->textureSampler))
    {
        logs_log(LOG_WARN, "Failed to load texture '%s'. Using renderer's default texture.", texturePath);
        // You could fall back to a default white/gray texture here
    }

    // Allocate per-frame descriptor sets for this model that point to:
    //   binding 0 -> per-frame UBO (view/proj)
    //   binding 1 -> model->textureView+model->textureSampler
    const uint32_t frames = state->config.maxFramesInFlight;
    model->pDescriptorSets = (VkDescriptorSet *)calloc(frames, sizeof(VkDescriptorSet));
    if (!modelDescriptorSetsCreate(state, model))
    {
        logs_log(LOG_ERROR, "Failed to create model descriptor sets");
        // clean up model texture & buffers here
        return NULL;
    }

    model->modelMatrix = MAT4_IDENTITY;

    // Cleanup CPU arrays
    free(vertices);
    free(indices);
    cgltf_free(data);

    return model;
}

/// @brief Placeholder
/// @param state
void m3d_destroy(State_t *state)
{
    // Placeholder. Model destruction currently handled by buffer destruction
    state;
}
