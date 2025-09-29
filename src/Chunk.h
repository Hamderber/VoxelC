#pragma once

#include "Toolkit.h"

/// @brief First 32 bits (X) Second 32 bits (Z)
typedef uint64_t ChunkPosPacked_t;

typedef struct
{
    ChunkPosPacked_t pos;
} *Chunk_t;

typedef struct
{
    int32_t xPos, zPos;
} ChunkPosUnpacked_t;

/// @brief Packs the signed 32-bit x and z values into an unsigned 64-bit value via bit operations.
/// @param x
/// @param z
/// @return
uint64_t packChunkPos(int32_t x, int32_t z)
{
    // Casting x or z to a uint64 just adds 32 zeros at the end which can then be shifted and combined
    // Have to cast to uint32 first to just add 32 zeros to the exact int32 bits. Otherwise, a negative
    // int32 could be incorrectly cast as a large uint64
    return ((uint64_t)(uint32_t)x << 32) | ((uint32_t)z);
}

/// @brief Unpack x from high 32 bits, z from low 32 bits
/// @param pos
/// @return
ChunkPosUnpacked_t unpackChunkPos(uint64_t pos)
{
    ChunkPosUnpacked_t unpacked;
    // Casting just takes the first 32 bits.
    unpacked.zPos = (int32_t)(pos);
    // Shift the top 32 bits to the lower bits and then take those bits.
    unpacked.xPos = (int32_t)(pos >> 32);
    return unpacked;
}
