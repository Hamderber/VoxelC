#pragma region Includes
#pragma once
#include <stdint.h>
#include "cmath/cmath.h"
#pragma endregion
#pragma region Defines
#define BLOCKPOS_FLAGS_SHIFT 12U
#define BLOCKPOS_FLAGS_MASK (0xFU << BLOCKPOS_FLAGS_SHIFT)
#define BLOCKPOS_INDEX_MASK 0x0FFFU

/// @brief 4 Block flags packed into last bits of blockpos. Implicit flags: !AIR = SOLID, !LIQUID = AIR || SOLID
typedef enum BlockPosPackedFlag_e
{
    // Bit 12 of packedPos
    BLOCKPOS_PACKED_FLAG_AIR,
    // Bit 13 of packedPos
    BLOCKPOS_PACKED_FLAG_LIQUID,
    // Bit 14 of packedPos
    BLOCKPOS_PACKED_FLAG_RESERVED_1,
    // Bit 15 of packedPos
    BLOCKPOS_PACKED_FLAG_RESERVED_2,
} BlockPosPackedFlag_e;
#pragma endregion
#pragma region Operations
/// @brief Converts a block's x y z (local space) to the packed12 + flags
static inline uint16_t blockPos_pack_localXYZ_flags(const uint8_t LOCAL_X, const uint8_t LOCAL_Y, const uint8_t LOCAL_Z,
                                                    const uint8_t FLAGS_0_TO_15)
{
    return (uint16_t)(((uint16_t)(FLAGS_0_TO_15 & 0x0F) << BLOCKPOS_FLAGS_SHIFT) |
                      blockPos_pack_localXYZ(LOCAL_X, LOCAL_Y, LOCAL_Z));
}

/// @brief Mask off flags, returning the pure 12-bit local index (X:4 | Y:4 | Z:4)
static inline uint16_t blockPosPacked_index12(const uint16_t BLOCK_POS_PACKED)
{
    return (uint16_t)(BLOCK_POS_PACKED & BLOCKPOS_INDEX_MASK);
}

/// @brief Per-bit set/clear/test for the 4 flags (bit 0..3 inside nibble → bits 12..15 overall)
static inline uint16_t blockPosPacked_flag_set(const uint16_t P, const BlockPosPackedFlag_e BIT_0_TO_3)
{
    return (uint16_t)(P | (uint16_t)(1U << (BLOCKPOS_FLAGS_SHIFT + (BIT_0_TO_3 & 3))));
}
static inline uint16_t blockPosPacked_flag_clear(const uint16_t P, const BlockPosPackedFlag_e BIT_0_TO_3)
{
    return (uint16_t)(P & (uint16_t)~(uint16_t)(1U << (BLOCKPOS_FLAGS_SHIFT + (BIT_0_TO_3 & 3))));
}
static inline bool blockPosPacked_flag_get(const uint16_t P, const BlockPosPackedFlag_e BIT_0_TO_3)
{
    return (((P >> (BLOCKPOS_FLAGS_SHIFT + (BIT_0_TO_3 & 3))) & 1U) != 0);
}

/// @brief Checks block packed flags to determine if it has been flagged as solid (not air)
static inline bool block_isSolid(const uint16_t BLOCK_POS_PACKED)
{
    return !blockPosPacked_flag_get(BLOCK_POS_PACKED, BLOCKPOS_PACKED_FLAG_AIR);
}
#pragma endregion
#pragma region Undefines
#undef BLOCKPOS_FLAGS_SHIFT
#undef BLOCKPOS_FLAGS_MASK
#undef BLOCKPOS_INDEX_MASK
#pragma endregion