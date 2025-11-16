#pragma region Includes
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "cmath/cmath.h"
#include "rendering/types/renderChunk_t.h"
#include "world/voxel/block_t.h"
#include "collection/linkedList_t.h"
#include "chunkSolidityGrid.h"
#pragma endregion
#pragma region Chunk Pos
/// @brief Converts chunk position to world position
static inline Vec3i_t chunkPos_to_worldOrigin(const Vec3i_t CHUNK_POS)
{
    return (Vec3i_t){
        CHUNK_POS.x * CHUNK_AXIS_LENGTH,
        CHUNK_POS.y * CHUNK_AXIS_LENGTH,
        CHUNK_POS.z * CHUNK_AXIS_LENGTH};
}

/// @brief Floor-divide to work for negative chunk pos
static inline int floor_div_chunk(int x)
{
    // Otherwise -1/16 for example = 0 which would result in wrong chunkPos for negative chunks
    return (x >= 0) ? (x / CHUNK_AXIS_LENGTH) : ((x - (CHUNK_AXIS_LENGTH - 1)) / CHUNK_AXIS_LENGTH);
}

/// @brief Converts world position to chunk position
static inline Vec3i_t worldPosi_to_chunkPos(const Vec3i_t WORLD_POS)
{
    return (Vec3i_t){
        .x = floor_div_chunk(WORLD_POS.x),
        .y = floor_div_chunk(WORLD_POS.y),
        .z = floor_div_chunk(WORLD_POS.z),
    };
}

/// @brief Converts a world position axis coord to chunk position (index)
static inline int chunk_index_from_worldf(float x)
{
    return (int)floorf(x / (float)CHUNK_AXIS_LENGTH);
}

/// @brief Converts world position (float) to chunk position
static inline Vec3i_t worldPosf_to_chunkPos(Vec3f_t wp)
{
    return (Vec3i_t){
        .x = chunk_index_from_worldf(wp.x),
        .y = chunk_index_from_worldf(wp.y),
        .z = chunk_index_from_worldf(wp.z),
    };
}
#pragma endregion
#pragma region Block Pos
/// @brief Converts a block's position packed12 to world pos
static inline Vec3i_t blockPosPacked_get_worldPos(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3i_t ORIGIN = chunkPos_to_worldOrigin(CHUNK_POS);
    return (Vec3i_t){
        ORIGIN.x + (int)cmath_chunkBlockPosPackedGetLocal_x(BLOCK_POS_PACKED12),
        ORIGIN.y + (int)cmath_chunkBlockPosPackedGetLocal_y(BLOCK_POS_PACKED12),
        ORIGIN.z + (int)cmath_chunkBlockPosPackedGetLocal_z(BLOCK_POS_PACKED12)};
}

/// @brief Converts a block's position packed12 to block index in the chunk
static inline uint16_t blockPosPacked_to_chunkBlockIndex(const uint16_t BLOCK_POS_PACKED)
{
    return cmath_chunkBlockPosPackedGetLocal_x(BLOCK_POS_PACKED) * CHUNK_AXIS_LENGTH * CHUNK_AXIS_LENGTH +
           cmath_chunkBlockPosPackedGetLocal_y(BLOCK_POS_PACKED) * CHUNK_AXIS_LENGTH +
           cmath_chunkBlockPosPackedGetLocal_z(BLOCK_POS_PACKED);
}

/// @brief Sample pos is in the center of the voxel (offset by 0.5F)
static inline Vec3f_t blockPacked_to_worldSamplePos(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3i_t ORIGIN = chunkPos_to_worldOrigin(CHUNK_POS);
    return (Vec3f_t){
        (float)ORIGIN.x + (float)cmath_chunkBlockPosPackedGetLocal_x(BLOCK_POS_PACKED12) + 0.5F,
        (float)ORIGIN.y + (float)cmath_chunkBlockPosPackedGetLocal_y(BLOCK_POS_PACKED12) + 0.5F,
        (float)ORIGIN.z + (float)cmath_chunkBlockPosPackedGetLocal_z(BLOCK_POS_PACKED12) + 0.5F};
}
#pragma endregion
#pragma region Queries
/// @brief Calculates the chunk's bounds in world space
static inline Boundsi_t chunk_getBounds(const Vec3i_t CHUNK_POS)
{
    const Vec3i_t WORLD_POS = chunkPos_to_worldOrigin(CHUNK_POS);
    return (Boundsi_t){
        .A = WORLD_POS,
        .B = (Vec3i_t){
            .x = WORLD_POS.x + CHUNK_AXIS_LENGTH,
            .y = WORLD_POS.y + CHUNK_AXIS_LENGTH,
            .z = WORLD_POS.z + CHUNK_AXIS_LENGTH}};
}
#pragma endregion
#pragma region Undefines

#pragma endregion