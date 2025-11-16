#pragma region Includes
#pragma once
#include <stdint.h>
#include <string.h>
#include "cmath/cmath.h"
#include "world/voxel/blockFlags.h"
#pragma endregion
#pragma region Defines
#define GRID_SIDE (CHUNK_AXIS_LENGTH + ((uint16_t)2))
#define GRID_STRIDE_X ((uint8_t)1)
#define GRID_STRIDE_Y GRID_SIDE
#define GRID_STRIDE_Z (GRID_SIDE * GRID_SIDE)
#define GRID_SIZE (GRID_SIDE * GRID_SIDE * GRID_SIDE)
#define HALO ((uint8_t)1)
typedef enum SolidityType_e
{
    SOLIDITY_AIR = 0,
    SOLIDITY_TRANSPARENT = 0,
    SOLIDITY_SOLID = 1,
} SolidityType_e;
/// @brief Contains a 1D array of tracking the solid state of the block.
/// Has a 1 block border around the chunk in this grid to prevent out of bounds checking.
typedef struct ChunkSolidityGrid_t
{
    uint8_t *pGrid;
} ChunkSolidityGrid_t;
#pragma endregion
#pragma region Operations
/// @brief Gets the grid index from block local coords
static inline size_t chunkSolidityGrid_index18(const uint8_t X, const uint8_t Y, const uint8_t Z)
{
    return (size_t)X * GRID_STRIDE_X + (size_t)Y * GRID_STRIDE_Y + (size_t)Z * GRID_STRIDE_Z;
}

/// @brief Map local block coords into the halo grid center
static inline size_t chunkSolidityGrid_index16(const uint8_t LOCAL_X, const uint8_t LOCAL_Y, const uint8_t LOCAL_Z)
{
    return chunkSolidityGrid_index18(LOCAL_X + HALO, LOCAL_Y + HALO, LOCAL_Z + HALO);
}

/// @brief Map local block coords into the halo grid center (from packedPos)
static inline size_t chunkSolidityGrid_index16_fromPacked(const uint16_t PACKED)
{
    const Vec3u8_t LOCAL = {
        .x = cmath_chunkBlockPosPackedGetLocal_x(PACKED),
        .y = cmath_chunkBlockPosPackedGetLocal_y(PACKED),
        .z = cmath_chunkBlockPosPackedGetLocal_z(PACKED)};

    return chunkSolidityGrid_index18(LOCAL.x + HALO, LOCAL.y + HALO, LOCAL.z + HALO);
}

/// @brief Sets each grid point to value
static inline void chunkSolidityGrid_fill(ChunkSolidityGrid_t *pSolidity, const uint8_t VALUE)
{
    if (!pSolidity)
        return;

    memset(pSolidity->pGrid, VALUE ? SOLIDITY_SOLID : SOLIDITY_AIR, (size_t)GRID_SIZE);
}

/// @brief Builds the grid from the isSolid flag baked into each packedPos
static inline void chunkSolidityGrid_build(ChunkSolidityGrid_t *restrict pSolidity, const uint16_t *restrict pPACKED_POS)
{
    const Vec3u8_t *pPOS = cmath_chunkPoints_Get();

    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        const size_t INDEX_18 = chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z);
        pSolidity->pGrid[INDEX_18] = (uint8_t)block_isSolid(pPACKED_POS[i]);
    }
}

/// @brief Checks if each surrounding grid point is marked as solid
static inline uint8_t chunkSolidityGrid_neighbors_allSolid(const ChunkSolidityGrid_t *pSOLIDITY, const size_t INDEX_16)
{
    const uint8_t *pGRID = pSOLIDITY->pGrid;
    return (uint8_t)(pGRID[INDEX_16 - GRID_STRIDE_X] &
                     pGRID[INDEX_16 + GRID_STRIDE_X] &
                     pGRID[INDEX_16 - GRID_STRIDE_Y] &
                     pGRID[INDEX_16 + GRID_STRIDE_Y] &
                     pGRID[INDEX_16 - GRID_STRIDE_Z] &
                     pGRID[INDEX_16 + GRID_STRIDE_Z]);
}

/// @brief Checks if each surrounding grid point is marked as air
static inline uint8_t chunkSolidityGrid_neighbors_anySolid(const ChunkSolidityGrid_t *pSOLIDITY, const size_t INDEX_16)
{
    const uint8_t *pGRID = pSOLIDITY->pGrid;
    return (uint8_t)(pGRID[INDEX_16 - GRID_STRIDE_X] |
                     pGRID[INDEX_16 + GRID_STRIDE_X] |
                     pGRID[INDEX_16 - GRID_STRIDE_Y] |
                     pGRID[INDEX_16 + GRID_STRIDE_Y] |
                     pGRID[INDEX_16 - GRID_STRIDE_Z] |
                     pGRID[INDEX_16 + GRID_STRIDE_Z]);
}

/// @brief Checks if each surrounding grid point is marked as air
static inline uint8_t chunkSolidityGrid_neighbors_noSolid(const ChunkSolidityGrid_t *pSOLIDITY, const size_t INDEX_16)
{
    return (uint8_t)(chunkSolidityGrid_neighbors_anySolid(pSOLIDITY, INDEX_16) == SOLIDITY_AIR);
}

/// @brief Initalizes the passed grid (memory allocation) and sets each point to solid
static inline ChunkSolidityGrid_t *chunkSolidityGrid_init(SolidityType_e initialFill)
{
    ChunkSolidityGrid_t *pSolidity = malloc(sizeof(ChunkSolidityGrid_t));
    if (!pSolidity)
        return NULL;

    pSolidity->pGrid = malloc(sizeof(uint8_t) * GRID_SIZE);
    if (!pSolidity->pGrid)
    {
        free(pSolidity);
        pSolidity = NULL;
        return NULL;
    }

    chunkSolidityGrid_fill(pSolidity, initialFill);

    return pSolidity;
}

/// @brief Frees the internal 1D array and then the struct memory itself
static inline void chunkSolidityGrid_destroy(ChunkSolidityGrid_t *pSolidity)
{
    if (!pSolidity)
        return;

    free(pSolidity->pGrid);
    pSolidity->pGrid = NULL;
    free(pSolidity);
    pSolidity = NULL;
}
#pragma endregion
#pragma region Undefines
#undef GRID_SIDE
#undef GRID_STRIDE_X
#undef GRID_STRIDE_Y
#undef GRID_STRIDE_Z
#undef GRID_SIZE
#undef HALO
#pragma endregion