#include "core/logs.h"
#include "core/types/state_t.h"
#include "cmath/weightedMap_t.h"
#include "world/chunk.h"
#include "world/voxel/block_t.h"
#include "chunkGenerator.h"
#include "cmath/weightedMaps.h"
#include "core/randomNoise.h"
#include "chunkManager.h"

void chunkGen_stoneNoise_init(State_t *pState)
{
    // TODO:
    // Add stone pallet selection and a better mapping selection _t for associating what adjacencies and rarities
    // Add a very simple perlin worm carver
    // change gen order to be solid/not-solid carver and then painting only nonair
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

static const BlockID_t mapNoiseToStone(const State_t *pSTATE, const float noise)
{
    const uint32_t stoneIdx = weightedMap_pick(&pSTATE->weightedMaps.pWeightMaps[WEIGHTED_MAP_STONE], noise);
    const BlockID_t blockID = gStoneIDs[stoneIdx];

    return blockID;
}

static bool chunkGen_cave_carve(const BlockDefinition_t *const *pBLOCK_DEFINITIONS, Chunk_t *pChunk)
{
    const float AIR_THRESHOLD = 0.5F;
    bool hasSolid = true;
    for (uint8_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint8_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint8_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                BlockVoxel_t *pBlock = &pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(x, y, z)];

                pBlock->blockPosPacked12 = blockPos_pack_localXYZ(x, y, z);

                float carveNoise = randomNoise_carving_sampleXYZ(pChunk, pBlock->blockPosPacked12);
                bool isSolid = carveNoise > AIR_THRESHOLD;

                if (!isSolid)
                    pBlock->blockPosPacked12 = blockPosPacked_flag_set(pBlock->blockPosPacked12, BLOCKPOS_PACKED_FLAG_AIR);

                BlockID_t stoneID = isSolid ? BLOCK_ID_STONE : BLOCK_ID_AIR;
                pBlock->pBLOCK_DEFINITION = pBLOCK_DEFINITIONS[stoneID];

                if (!hasSolid && isSolid)
                    hasSolid = true;
            }

    return hasSolid;
}

/// @brief MUST be called before painting and AFTER carving. Assumes packedPos/flags and blockID are set
static bool chunkGen_fillSingleAirsInChunk(const BlockDefinition_t *const *pBLOCK_DEFINITIONS, Chunk_t *pChunk)
{
    // Does NOT fill the outer border at this time (to avoid checking neighbor chunks)
    // Neighbor check avoidance is because checking neighbor chunks is loading dependent and would not make this deterministic.
    // But by checking the chunk itself it is the same every time
    for (uint8_t x = 1; x < CHUNK_AXIS_LENGTH - 1; x++)
        for (uint8_t y = 1; y < CHUNK_AXIS_LENGTH - 1; y++)
            for (uint8_t z = 1; z < CHUNK_AXIS_LENGTH - 1; z++)
            {
                BlockVoxel_t *pBlock = &pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(x, y, z)];

                // Only focus on blocks that are surrounded by solid (to eliminate air pockets)
                if (block_isSolid(pBlock->blockPosPacked12))
                    continue;

                bool allNeighborsSolid = true;
                for (int face = 0; face < CUBE_FACE_COUNT; ++face)
                {
                    const CubeFace_t CUBE_FACE = (CubeFace_t)face;

                    // compute in signed space to avoid unsigned wrap tricks
                    const uint8_t NX = x + (uint8_t)spNEIGHBOR_OFFSETS[CUBE_FACE].x;
                    const uint8_t NY = y + (uint8_t)spNEIGHBOR_OFFSETS[CUBE_FACE].y;
                    const uint8_t NZ = z + (uint8_t)spNEIGHBOR_OFFSETS[CUBE_FACE].z;

                    // interior loop guarantees 0..15 here, so no bounds check needed
                    if (!block_isSolid(pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(NX, NY, NZ)].blockPosPacked12))
                    {
                        allNeighborsSolid = false;
                        break;
                    }
                }

                if (allNeighborsSolid)
                {
                    pBlock->blockPosPacked12 = blockPosPacked_flag_clear(pBlock->blockPosPacked12, BLOCKPOS_PACKED_FLAG_AIR);
                    pBlock->pBLOCK_DEFINITION = pBLOCK_DEFINITIONS[BLOCK_ID_STONE];
                }
            }

    return true;
}

static bool chunkGen_paint(const State_t *pSTATE, const BlockDefinition_t *const *pBLOCK_DEFINITIONS, Chunk_t *pChunk)
{
    for (uint8_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint8_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint8_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                BlockVoxel_t *pBlock = &pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(x, y, z)];

                if (!block_isSolid(pBlock->blockPosPacked12))
                    continue;

                float n = randomNoise_stone_samplePackedPos(pChunk, pBlock->blockPosPacked12);
                BlockID_t stoneID = mapNoiseToStone(pSTATE, n);

                pBlock->pBLOCK_DEFINITION = pBLOCK_DEFINITIONS[stoneID];
            }

    return true;
}

bool chunkGen_genChunk(const State_t *pSTATE, const BlockDefinition_t *const *pBLOCK_DEFINITIONS, Chunk_t *pChunk)
{
    bool hasSolid = chunkGen_cave_carve(pBLOCK_DEFINITIONS, pChunk);

    if (hasSolid)
    {
        chunkGen_fillSingleAirsInChunk(pBLOCK_DEFINITIONS, pChunk);
        chunkGen_paint(pSTATE, pBLOCK_DEFINITIONS, pChunk);
    }

    return true;
}