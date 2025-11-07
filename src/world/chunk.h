#pragma region Includes
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "rendering/types/renderChunk_t.h"
#include "world/voxel/block_t.h"
#include "collection/linkedList_t.h"
#pragma endregion
#pragma region Definitions
#define LXYZ_MASK_4 0x0FU
#define LXYZ_SHIFT_X 8U
#define LXYZ_SHIFT_Y 4U
#define LXYZ_SHIFT_Z 0U
#define BLOCKPOS_FLAGS_SHIFT 12U
#define BLOCKPOS_FLAGS_MASK (0xFU << BLOCKPOS_FLAGS_SHIFT)
#define BLOCKPOS_INDEX_MASK 0x0FFFU

// This shall NEVER change
static const uint16_t CHUNK_AXIS_LENGTH = 16;
// 16x16x16
static const uint32_t CHUNK_BLOCK_CAPACITY = 4096;

typedef struct Chunk_t
{
    RenderChunk_t *pRenderChunk;
    BlockVoxel_t *pBlockVoxels;
    Vec3i_t chunkPos;
    LinkedList_t *pEntitiesLoadingChunkLL;
} Chunk_t;

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
/// @brief 4 Block flags packed into last bits of blockpos. Implicit flags: !AIR = SOLID, !LIQUID = AIR || SOLID
typedef enum
{
    // Bit 12 of packedPos
    BLOCKPOS_PACKED_FLAG_AIR,
    // Bit 13 of packedPos
    BLOCKPOS_PACKED_FLAG_LIQUID,
    // Bit 14 of packedPos
    BLOCKPOS_PACKED_FLAG_RESERVED_1,
    // Bit 15 of packedPos
    BLOCKPOS_PACKED_FLAG_RESERVED_2,
} BlockPosPackedFlag_t;

/// @brief Converts a block's x y z (local space) to the packed12 (flags = 0)
static inline uint16_t blockPos_pack_localXYZ(const uint8_t LOCAL_X, const uint8_t LOCAL_Y, const uint8_t LOCAL_Z)
{
    return (uint16_t)((LOCAL_X & LXYZ_MASK_4) << LXYZ_SHIFT_X) |
           (uint16_t)((LOCAL_Y & LXYZ_MASK_4) << LXYZ_SHIFT_Y) |
           (uint16_t)((LOCAL_Z & LXYZ_MASK_4) << LXYZ_SHIFT_Z);
}

/// @brief Converts a block's x y z (local space) to the packed12 + flags
static inline uint16_t blockPos_pack_localXYZ_flags(const uint8_t LOCAL_X, const uint8_t LOCAL_Y, const uint8_t LOCAL_Z,
                                                    const uint8_t FLAGS_0_TO_15)
{
    return (uint16_t)(((uint16_t)(FLAGS_0_TO_15 & 0x0F) << BLOCKPOS_FLAGS_SHIFT) |
                      blockPos_pack_localXYZ(LOCAL_X, LOCAL_Y, LOCAL_Z));
}

static inline uint8_t blockPosPacked_getLocal_x(const uint16_t P) { return (uint8_t)((P >> LXYZ_SHIFT_X) & LXYZ_MASK_4); }
static inline uint8_t blockPosPacked_getLocal_y(const uint16_t P) { return (uint8_t)((P >> LXYZ_SHIFT_Y) & LXYZ_MASK_4); }
static inline uint8_t blockPosPacked_getLocal_z(const uint16_t P) { return (uint8_t)((P >> LXYZ_SHIFT_Z) & LXYZ_MASK_4); }

/// @brief Mask off flags, returning the pure 12-bit local index (X:4 | Y:4 | Z:4)
static inline uint16_t blockPosPacked_index12(const uint16_t BLOCK_POS_PACKED)
{
    return (uint16_t)(BLOCK_POS_PACKED & BLOCKPOS_INDEX_MASK);
}

