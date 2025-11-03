#pragma region Includes
#include <stdbool.h>
#include <string.h>
#include "core/types/state_t.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/types/shaderVertexVoxel_t.h"
#include "world/chunk.h"
#include "chunkManager.h"
#include "rendering/uvs.h"
#include "rendering/chunk/chunkRendering.h"
#include "core/random.h"
#include "world/world.h"
#include "core/randomNoise.h"
#include "cmath/weightedMap_t.h"
#include "chunkGenerator.h"
#include "core/logs.h"
#include "cmath/weightedMaps.h"
#pragma endregion
#pragma region Get Chunk
Chunk_t *chunkManager_getChunk(const State_t *pSTATE, const ChunkPos_t CHUNK_POS)
{
    Chunk_t *pChunk = NULL;

    for (size_t i = 0; i < pSTATE->pWorldState->chunkCount; i++)
    {
        Chunk_t *pCurrentChunk = pSTATE->pWorldState->ppChunks[i];

        // The chunk might try to access itself during generation/neighbor checks/etc
        if (!pCurrentChunk)
            continue;

        if (chunk_chunkPos_equals(pSTATE->pWorldState->ppChunks[i]->chunkPos, CHUNK_POS))
            pChunk = pSTATE->pWorldState->ppChunks[i];
    }

    return pChunk;
}
#pragma endregion
#pragma region Create Mesh
static bool chunk_mesh_create(State_t *pState, Chunk_t *pChunk)
{
    if (!pState || !pChunk || !pChunk->pBlockVoxels)
        return false;

    // max faces in a chunk possible (all transparent faces like glass)
    const size_t MAX_VERTEX_COUNT = (size_t)CHUNK_BLOCK_CAPACITY * FACE_COUNT * VERTS_PER_FACE;
    const size_t MAX_INDEX_COUNT = (size_t)CHUNK_BLOCK_CAPACITY * FACE_COUNT * INDICIES_PER_FACE;

    ShaderVertexVoxel_t *pVertices = calloc(MAX_VERTEX_COUNT, sizeof(ShaderVertexVoxel_t));
    uint32_t *pIndices = calloc(MAX_INDEX_COUNT, sizeof(uint32_t));
    if (!pVertices || !pIndices)
    {
        free(pVertices);
        free(pIndices);
        free(pChunk->pBlockVoxels);
        free(pChunk);
        return false;
    }

    size_t vertexCursor = 0;
    size_t indexCursor = 0;

    // Written to be accessed using CubeFace enum
    static const Vec3i_t spNEIGHBOR_OFFSETS[6] = {{-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    for (uint32_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint32_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint32_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                const BlockDefinition_t *pBLOCK = pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(x, y, z)].pBLOCK_DEFINITION;

                if (pBLOCK->BLOCK_ID == BLOCK_ID_AIR)
                    continue;

#pragma region Neighbor Check
                for (int face = 0; face < FACE_COUNT; ++face)
                {
                    CubeFace_t cubeFace = (CubeFace_t)face;
                    int nx = (int)x + spNEIGHBOR_OFFSETS[cubeFace].x;
                    int ny = (int)y + spNEIGHBOR_OFFSETS[cubeFace].y;
                    int nz = (int)z + spNEIGHBOR_OFFSETS[cubeFace].z;

                    bool neighborInChunk =
                        nx >= 0 && ny >= 0 && nz >= 0 &&
                        nx < (int)CHUNK_AXIS_LENGTH &&
                        ny < (int)CHUNK_AXIS_LENGTH &&
                        nz < (int)CHUNK_AXIS_LENGTH;

                    BlockVoxel_t neighbor = {0};
                    bool drawFace = true;

                    if (neighborInChunk)
                    {
                        neighbor = chunkManager_getBlock(pChunk, (Vec3i_t){nx, ny, nz});
                        if (neighbor.pBLOCK_DEFINITION->BLOCK_RENDER_TYPE == BLOCK_RENDER_SOLID)
                            drawFace = false;
                    }
                    else
                    {
                        // Determine which adjacent chunk this face crosses into
                        ChunkPos_t neighborChunkPos = pChunk->chunkPos;
                        if (nx < 0)
                            neighborChunkPos.x -= 1;
                        else if (nx >= (int)CHUNK_AXIS_LENGTH)
                            neighborChunkPos.x += 1;

                        if (ny < 0)
                            neighborChunkPos.y -= 1;
                        else if (ny >= (int)CHUNK_AXIS_LENGTH)
                            neighborChunkPos.y += 1;

                        if (nz < 0)
                            neighborChunkPos.z -= 1;
                        else if (nz >= (int)CHUNK_AXIS_LENGTH)
                            neighborChunkPos.z += 1;

                        // Get pointer to that chunk (NULL if not loaded)
                        Chunk_t *pNeighborChunk = chunkManager_getChunk(pState, neighborChunkPos);
                        if (pNeighborChunk)
                        {
                            // Prevent out of bounds
                            Vec3i_t localPos = {
                                (nx + CHUNK_AXIS_LENGTH) % CHUNK_AXIS_LENGTH,
                                (ny + CHUNK_AXIS_LENGTH) % CHUNK_AXIS_LENGTH,
                                (nz + CHUNK_AXIS_LENGTH) % CHUNK_AXIS_LENGTH};

                            neighbor = chunkManager_getBlock(pNeighborChunk, localPos);

                            // drawing even if neighbor is a block (but transparent)
                            if (neighbor.pBLOCK_DEFINITION->BLOCK_RENDER_TYPE == BLOCK_RENDER_SOLID)
                                drawFace = false;
                        }
                    }

                    if (!drawFace)
                        continue;
#pragma endregion
#pragma region Face Creation
                    const FaceTexture_t TEX = pBLOCK->pFACE_TEXTURES[face];
                    const AtlasRegion_t *pATLAS_REGION = &pState->renderer.pAtlasRegions[TEX.atlasIndex];
                    const Vec3f_t BASE_POS = {(float)x, (float)y, (float)z};

                    // Copy per-face vertices
                    for (int v = 0; v < VERTS_PER_FACE; ++v)
                    {
                        ShaderVertexVoxel_t vert = {0};
                        vert.pos = cmath_vec3f_add_vec3f(BASE_POS, pFACE_POSITIONS[face][v]);
                        vert.color = COLOR_WHITE;
                        vert.atlasIndex = TEX.atlasIndex;
                        vert.faceID = face;
                        pVertices[vertexCursor + v] = vert;
                    }

                    assignFaceUVs(pVertices, vertexCursor, pATLAS_REGION, TEX.rotation);

                    pIndices[indexCursor + 0] = (uint32_t)(vertexCursor + 0);
                    pIndices[indexCursor + 1] = (uint32_t)(vertexCursor + 1);
                    pIndices[indexCursor + 2] = (uint32_t)(vertexCursor + 3);
                    pIndices[indexCursor + 3] = (uint32_t)(vertexCursor + 0);
                    pIndices[indexCursor + 4] = (uint32_t)(vertexCursor + 3);
                    pIndices[indexCursor + 5] = (uint32_t)(vertexCursor + 2);

                    vertexCursor += VERTS_PER_FACE;
                    indexCursor += INDICIES_PER_FACE;
                }
            }

    if (indexCursor == 0 || vertexCursor == 0)
    {
        free(pVertices);
        free(pIndices);

        pChunk->pRenderChunk = NULL;
        // Meshing succeeded and nothing to draw (full chunk and surrounded)
        return true;
    }
#pragma endregion
#pragma region RenderChunk
    // Shrink to used size <= max allocation
    ShaderVertexVoxel_t *pFinalVerts = realloc(pVertices, sizeof(ShaderVertexVoxel_t) * vertexCursor);
    uint32_t *pFinalIndices = realloc(pIndices, sizeof(uint32_t) * indexCursor);
    if (!pFinalVerts)
        pFinalVerts = pVertices;
    if (!pFinalIndices)
        pFinalIndices = pIndices;

    vertexBuffer_createFromData_Voxel(pState, pFinalVerts, (uint32_t)vertexCursor);
    indexBuffer_createFromData(pState, pFinalIndices, (uint32_t)indexCursor);

    RenderChunk_t *pRenderChunk = calloc(1, sizeof(RenderChunk_t));
    if (!pRenderChunk)
    {
        free(pFinalVerts);
        free(pFinalIndices);
        free(pChunk->pBlockVoxels);
        free(pChunk);
        return false;
    }

    pRenderChunk->vertexBuffer = pState->renderer.vertexBuffer;
    pRenderChunk->vertexMemory = pState->renderer.vertexBufferMemory;
    pRenderChunk->indexBuffer = pState->renderer.indexBuffer;
    pRenderChunk->indexMemory = pState->renderer.indexBufferMemory;
    pRenderChunk->indexCount = (uint32_t)indexCursor;

    free(pFinalVerts);
    free(pFinalIndices);

    pChunk->pRenderChunk = pRenderChunk;

    Vec3f_t worldPosition = {
        .x = pChunk->chunkPos.x * (float)CHUNK_AXIS_LENGTH,
        .y = pChunk->chunkPos.y * (float)CHUNK_AXIS_LENGTH,
        .z = pChunk->chunkPos.z * (float)CHUNK_AXIS_LENGTH,
    };

    chunk_placeRenderInWorld(pChunk->pRenderChunk, &worldPosition);

    return true;
}
#pragma endregion
#pragma endregion
#pragma region Generate Voxels
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

    // logs_log(LOG_DEBUG, "Noise: %.6f -> idx %u -> %s",
    //          noise, stoneIdx, pBLOCK_NAMES[blockID]);

    return blockID;
}

