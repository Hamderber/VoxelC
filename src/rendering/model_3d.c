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

    if (data->meshes_count == 0)
    {
        logs_log(LOG_ERROR, "Model has no meshes: %s", glbPath);
        cgltf_free(data);
        return NULL;
    }

    // Accumulate all primitives from all meshes into contiguous arrays
    ShaderVertex_t *vertices = NULL;
    uint32_t verticesCount = 0, verticesCapacity = 0;

    uint16_t *indices = NULL;
    uint32_t indexCount = 0, indexCapacity = 0;

    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f, minz = 1e9f, maxz = -1e9f;
    uint32_t primitiveTotal = 0;

    for (size_t m = 0; m < data->meshes_count; ++m)
    {
        cgltf_mesh *mesh = &data->meshes[m];

        // Find a node that references this mesh (Blockbench usually has one per mesh)
        cgltf_node *node = NULL;
        for (size_t n = 0; n < data->nodes_count; ++n)
        {
            if (data->nodes[n].mesh == mesh)
            {
                node = &data->nodes[n];
                break;
            }
        }

        float nodeMat44[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
        if (node)
        {
            // Local TRS → 4x4 (column-major array)
            cgltf_node_transform_local(node, nodeMat44);
        }
        Mat4c_t nodeXf = mat4_from_cgltf_colmajor(nodeMat44);

        for (size_t p = 0; p < mesh->primitives_count; ++p)
        {
            cgltf_primitive *prim = &mesh->primitives[p];
            ++primitiveTotal;

            // Locate accessors
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
                logs_log(LOG_WARN, "Primitive missing POSITION; skipping (mesh %zu prim %zu)", m, p);
                continue;
            }

            const size_t vcount = posAccessor->count;
            if (vcount == 0)
                continue;

            // Ensure vertex capacity
            if (verticesCount + (uint32_t)vcount > verticesCapacity)
            {
                uint32_t newCap = verticesCapacity ? (verticesCapacity * 2u) : (uint32_t)vcount;
                while (newCap < verticesCount + (uint32_t)vcount)
                    newCap *= 2u;
                ShaderVertex_t *newVerts = (ShaderVertex_t *)realloc(vertices, sizeof(ShaderVertex_t) * newCap);
                if (!newVerts)
                {
                    logs_log(LOG_ERROR, "Vertex realloc failed");
                    free(vertices);
                    free(indices);
                    cgltf_free(data);
                    return NULL;
                }
                vertices = newVerts;
                verticesCapacity = newCap;
            }

            // Read vertices (apply node transform to positions)
            float tmp[4];
            const uint32_t baseVertex = verticesCount;
            for (size_t v = 0; v < vcount; ++v)
            {
                cgltf_accessor_read_float(posAccessor, v, tmp, 3);
                Vec4f_t local = (Vec4f_t){tmp[0], tmp[1], tmp[2], 1.0f};
                Vec4f_t world = cmath_mat_transformByVec4(nodeXf, local);

                vertices[baseVertex + (uint32_t)v].pos = (Vec3f_t){world.x, world.y, world.z};

                // UVs (keep your current V flip because your texture load flips pixels via STB)
                if (uvAccessor)
                {
                    cgltf_accessor_read_float(uvAccessor, v, tmp, 2);
                    vertices[baseVertex + (uint32_t)v].texCoord = (Vec2f_t){tmp[0], 1.0f - tmp[1]};
                }
                else
                {
                    vertices[baseVertex + (uint32_t)v].texCoord = (Vec2f_t){0.0f, 0.0f};
                }

                // Color (optional)
                if (colAccessor)
                {
                    cgltf_accessor_read_float(colAccessor, v, tmp, 3);
                    vertices[baseVertex + (uint32_t)v].color = (Vec3f_t){tmp[0], tmp[1], tmp[2]};
                }
                else
                {
                    vertices[baseVertex + (uint32_t)v].color = COLOR_WHITE;
                }

                vertices[baseVertex + (uint32_t)v].atlasIndex = 0u;

                // Bounds (after transform)
                minx = fminf(minx, vertices[baseVertex + (uint32_t)v].pos.x);
                maxx = fmaxf(maxx, vertices[baseVertex + (uint32_t)v].pos.x);
                miny = fminf(miny, vertices[baseVertex + (uint32_t)v].pos.y);
                maxy = fmaxf(maxy, vertices[baseVertex + (uint32_t)v].pos.y);
                minz = fminf(minz, vertices[baseVertex + (uint32_t)v].pos.z);
                maxz = fmaxf(maxz, vertices[baseVertex + (uint32_t)v].pos.z);
            }
            verticesCount += (uint32_t)vcount;

            // Indices (append, offset by baseVertex)
            if (prim->indices)
            {
                cgltf_accessor *ia = prim->indices;
                const size_t icount = ia->count;

                if (indexCount + (uint32_t)icount > indexCapacity)
                {
                    uint32_t newCap = indexCapacity ? (indexCapacity * 2u) : (uint32_t)icount;
                    while (newCap < indexCount + (uint32_t)icount)
                        newCap *= 2u;
                    uint16_t *newIdx = (uint16_t *)realloc(indices, sizeof(uint16_t) * newCap);
                    if (!newIdx)
                    {
                        logs_log(LOG_ERROR, "Index realloc failed");
                        free(vertices);
                        free(indices);
                        cgltf_free(data);
                        return NULL;
                    }
                    indices = newIdx;
                    indexCapacity = newCap;
                }

                for (size_t i = 0; i < icount; ++i)
                {
                    uint32_t idx = (uint32_t)cgltf_accessor_read_index(ia, i);
                    uint32_t off = baseVertex + idx;
                    if (off > 0xFFFFu)
                    {
                        logs_log(LOG_ERROR, "Index overflow (>65535). Use 32-bit indices or split meshes.");
                        free(vertices);
                        free(indices);
                        cgltf_free(data);
                        return NULL;
                    }
                    indices[indexCount + (uint32_t)i] = (uint16_t)off;
                }
                indexCount += (uint32_t)icount;
            }
            else
            {
                const size_t icount = vcount;
                if (indexCount + (uint32_t)icount > indexCapacity)
                {
                    uint32_t newCap = indexCapacity ? (indexCapacity * 2u) : (uint32_t)icount;
                    while (newCap < indexCount + (uint32_t)icount)
                        newCap *= 2u;
                    uint16_t *newIdx = (uint16_t *)realloc(indices, sizeof(uint16_t) * newCap);
                    if (!newIdx)
                    {
                        logs_log(LOG_ERROR, "Index realloc failed (no-indices case)");
                        free(vertices);
                        free(indices);
                        cgltf_free(data);
                        return NULL;
                    }
                    indices = newIdx;
                    indexCapacity = newCap;
                }
                for (size_t i = 0; i < icount; ++i)
                {
                    uint32_t off = baseVertex + (uint32_t)i;
                    if (off > 0xFFFFu)
                    {
                        logs_log(LOG_ERROR, "Index overflow (>65535) in no-indices case.");
                        free(vertices);
                        free(indices);
                        cgltf_free(data);
                        return NULL;
                    }
                    indices[indexCount + (uint32_t)i] = (uint16_t)off;
                }
                indexCount += (uint32_t)icount;
            }
        }
    }

    logs_log(LOG_DEBUG, "Loaded %zu meshes, %u primitives. Total: %u verts, %u indices.",
             (size_t)data->meshes_count, primitiveTotal, verticesCount, indexCount);
    logs_log(LOG_DEBUG, "Model bounds (world-baked): X[%f,%f] Y[%f,%f] Z[%f,%f]",
             minx, maxx, miny, maxy, minz, maxz);

    if (verticesCount == 0 || indexCount == 0)
    {
        logs_log(LOG_ERROR, "No drawable geometry in: %s", glbPath);
        free(vertices);
        free(indices);
        cgltf_free(data);
        return NULL;
    }

    // Create the GPU buffers for the combined model
    RenderModel_t *model = (RenderModel_t *)calloc(1, sizeof(RenderModel_t));
    if (!model)
    {
        free(vertices);
        free(indices);
        cgltf_free(data);
        return NULL;
    }

    // (NOTE: your helpers populate renderer's shared buffers)
    vertexBufferCreateFromData(state, vertices, verticesCount);
    indexBufferCreateFromData(state, indices, indexCount);

    model->vertexBuffer = state->renderer.vertexBuffer;
    model->vertexMemory = state->renderer.vertexBufferMemory;
    model->indexBuffer = state->renderer.indexBuffer;
    model->indexMemory = state->renderer.indexBufferMemory;
    model->indexCount = indexCount;

    // Texture
    if (!texture2DCreateFromFile(state, texturePath, &model->textureImage, &model->textureMemory,
                                 &model->textureView, &model->textureSampler))
    {
        logs_log(LOG_WARN, "Failed to load texture '%s'. Using renderer default.", texturePath);
    }

    // Descriptor sets (model-local)
    const uint32_t frames = state->config.maxFramesInFlight;
    model->pDescriptorSets = (VkDescriptorSet *)calloc(frames, sizeof(VkDescriptorSet));
    if (!modelDescriptorSetsCreate(state, model))
    {
        logs_log(LOG_ERROR, "Failed to create model descriptor sets");
        free(vertices);
        free(indices);
        cgltf_free(data);
        free(model->pDescriptorSets);
        free(model);
        return NULL;
    }

    model->modelMatrix = MAT4_IDENTITY; // per-instance transform (additional to baked node TRS)

    // Cleanup CPU arrays + glTF
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
