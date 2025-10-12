#pragma once

#include <vulkan/vulkan.h>
#include "..\\res\\shaders\\shader.frag.h"
#include "..\\res\\shaders\\shader.vert.h"

typedef enum
{
    SHADER_STAGE_VERTEX = 0,
    SHADER_STAGE_FRAGMENT = 1,
} ShaderStage_t;

typedef struct
{
    Vec3f_t pos;
    Vec3f_t color;
    Vec2f_t texCoord;
    uint32_t atlasIndex;
} ShaderVertex_t;

static const uint32_t NUM_SHADER_VERTEX_BINDING_DESCRIPTIONS = 1U;
// A vertex binding describes at which rate to load data from memory throughout the vertices. It specifies the number
// of bytes between data entries and whether to move to the next data entry after each vertex or after each instance.
static inline const VkVertexInputBindingDescription *shaderVertexGetBindingDescription(void)
{
    static const VkVertexInputBindingDescription descriptions[1] = {
        {
            .binding = 0,
            .stride = sizeof(ShaderVertex_t),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,

        },
    };

    return descriptions;
}

static const uint32_t NUM_SHADER_VERTEX_ATTRIBUTES = 3U;
// An attribute description struct describes how to extract a vertex attribute from a chunk of vertex data originating
// from a binding description. We have two attributes, position and color, so we need two attribute description structs.
static inline const VkVertexInputAttributeDescription *shaderVertexGetInputAttributeDescriptions(void)
{
    static const VkVertexInputAttributeDescription descriptions[3] = {
        // Position
        {
            .binding = 0U,
            // The location for the position in the vertex shader
            .location = 0U,
            // Type of data (format is data type so color is same format as pos)
            // a float would be VK_FORMAT_R32_SFLOAT but a vec4 (ex quaternion or rgba) would be VK_FORMAT_R32G32B32A32_SFLOAT
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            // Specifies the number of bytes since the start of the per-vertex data to read from.
            // Position is first so 0
            .offset = offsetof(ShaderVertex_t, pos),
        },
        // Color
        {
            .binding = 0U,
            // The location for the color in the vertex shader
            .location = 1U,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            // Position is first so sizeof(pos) type to get offset
            .offset = offsetof(ShaderVertex_t, color),
        },
        {
            .binding = 0U,
            .location = 2U,
            .format = VK_FORMAT_R32G32_SFLOAT,
            .offset = offsetof(ShaderVertex_t, texCoord),
        },
    };

    return descriptions;
}