static bool chunk_generate_voxels(const State_t *pSTATE, Chunk_t *pChunk)
{
    const BlockDefinition_t *const *pBLOCK_DEFINITIONS = block_defs_getAll();

    // fill with stone for now
    uint32_t blockCount = 0;
    for (uint32_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint32_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint32_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                BlockVoxel_t *pBlock = &pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(x, y, z)];

                float n = randomNoise_stoneSampleXYZ(pChunk, (int)x, (int)y, (int)z);
                BlockID_t stoneID = mapNoiseToStone(pSTATE, n);

                pBlock->pBLOCK_DEFINITION = pBLOCK_DEFINITIONS[stoneID];

                // pBlock->pBLOCK_DEFINITION = pBLOCK_DEFINITIONS[random_rangeI32(BLOCK_ID_STONE, BLOCK_ID_COUNT - 1)];

                pBlock->x = (short)x;
                pBlock->y = (short)y;
                pBlock->z = (short)z;

                blockCount++;
            }

    return true;
}
#pragma endregion
#pragma region Allocate Voxels
static Chunk_t *chunk_voxels_allocate(const State_t *pSTATE, const ChunkPos_t CHUNK_POS)
{
    Chunk_t *pChunk = malloc(sizeof(Chunk_t));
    if (!pChunk)
        return NULL;

    pChunk->pBlockVoxels = calloc(CHUNK_BLOCK_CAPACITY, sizeof(BlockVoxel_t));
    pChunk->chunkPos = CHUNK_POS;

    if (!chunk_generate_voxels(pSTATE, pChunk))
        return NULL;

    return pChunk;
}
#pragma endregion
#pragma region Create Chunk
bool chunkManager_chunk_createBatch(State_t *pState, const ChunkPos_t *pCHUNK_POS, const size_t COUNT)
{
    if (!pState || !pCHUNK_POS || COUNT == 0)
        return false;

    Chunk_t **ppChunks = calloc(COUNT, sizeof(Chunk_t *));

    for (size_t i = 0; i < COUNT; i++)
    {
        ppChunks[i] = chunk_voxels_allocate(pState, pCHUNK_POS[i]);
        world_chunk_addToCollection(pState, ppChunks[i]);
    }

    // Blocks generated first so that neighbors can be accessed for mesh generation
    for (size_t i = 0; i < COUNT; i++)
        chunk_mesh_create(pState, ppChunks[i]);

    free(ppChunks);
    return true;
}
#pragma endregion