#pragma once
#include <stdint.h>
#include <string.h>
#include "cmath/cmath.h"
#include "chunk.h"

enum
{
    AIR = 0,
    SOLID = 1,
    CHUNK_SIDE = 16,
    HALO = 1,
    GRID_SIDE = CHUNK_SIDE + 2 * HALO,
    GRID_STRIDE_X = 1,
    GRID_STRIDE_Y = GRID_SIDE,
    GRID_STRIDE_Z = GRID_SIDE * GRID_SIDE,
    GRID_SIZE = GRID_SIDE * GRID_SIDE * GRID_SIDE
};

/// @brief Contains a 1D array of tracking the solid state of the block.
/// Has a 1 block border around the chunk in this grid to prevent out of bounds checking.
typedef struct ChunkSolidityGrid_t
{
    uint8_t *pGrid;
} ChunkSolidityGrid_t;

/// @brief Gets the grid index from block local coords
static inline size_t chunkSolidityGrid_index18(const uint8_t X, const uint8_t Y, const uint8_t Z)
{
    return (size_t)X + (size_t)Y * GRID_STRIDE_Y + (size_t)Z * GRID_STRIDE_Z;
}

/// @brief Map local block coords into the halo grid center
static inline size_t chunkSolidityGrid_index16(const uint8_t LOCAL_X, const uint8_t LOCAL_Y, const uint8_t LOCAL_Z)
{
    return chunkSolidityGrid_index18(LOCAL_X + HALO, LOCAL_Y + HALO, LOCAL_Z + HALO);
}

/// @brief Sets each grid point to value
static inline void chunkSolidityGrid_fill(ChunkSolidityGrid_t *pSolidity, const uint8_t VALUE)
{
    if (!pSolidity)
        return;

    memset(pSolidity->pGrid, VALUE ? SOLID : AIR, GRID_SIZE);
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
    return (uint8_t)(chunkSolidityGrid_neighbors_anySolid(pSOLIDITY, INDEX_16) == AIR);
}

/// @brief Initalizes the passed grid (memory allocation) and sets each point to solid
static inline ChunkSolidityGrid_t *chunkSolidityGrid_init(void)
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

    chunkSolidityGrid_fill(pSolidity, SOLID);

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