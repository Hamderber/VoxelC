#pragma region Includes
#include "core/logs.h"
#include "cmath/cmath.h"
#include "core/types/state_t.h"
#include "cmath/weightedMap_t.h"
#include "world/chunk.h"
#include "world/voxel/block_t.h"
#include "chunkGenerator.h"
#include "cmath/weightedMaps.h"
#include "core/randomNoise.h"
#include "chunkManager.h"
#include "chunkSolidityGrid.h"
#pragma endregion
#pragma region Settings
static const float CARVING_AIR_THRESHOLD = 0.5F;
#pragma endregion
void chunkGen_stoneNoise_init(State_t *pState)
{
    // TODO:
    // Add stone pallet selection and a better mapping selection _t for associating what adjacencies and rarities
    // Change this to part of the world config json
    float pStoneWeights[BLOCK_DEFS_STONE_COUNT] = {
        5.50F,
        4.50F,
        1.50F,
        1.50F,
        0.50F,
        0.75F,
        0.50F,
        0.25F,
        0.50F,
        0.75F,
        0.50F,
        1.50F,
        4.50F,
        5.50F,
    };

    WeightedMap_t *pWeightedMap = &pState->weightedMaps.pWeightMaps[WEIGHTED_MAP_STONE];
    pWeightedMap->cdf = gStoneCDF;
    if (!weightedMap_bake(pWeightedMap, pStoneWeights, BLOCK_DEFS_STONE_COUNT))
    {
        logs_log(LOG_ERROR, "Failed to bake stone weight map!");
        return;
    }

    logs_log(LOG_DEBUG, "Stone weight map baked (total weight = %.2f)", pWeightedMap->total);
    logs_log(LOG_DEBUG, "Stone CDF: count=%u total=%.3f first=%.3f last=%.3f",
             pWeightedMap->count, pWeightedMap->total,
             pWeightedMap->cdf[0], pWeightedMap->cdf[pWeightedMap->count - 1]);
}

// The order of these determines what will occur adjacent (blending)
static const BlockID_t gStoneIDs[BLOCK_DEFS_STONE_COUNT] = {
    BLOCK_ID_STONE,
    BLOCK_ID_ANDESITE,
    BLOCK_ID_CHERT,
    BLOCK_ID_LIMESTONE,
    BLOCK_ID_SANDSTONE_YELLOW,
    BLOCK_ID_GRANITE,
    BLOCK_ID_JASPER,
    BLOCK_ID_SANDSTONE_RED,
    BLOCK_ID_SLATE,
    BLOCK_ID_SHALE,
    BLOCK_ID_MARBLE_BLACK,
    BLOCK_ID_DIORITE,
    BLOCK_ID_CHALK,
    BLOCK_ID_MARBLE_WHITE,
};

static const BlockID_t mapNoiseToStone(const State_t *restrict pSTATE, const float NOISE)
{
    const uint32_t STONE_INDEX = weightedMap_pick(&pSTATE->weightedMaps.pWeightMaps[WEIGHTED_MAP_STONE], NOISE);
    const BlockID_t BLOCK_ID = gStoneIDs[STONE_INDEX];

    return BLOCK_ID;
}

static bool chunkGen_cave_carve(const Vec3i_t CHUNK_POS, uint16_t *restrict pPackedPos, ChunkSolidityGrid_t *restrict pSolidity)
{
    if (!pSolidity || !pPackedPos)
        return false;

    bool chunkHasSolid = false;

    // Full chunk (not cross check to keep determinism)
    for (size_t i = 0; i < CMATH_CHUNK_POINTS_PACKED_COUNT; i++)
    {
        float carveNoise = randomNoise_carving_sampleXYZ(CHUNK_POS, pPackedPos[i]);
        bool isSolid = carveNoise > CARVING_AIR_THRESHOLD;

        if (!isSolid)
            pPackedPos[i] = blockPosPacked_flag_set(pPackedPos[i], BLOCKPOS_PACKED_FLAG_AIR);

        chunkHasSolid = chunkHasSolid || isSolid;
    }

    // Build again using the modified packedpos (because flags were changed)
    chunkSolidityGrid_build(pSolidity, pPackedPos);

    return chunkHasSolid;
}

/// @brief MUST be called before painting and AFTER carving. Assumes packedPos/flags and blockID are set
static bool chunkGen_fillSingleAirsInChunk(uint16_t *restrict pPackedPos, ChunkSolidityGrid_t *restrict pSolidity)
{
    const Vec3u8_t *pPOS = cmath_chunkPoints_Get();

    // Full chunk (not cross check to keep determinism)
    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        // Only focus on blocks that are air
        if (pSolidity->pGrid[chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z)])
            continue;

        if (chunkSolidityGrid_neighbors_allSolid(pSolidity, chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z)))
            pPackedPos[i] = blockPosPacked_flag_clear(pPackedPos[i], BLOCKPOS_PACKED_FLAG_AIR);
    }

    // Build again using the modified packedpos (because flags were changed)
    chunkSolidityGrid_build(pSolidity, pPackedPos);

    return true;
}

/// @brief MUST be called before painting and AFTER carving. Assumes packedPos/flags and blockID are set
static bool chunkGen_clearSingleSolidsInChunk(uint16_t *restrict pPackedPos, ChunkSolidityGrid_t *restrict pSolidity)
{
    const Vec3u8_t *pPOS = cmath_chunkPoints_Get();

    // Full chunk (not cross check)
    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        // Only focus on blocks that are solid
        if (!pSolidity->pGrid[chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z)])
            continue;

        if (chunkSolidityGrid_neighbors_noSolid(pSolidity, chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z)))
            pPackedPos[i] = blockPosPacked_flag_set(pPackedPos[i], BLOCKPOS_PACKED_FLAG_AIR);
    }

    // Build again using the modified packedpos (because flags were changed)
    chunkSolidityGrid_build(pSolidity, pPackedPos);

    return true;
}

