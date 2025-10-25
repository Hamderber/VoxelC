#pragma once
#include <stdint.h>
#include <stdbool.h>

/*
Layout: [63][62‒42][41‒21][20‒0]
         isLoaded |  X  |  Y  |  Z
 Each axis: signed 21-bit (−1,048,576 -> +1,048,575)
 */
typedef uint64_t ChunkPosPacked_t;

typedef struct
{
    int32_t xPos;
    int32_t yPos;
    int32_t zPos;
    bool isLoaded;
} ChunkPosUnpacked_t;

#define CHUNK_BITS_PER_AXIS 21
#define CHUNK_AXIS_MASK ((1ULL << CHUNK_BITS_PER_AXIS) - 1ULL)
#define CHUNK_FLAG_ISLOADED (1ULL << 63)

/// @brief Packs signed 21-bit x/y/z coordinates and a 1-bit isLoaded flag into 64 bits.
/// @param x
/// @param y
/// @param z
/// @param isLoaded
/// @return uint64_t
static inline uint64_t packChunkPos3D(int32_t x, int32_t y, int32_t z, bool isLoaded)
{
    uint64_t ux = ((uint64_t)(x & CHUNK_AXIS_MASK)) << (CHUNK_BITS_PER_AXIS * 2); // bits 42–62
    uint64_t uy = ((uint64_t)(y & CHUNK_AXIS_MASK)) << (CHUNK_BITS_PER_AXIS * 1); // bits 21–41
    uint64_t uz = ((uint64_t)(z & CHUNK_AXIS_MASK));                              // bits 0–20
    uint64_t flag = isLoaded ? CHUNK_FLAG_ISLOADED : 0LL;
    return flag | ux | uy | uz;
}

/// @brief Unpacks the 64-bit chunk position into signed x/y/z and isLoaded flag.
static inline ChunkPosUnpacked_t unpackChunkPos3D(uint64_t packed)
{
    ChunkPosUnpacked_t out;
    out.isLoaded = (packed & CHUNK_FLAG_ISLOADED) != 0;

    out.zPos = (int32_t)(packed & CHUNK_AXIS_MASK);
    out.yPos = (int32_t)((packed >> CHUNK_BITS_PER_AXIS) & CHUNK_AXIS_MASK);
    out.xPos = (int32_t)((packed >> (CHUNK_BITS_PER_AXIS * 2)) & CHUNK_AXIS_MASK);

    // Sign extension for 21-bit signed values
    if (out.xPos & (1 << (CHUNK_BITS_PER_AXIS - 1)))
        out.xPos |= ~((1 << CHUNK_BITS_PER_AXIS) - 1);
    if (out.yPos & (1 << (CHUNK_BITS_PER_AXIS - 1)))
        out.yPos |= ~((1 << CHUNK_BITS_PER_AXIS) - 1);
    if (out.zPos & (1 << (CHUNK_BITS_PER_AXIS - 1)))
        out.zPos |= ~((1 << CHUNK_BITS_PER_AXIS) - 1);

    return out;
}

/// @brief Marks a packed chunk as loaded.
static inline uint64_t chunkSetLoaded(uint64_t packed, bool loaded)
{
    if (loaded)
        return packed | CHUNK_FLAG_ISLOADED;
    else
        return packed & ~CHUNK_FLAG_ISLOADED;
}

/// @brief Checks if the packed chunk is loaded.
static inline bool chunkIsLoaded(uint64_t packed)
{
    return (packed & CHUNK_FLAG_ISLOADED) != 0;
}