/// @brief Per-bit set/clear/test for the 4 flags (bit 0..3 inside nibble → bits 12..15 overall)
static inline uint16_t blockPosPacked_flag_set(const uint16_t P, const BlockPosPackedFlag_t BIT_0_TO_3)
{
    return (uint16_t)(P | (uint16_t)(1U << (BLOCKPOS_FLAGS_SHIFT + (BIT_0_TO_3 & 3))));
}
static inline uint16_t blockPosPacked_flag_clear(const uint16_t P, const BlockPosPackedFlag_t BIT_0_TO_3)
{
    return (uint16_t)(P & (uint16_t)~(uint16_t)(1U << (BLOCKPOS_FLAGS_SHIFT + (BIT_0_TO_3 & 3))));
}
static inline bool blockPosPacked_flag_get(const uint16_t P, const BlockPosPackedFlag_t BIT_0_TO_3)
{
    return (((P >> (BLOCKPOS_FLAGS_SHIFT + (BIT_0_TO_3 & 3))) & 1U) != 0);
}

/// @brief Converts a block's position packed12 to world pos
static inline Vec3i_t blockPosPacked_get_worldPos(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3i_t ORIGIN = chunkPos_to_worldOrigin(CHUNK_POS);
    return (Vec3i_t){
        ORIGIN.x + (int)blockPosPacked_getLocal_x(BLOCK_POS_PACKED12),
        ORIGIN.y + (int)blockPosPacked_getLocal_y(BLOCK_POS_PACKED12),
        ORIGIN.z + (int)blockPosPacked_getLocal_z(BLOCK_POS_PACKED12)};
}

/// @brief Converts a block's position packed12 to block index in the chunk
static inline uint16_t blockPosPacked_to_chunkBlockIndex(const uint16_t BLOCK_POS_PACKED)
{
    return blockPosPacked_getLocal_x(BLOCK_POS_PACKED) * CHUNK_AXIS_LENGTH * CHUNK_AXIS_LENGTH +
           blockPosPacked_getLocal_y(BLOCK_POS_PACKED) * CHUNK_AXIS_LENGTH +
           blockPosPacked_getLocal_z(BLOCK_POS_PACKED);
}

/// @brief Converts a block's x y z (local space) to block index in the chunk
static inline uint16_t xyz_to_chunkBlockIndex(const uint8_t X, const uint8_t Y, const uint8_t Z)
{
    return X * CHUNK_AXIS_LENGTH * CHUNK_AXIS_LENGTH + Y * CHUNK_AXIS_LENGTH + Z;
}

/// @brief Sample pos is in the center of the voxel (offset by 0.5F)
static inline Vec3f_t blockPacked_to_worldSamplePos(const Vec3i_t CHUNK_POS, const uint16_t BLOCK_POS_PACKED12)
{
    const Vec3i_t ORIGIN = chunkPos_to_worldOrigin(CHUNK_POS);
    return (Vec3f_t){
        (float)ORIGIN.x + (float)blockPosPacked_getLocal_x(BLOCK_POS_PACKED12) + 0.5F,
        (float)ORIGIN.y + (float)blockPosPacked_getLocal_y(BLOCK_POS_PACKED12) + 0.5F,
        (float)ORIGIN.z + (float)blockPosPacked_getLocal_z(BLOCK_POS_PACKED12) + 0.5F};
}
#pragma endregion
#pragma region Queries
/// @brief Checks block packed flags to determine if it has been flagged as solid (not air)
static inline bool block_isSolid(const uint16_t BLOCK_POS_PACKED)
{
    return !blockPosPacked_flag_get(BLOCK_POS_PACKED, BLOCKPOS_PACKED_FLAG_AIR);
}

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
#undef LXYZ_MASK_4
#undef LXYZ_SHIFT_X
#undef LXYZ_SHIFT_Y
#undef LXYZ_SHIFT_Z
#undef BLOCKPOS_FLAGS_SHIFT
#undef BLOCKPOS_FLAGS_MASK
#undef BLOCKPOS_INDEX_MASK
#pragma endregion