#pragma once

#include <vulkan/vulkan.h>
#include "..\\res\\shaders\\shader_model_fill.frag.h"
#include "..\\res\\shaders\\shader_voxel_fill.frag.h"
#include "..\\res\\shaders\\shader_wireframe.frag.h"
#include "..\\res\\shaders\\shader.vert.h"
#include "..\\res\\shaders\\shader_voxel.vert.h"
#include "rendering/types/shaderVertexModel_t.h"
#include "rendering/types/shaderVertexVoxel_t.h"
#include "rendering/graphics_pipeline.h"

typedef enum
{
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_FRAGMENT = 1,
    SHADER_STAGE_COUNT,
} ShaderStage_t;

/// @brief Converts the ShaderStage_t to string for debugging purposes
static const char *shaderStage_ToString(const ShaderStage_t S)
{
    switch (S)
    {
    case SHADER_STAGE_VERTEX:
        return "SHADER_STAGE_VERTEX";
    case SHADER_STAGE_FRAGMENT:
        return "SHADER_STAGE_FRAGMENT";
    default:
        return "SHADER_STAGE_UNKNOWN";
    }
}

typedef struct
{
    const uint32_t *pCODE;
    size_t codeSize;
} ShaderBlob_t;

static const uint32_t NUM_SHADER_VERTEX_BINDING_DESCRIPTIONS_MODEL = 1;
// A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number
// of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
static inline const VkVertexInputBindingDescription *shaderVertexGetBindingDescriptionModel(void)
{
    static const VkVertexInputBindingDescription descriptions[1] = {
        {
            .binding = 0,
            .stride = sizeof(ShaderVertexModel_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    return descriptions;
}

static const uint32_t NUM_SHADER_VERTEX_BINDING_DESCRIPTIONS_VOXEL = 1;
static inline const VkVertexInputBindingDescription *shaderVertexGetBindingDescriptionVoxel(void)
{
    static const VkVertexInputBindingDescription descriptions[1] = {
        {
            .binding = 0,
            .stride = sizeof(ShaderVertexVoxel_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        },
    };

    return descriptions;
}

static const uint32_t NUM_SHADER_VERTEX_ATTRIBUTES_MODEL = 3;
// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data originating
// from a binding description. We have two attributes, position and color, so we need two attribute description structs.
static inline const VkVertexInputAttributeDescription *shaderVertexGetInputAttributeDescriptionsModel(void)
{
    static const VkVertexInputAttributeDescription descriptions[3] = {
        // Position
        {
            .binding = 0,
            // The location for the position in the vertex shader
            .location = 0,
            // Type of data (format is data type so color is same format as pos)
            // a float would be VK_FORMAT_R32_SFLOAT but a vec4 (ex quaternion or rgba) would be VK_FORMAT_R32G32B32A32_SFLOAT
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            // Specifies the number of bytes since the start of the per-vertex data to read from.
            // Position is first so 0
            .offset = offsetof(ShaderVertexModel_t, pos),
        },
        // Color
        {
            .binding = 0,
            // The location for the color in the vertex shader
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            // Position is first so sizeof(pos) type to get offset
            .offset = offsetof(ShaderVertexModel_t, color),
        },
        // TexCoord
        {
            .binding = 0,
            .location = 2,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(ShaderVertexModel_t, texCoord),
        },
    };

    return descriptions;
}

static const uint32_t NUM_SHADER_VERTEX_ATTRIBUTES_VOXEL = 4;
static inline const VkVertexInputAttributeDescription *shaderVertexGetInputAttributeDescriptionsVoxel(void)
{
    static const VkVertexInputAttributeDescription descriptions[4] = {
        // Position
        {
            .binding = 0,
            .location = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(ShaderVertexVoxel_t, pos),
        },
        // Color
        {
            .binding = 0,
            .location = 1,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = offsetof(ShaderVertexVoxel_t, color),
        },
        // TexCoord
        {
            .binding = 0,
            .location = 2,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(ShaderVertexVoxel_t, texCoord),
        },
        // // Face normal
        {
            .binding = 0,
            .location = 3,
            .format = VK_FORMAT_R32_SINT,
            .offset = offsetof(ShaderVertexVoxel_t, faceID),
        },
    };

    return descriptions;
}