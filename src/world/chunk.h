#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "rendering/types/renderChunk_t.h"
#include "world/voxel/block_t.h"

// This shall NEVER change
static const uint32_t CHUNK_AXIS_LENGTH = 16;
// 16x16x16
static const uint32_t CHUNK_BLOCK_CAPACITY = 4096;

typedef struct
{
    int x, y, z;
} ChunkPos_t;

typedef struct
{
    RenderChunk_t *pRenderChunk;
    BlockVoxel_t *pBlockVoxels;
    ChunkPos_t coordinate;
} Chunk_t;