static bool chunkGen_paintStone(const State_t *restrict pSTATE, const BlockDefinition_t *const *restrict pBLOCK_DEFINITIONS,
                                Chunk_t *restrict pChunk, const Vec3i_t CHUNK_POS, const ChunkSolidityGrid_t *restrict pSOLIDITY)
{
    const Vec3u8_t *pPOS = cmath_chunkPoints_Get();
    BlockVoxel_t *pBlock = NULL;

    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        pBlock = &pChunk->pBlockVoxels[i];

        if (!pSOLIDITY->pGrid[chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z)])
            continue;

        float stoneNoise = randomNoise_stone_samplePackedPos(CHUNK_POS, pBlock->blockPosPacked12);
        BlockID_t stoneID = mapNoiseToStone(pSTATE, stoneNoise);

        pBlock->pBLOCK_DEFINITION = pBLOCK_DEFINITIONS[stoneID];
    }

    return true;
}

static bool chunkGen_blockDefinitionsInit(const BlockDefinition_t *const *restrict pBLOCK_DEFINITIONS, Chunk_t *restrict pChunk,
                                          const uint16_t *restrict pPACKED_POS, const ChunkSolidityGrid_t *restrict pSOLIDITY)
{
    const Vec3u8_t *pPOS = cmath_chunkPoints_Get();

    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        pChunk->pBlockVoxels[i].blockPosPacked12 = pPACKED_POS[i];
        pChunk->pBlockVoxels[i].pBLOCK_DEFINITION = pSOLIDITY->pGrid[chunkSolidityGrid_index16(pPOS[i].x, pPOS[i].y, pPOS[i].z)]
                                                        ? pBLOCK_DEFINITIONS[BLOCK_ID_STONE]
                                                        : pBLOCK_DEFINITIONS[BLOCK_ID_AIR];
    }

    return true;
}

ChunkSolidityGrid_t *chunkGen_transparencyGrid(const Chunk_t *restrict pCHUNK)
{
    if (!pCHUNK || !pCHUNK->pBlockVoxels)
        return NULL;

    ChunkSolidityGrid_t *pTransparancyGrid = chunkSolidityGrid_init(SOLIDITY_AIR);
    if (!pTransparancyGrid)
        return NULL;

    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        uint16_t packedPos = pCHUNK->pBlockVoxels[i].blockPosPacked12;
        const BlockDefinition_t *pBLOCK_DEF = pCHUNK->pBlockVoxels[i].pBLOCK_DEFINITION;
        bool isTransparent = blockDef_isTransparent(pBLOCK_DEF);
        // flip isTransparent so transparency is stored as 0U in the grid
        pTransparancyGrid->pGrid[chunkSolidityGrid_index16_fromPacked(packedPos)] = (uint8_t)!isTransparent;
    }

    return pTransparancyGrid;
}

bool chunkGen_genChunk(const State_t *restrict pSTATE, const BlockDefinition_t *const *restrict pBLOCK_DEFINITIONS,
                       Chunk_t *restrict pChunk)
{
    if (!pSTATE || !pBLOCK_DEFINITIONS || !pChunk)
    {
        logs_log(LOG_ERROR, "Attempted to generate a chunk with invalid pointer parameters!");
        return false;
    }

    const Vec3i_t CHUNK_POS = pChunk->chunkPos;

    size_t size = sizeof(uint16_t) * CMATH_CHUNK_POINTS_PACKED_COUNT;
    uint16_t *pPackedPos = malloc(size);
    if (!pPackedPos)
        return false;

    memcpy_s(pPackedPos, size, cmath_chunkPointsPacked_Get(), size);

    // This is NOT the solidity that is stored in the chunk. This is an intermediate tool used for faster generation (doesn't)
    // require passing the entire block struct array around between generation steps
    ChunkSolidityGrid_t *pStoneSolidity = chunkSolidityGrid_init(SOLIDITY_SOLID);
    if (!pStoneSolidity)
        return false;

    chunkSolidityGrid_build(pStoneSolidity, cmath_chunkPointsPacked_Get());

    // Carve the chunk. Non-air is basic stone first
    const bool CHUNK_HAS_ANYTHING_SOLID = chunkGen_cave_carve(CHUNK_POS, pPackedPos, pStoneSolidity);

    // Mitigate small air pockets and single floating stones
    if (CHUNK_HAS_ANYTHING_SOLID)
    {
        chunkGen_fillSingleAirsInChunk(pPackedPos, pStoneSolidity);
        chunkGen_clearSingleSolidsInChunk(pPackedPos, pStoneSolidity);
    }

    chunkGen_blockDefinitionsInit(pBLOCK_DEFINITIONS, pChunk, pPackedPos, pStoneSolidity);

    // Apply characteristics if there is anything solid
    if (CHUNK_HAS_ANYTHING_SOLID)
        // This is by far the most expensive operation
        chunkGen_paintStone(pSTATE, pBLOCK_DEFINITIONS, pChunk, CHUNK_POS, pStoneSolidity);

    pChunk->pTransparencyGrid = chunkGen_transparencyGrid(pChunk);

    free(pPackedPos);
    pPackedPos = NULL;
    chunkSolidityGrid_destroy(pStoneSolidity);
    return true;
}