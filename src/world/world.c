#include <string.h>
#include <stdlib.h>
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/buffers/index_buffer.h"
#include "core/logs.h"
#include "core/types/state_t.h"
#include "world/world_t.h"
#include "character/characterType_t.h"
#include "character/character.h"
#include "rendering/chunk/chunkRendering.h"
#include "core/types/atlasRegion_t.h"
#include "core/types/atlasFace_t.h"
#include "rendering/types/faceTexture_t.h"
#include "rendering/uvs.h"
#include "chunk.h"
#include "rendering/chunk/chunkRendering.h"
#include "world/voxel/cubeFace_t.h"
#include "rendering/types/shaderVertexVoxel_t.h"

static inline uint32_t xyz_to_index(int x, int y, int z) { return x * CHUNK_AXIS_LENGTH * CHUNK_AXIS_LENGTH + y * CHUNK_AXIS_LENGTH + z; }

static Chunk_t *world_dummyChunkCreate(State_t *state, ChunkPos_t coord)
{
    Chunk_t *pChunk = malloc(sizeof(Chunk_t));
    if (!pChunk)
        return NULL;

    pChunk->pBlockVoxels = calloc(CHUNK_BLOCK_CAPACITY, sizeof(BlockVoxel_t));
    pChunk->coordinate = coord;

    // fill with stone for now
    uint32_t blockCount = 0;
    for (uint32_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
    {
        for (uint32_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
        {
            for (uint32_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                BlockVoxel_t *pBlock = &pChunk->pBlockVoxels[xyz_to_index(x, y, z)];

                pBlock->pBLOCK_DEFINITION = blockCount % 2 == 0 ? &BLOCK_STONE : &BLOCK_AIR;
                pBlock->x = (short)x;
                pBlock->y = (short)y;
                pBlock->z = (short)z;

                blockCount++;
            }
            blockCount++;
        }
        blockCount++;
    }

    // max faces in a chunk possible (all transparent cubes)
    const size_t maxVertexCount = (size_t)CHUNK_BLOCK_CAPACITY * FACE_COUNT * VERTS_PER_FACE;
    const size_t maxIndexCount = (size_t)CHUNK_BLOCK_CAPACITY * FACE_COUNT * INDICIES_PER_FACE;

    ShaderVertexVoxel_t *pVertices = calloc(maxVertexCount, sizeof(ShaderVertexVoxel_t));
    uint32_t *pIndices = calloc(maxIndexCount, sizeof(uint32_t));
    if (!pVertices || !pIndices)
    {
        free(pVertices);
        free(pIndices);
        free(pChunk->pBlockVoxels);
        free(pChunk);
        return NULL;
    }

    size_t vertexCursor = 0;
    size_t indexCursor = 0;

    // const Vec3i_t pNEIGHBOR_OFFSETS[6] = {{-1, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};

    for (uint32_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint32_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint32_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                const BlockDefinition_t *pBLOCK = pChunk->pBlockVoxels[xyz_to_index(x, y, z)].pBLOCK_DEFINITION;
                if (pBLOCK->BLOCK_ID == BLOCK_ID_AIR)
                    continue;

                for (int face = 0; face < FACE_COUNT; ++face)
                {
                    // CubeFace_t cubeFace = (CubeFace_t)face;
                    // int nx = (int)x + pNEIGHBOR_OFFSETS[cubeFace].x;
                    // int ny = (int)y + pNEIGHBOR_OFFSETS[cubeFace].y;
                    // int nz = (int)z + pNEIGHBOR_OFFSETS[cubeFace].z;

                    // bool faceVisible =
                    //     (nx < 0 || nx >= (int)CHUNK_AXIS_LENGTH ||
                    //      ny < 0 || ny >= (int)CHUNK_AXIS_LENGTH ||
                    //      nz < 0 || nz >= (int)CHUNK_AXIS_LENGTH);

                    // if (!faceVisible)
                    //     continue;

                    const FaceTexture_t TEX = pBLOCK->pFACE_TEXTURES[face];
                    const AtlasRegion_t *pATLAS_REGION = &state->renderer.pAtlasRegions[TEX.atlasIndex];
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

                    // Indices
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

    // Shrink to used size
    ShaderVertexVoxel_t *finalVerts = realloc(pVertices, vertexCursor * sizeof(ShaderVertexVoxel_t));
    uint32_t *finalIndices = realloc(pIndices, indexCursor * sizeof(uint32_t));
    if (!finalVerts)
        finalVerts = pVertices;
    if (!finalIndices)
        finalIndices = pIndices;

    vertexBuffer_createFromData_Voxel(state, finalVerts, (uint32_t)vertexCursor);
    indexBuffer_createFromData(state, finalIndices, (uint32_t)indexCursor);

    RenderChunk_t *pRenderChunk = calloc(1, sizeof(RenderChunk_t));
    if (!pRenderChunk)
    {
        free(finalVerts);
        free(finalIndices);
        free(pChunk->pBlockVoxels);
        free(pChunk);
        return NULL;
    }

    pRenderChunk->vertexBuffer = state->renderer.vertexBuffer;
    pRenderChunk->vertexMemory = state->renderer.vertexBufferMemory;
    pRenderChunk->indexBuffer = state->renderer.indexBuffer;
    pRenderChunk->indexMemory = state->renderer.indexBufferMemory;
    pRenderChunk->indexCount = (uint32_t)indexCursor;

    free(finalVerts);
    free(finalIndices);

    pChunk->pRenderChunk = pRenderChunk;

    Vec3f_t worldPosition = {
        .x = coord.x * (float)CHUNK_AXIS_LENGTH,
        .y = coord.y * (float)CHUNK_AXIS_LENGTH,
        .z = coord.z * (float)CHUNK_AXIS_LENGTH,
    };

    chunk_placeRenderInWorld(pRenderChunk, &worldPosition);
    return pChunk;
}

void world_init(State_t *state)
{
    state->pWorldState = calloc(1, sizeof(WorldState_t));

    state->pWorldState->world.pPlayer = character_create(state, CHARACTER_TYPE_PLAYER);

    int chunksPerAxis = 3;
    state->pWorldState->chunkCount = chunksPerAxis * chunksPerAxis * chunksPerAxis;
    state->pWorldState->ppChunks = calloc(state->pWorldState->chunkCount, sizeof(Chunk_t *));

    for (int i = 0; i < chunksPerAxis; i++)
        for (int j = 0; j < chunksPerAxis; j++)
            for (int k = 0; k < chunksPerAxis; k++)
            {
                size_t idx = (size_t)i * chunksPerAxis * chunksPerAxis + (size_t)j * chunksPerAxis + (size_t)k;
                ChunkPos_t pos = {i, j, k};
                state->pWorldState->ppChunks[idx] = world_dummyChunkCreate(state, pos);
                logs_log(LOG_INFO, "Dummy voxel chunk created at (%f, %f, %f).", pos.x, pos.y, pos.z);
            }
}

void world_load(State_t *state)
{
    world_init(state);
}

void world_destroy(State_t *state)
{
    if (!state || !state->pWorldState || !state->pWorldState->ppChunks)
        return;

    // Ensure nothing is in-flight that still uses these buffers
    vkDeviceWaitIdle(state->context.device);

    size_t count = state->pWorldState->chunkCount;
    for (size_t i = 0; i < count; ++i)
    {
        RenderChunk_t *chunk = state->pWorldState->ppChunks[i]->pRenderChunk;
        if (!chunk)
            continue;

        chunk_renderDestroy(state, chunk);
        state->pWorldState->ppChunks[i] = NULL;
    }

    // Free the container array and reset counters
    free(state->pWorldState->ppChunks);
    state->pWorldState->ppChunks = NULL;
    state->pWorldState->chunkCount = 0;
}