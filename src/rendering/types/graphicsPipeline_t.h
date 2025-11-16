#pragma once

typedef enum GraphicsPipeline_e
{
    GRAPHICS_PIPELINE_MODEL_FILL,
    GRAPHICS_PIPELINE_VOXEL_FILL,
    GRAPHICS_PIPELINE_WIREFRAME,
    GRAPHICS_PIPELINE_COUNT,
} GraphicsPipeline_e;

typedef enum GraphicsTarget_e
{
    GRAPHICS_TARGET_MODEL,
    GRAPHICS_TARGET_VOXEL,
    GRAPHICS_TARGET_COUNT,
} GraphicsTarget_e;

/// @brief Converts the GraphicsPipeline_e to string for debugging purposes
static const char *graphicsPipeline_ToString(const GraphicsPipeline_e GP)
{
    switch (GP)
    {
    case GRAPHICS_PIPELINE_MODEL_FILL:
        return "GRAPHICS_PIPELINE_MODEL_FILL";
    case GRAPHICS_PIPELINE_VOXEL_FILL:
        return "GRAPHICS_PIPELINE_VOXEL_FILL";
    case GRAPHICS_PIPELINE_WIREFRAME:
        return "GRAPHICS_PIPELINE_WIREFRAME";
    default:
        return "GRAPHICS_PIPELINE_UNKNOWN";
    }
}

/// @brief Converts the GraphicsTarget_e to string for debugging purposes
static const char *graphicsTarget_ToString(const GraphicsTarget_e GT)
{
    switch (GT)
    {
    case GRAPHICS_TARGET_MODEL:
        return "GRAPHICS_TARGET_MODEL";
    case GRAPHICS_TARGET_VOXEL:
        return "GRAPHICS_TARGET_VOXEL";
    default:
        return "GRAPHICS_TARGET_UNKNOWN";
    }
}