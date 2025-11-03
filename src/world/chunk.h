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
    ChunkPos_t chunkPos;
} Chunk_t;

static inline bool chunk_chunkPos_equals(ChunkPos_t left, ChunkPos_t right)
{
    return left.x == right.x && left.y == right.y && left.z == right.z;
}

static inline uint32_t xyz_to_chunkBlockIndex(int x, int y, int z)
{
    return x * CHUNK_AXIS_LENGTH * CHUNK_AXIS_LENGTH + y * CHUNK_AXIS_LENGTH + z;
}

static inline Vec3i_t chunkPos_to_worldOrigin(ChunkPos_t cp)
{
    return (Vec3i_t){
        cp.x * CHUNK_AXIS_LENGTH,
        cp.y * CHUNK_AXIS_LENGTH,
        cp.z * CHUNK_AXIS_LENGTH};
}

static inline Vec3i_t blockXYZ_to_worldPos(ChunkPos_t cp, short lx, short ly, short lz)
{
    Vec3i_t origin = chunkPos_to_worldOrigin(cp);
    return (Vec3i_t){origin.x + lx, origin.y + ly, origin.z + lz};
}

/// @brief Local block -> float world position at the CENTER of the voxel (best for noise)
static inline Vec3f_t blockXYZ_to_worldSamplePos(ChunkPos_t cp, short lx, short ly, short lz)
{
    Vec3i_t origin = chunkPos_to_worldOrigin(cp);
    return (Vec3f_t){
        (float)origin.x + (float)lx + 0.5F,
        (float)origin.y + (float)ly + 0.5F,
        (float)origin.z + (float)lz + 0.5F};
}
