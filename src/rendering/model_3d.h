#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdalign.h>
#include "c_math/c_math.h"
#include "rendering/uvs.h"
#include "rendering/texture.h"
#include "rendering/voxel.h"
#include "rendering/buffers/buffers.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/types/uniformBufferObject_t.h"
#include "rendering/types/faceTexture_t.h"
#include "main.h"

/// @brief Loads a 3d model (.glb file)
/// @param state
void m3d_load(State_t *state)
{
    const char *path = MODEL_PATH "base_16x16_block.glb";

    cgltf_options options = {0};
    cgltf_data *data = NULL;

    // Parse the glTF model file
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    if (result != cgltf_result_success)
    {
        logs_log(LOG_ERROR, "Failed to parse glTF file: %s", path);
        return;
    }

    // Load external or embedded buffer data (needed for vertices, indices, etc.)
    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        logs_log(LOG_ERROR, "Failed to load buffers for: %s", path);
        cgltf_free(data);
        return;
    }

    // Validate structure (optional, but useful for debugging)
    result = cgltf_validate(data);
    if (result != cgltf_result_success)
    {
        logs_log(LOG_ERROR, "Invalid glTF data in: %s", path);
        cgltf_free(data);
        return;
    }

    logs_log(LOG_DEBUG, "Loaded model: %s", path);
    logs_log(LOG_DEBUG, "Meshes: %zu", data->meshes_count);
    logs_log(LOG_DEBUG, "Materials: %zu", data->materials_count);
    logs_log(LOG_DEBUG, "Nodes: %zu", data->nodes_count);

    // For now, only load the first mesh and its first primitive
    if (data->meshes_count == 0)
    {
        logs_log(LOG_ERROR, "Model has no meshes! %s", path);
        cgltf_free(data);
        return;
    }

    cgltf_mesh *mesh = &data->meshes[0];
    if (mesh->primitives_count == 0)
    {
        logs_log(LOG_ERROR, "Mesh has no primitives! %s", path);
        cgltf_free(data);
        return;
    }

    cgltf_primitive *prim = &mesh->primitives[0];

    logs_log(LOG_DEBUG, "Loading mesh: %s (%zu primitives)", mesh->name ? mesh->name : "(unnamed)", mesh->primitives_count);

    // Extract the accessors for position, texcoord, and optional color
    cgltf_accessor *posAccessor = NULL;
    cgltf_accessor *uvAccessor = NULL;
    for (size_t k = 0; k < prim->attributes_count; k++)
    {
        cgltf_attribute *attr = &prim->attributes[k];
        if (attr->type == cgltf_attribute_type_position)
            posAccessor = attr->data;
        else if (attr->type == cgltf_attribute_type_texcoord)
            uvAccessor = attr->data;
    }

    if (posAccessor == NULL)
    {
        logs_log(LOG_ERROR, "Model missing position attribute! %s", path);
        cgltf_free(data);
        return;
    }

    size_t vertexCount = posAccessor->count;
    ShaderVertex_t *vertices = malloc(sizeof(ShaderVertex_t) * vertexCount);
    logs_logIfError(vertices == NULL, "Failed to allocate vertex array for model: %s", path);

    float tmp[4];
    for (size_t v = 0; v < vertexCount; v++)
    {
        // Positions
        cgltf_accessor_read_float(posAccessor, v, tmp, 3);
        vertices[v].pos = (Vec3f_t){tmp[0], tmp[1], tmp[2]};

        // Texture coordinates
        if (uvAccessor)
        {
            cgltf_accessor_read_float(uvAccessor, v, tmp, 2);
            // Flip V coordinate to match Vulkan convention
            vertices[v].texCoord = (Vec2f_t){tmp[0], 1.0f - tmp[1]};
        }
        else
        {
            vertices[v].texCoord = (Vec2f_t){0.0f, 0.0f};
        }

        // Default white color for now (can be replaced if color attributes are added)
        vertices[v].color = WHITE;
    }

    // Load indices if they exist
    uint16_t *indices = NULL;
    size_t indexCount = 0;
    if (prim->indices)
    {
        cgltf_accessor *indexAccessor = prim->indices;
        indexCount = indexAccessor->count;
        indices = malloc(sizeof(uint16_t) * indexCount);
        logs_logIfError(indices == NULL, "Failed to allocate index array for model: %s", path);

        for (size_t i = 0; i < indexCount; i++)
        {
            indices[i] = (uint16_t)cgltf_accessor_read_index(indexAccessor, i);
        }
    }
    else
    {
        // Fallback: sequential indices
        indexCount = vertexCount;
        indices = malloc(sizeof(uint16_t) * indexCount);
        logs_logIfError(indices == NULL, "Failed to allocate fallback indices for model: %s", path);

        for (size_t i = 0; i < indexCount; i++)
        {
            indices[i] = (uint16_t)i;
        }
    }

    // Assign each face to an atlas region
    const int vertsPerFace = 4;
    const FaceTexture_t FACE_TEXTURES[6] = {
        [FACE_LEFT] = {MELON_SIDE, TEX_ROT_0},
        [FACE_RIGHT] = {MELON_SIDE, TEX_ROT_270},
        [FACE_TOP] = {MELON_TOP, TEX_ROT_0},
        [FACE_BOTTOM] = {MELON_TOP, TEX_ROT_90},
        [FACE_FRONT] = {MELON_SIDE, TEX_ROT_270},
        [FACE_BACK] = {MELON_SIDE, TEX_ROT_0},
    };

    for (int face = 0; face < 6; face++)
    {
        const FaceTexture_t tex = FACE_TEXTURES[face];
        const AtlasRegion_t *region = &state->renderer.pAtlasRegions[tex.atlasIndex];

        assignFaceUVs(vertices, face * vertsPerFace, region, tex.rotation);
    }

    // Upload to GPU
    vertexBufferCreateFromData(state, vertices, (uint32_t)vertexCount);
    indexBufferCreateFromData(state, indices, (uint32_t)indexCount);

    // Store index count in renderer for drawing
    state->renderer.modelIndexCount = (uint32_t)indexCount;

    // Cleanup CPU-side memory
    free(vertices);
    free(indices);
    cgltf_free(data);

    logs_log(LOG_DEBUG, "Model successfully uploaded to GPU: %s", path);
}

/// @brief Placeholder
/// @param state
void m3d_destroy(State_t *state)
{
    // Placeholder. Model destruction currently handled by buffer destruction
    state;
}
