#include <vulkan/vulkan.h>
#include "cmath/cmath.h"
#include <stdlib.h>
#include "core/types/state_t.h"
#include "rendering/types/renderChunk_t.h"
#include "collection/linkedList_t.h"
#include "rendering/types/shaderVertexVoxel_t.h"
#include "world/chunkManager.h"
#include "rendering/buffers/index_buffer.h"
#include "rendering/buffers/vertex_buffer.h"
#include "rendering/uvs.h"
#include "rendering/chunk/chunkRenderer.h"
#include "rendering/renderGC.h"

void chunkRendering_drawChunks(State_t *pState, VkCommandBuffer *pCmd, VkPipelineLayout *pPipelineLayout)
{
    if (!pState || !pState->pWorldState || !pState->pWorldState->pChunksLL || !pCmd || !pPipelineLayout)
        return;

    LinkedList_t *pCurrent = pState->pWorldState->pChunksLL;
    while (pCurrent)
    {
        Chunk_t *pChunk = (Chunk_t *)pCurrent->pData;
        pCurrent = pCurrent->pNext;
        if (!pChunk)
            continue;

        RenderChunk_t *pRenderChunk = pChunk->pRenderChunk;

        // A solid chunk surrounded by solid blocks will have no verticies to draw and will thus have the whole renderchunk be null
        if (!pRenderChunk || pRenderChunk->indexCount == 0)
            continue;

        VkBuffer chunkVB[] = {pRenderChunk->vertexBuffer};
        VkDeviceSize offs[] = {0};
        vkCmdBindVertexBuffers(*pCmd, 0, 1, chunkVB, offs);
        vkCmdBindIndexBuffer(*pCmd, pRenderChunk->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdPushConstants(*pCmd, *pPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, (uint32_t)sizeof(Mat4c_t), &pRenderChunk->modelMatrix);
        vkCmdDrawIndexed(*pCmd, pRenderChunk->indexCount, 1, 0, 0, 0);
    }
}

void chunk_placeRenderInWorld(RenderChunk_t *chunk, Vec3f_t *position)
{
    chunk->modelMatrix = cmath_mat_rotate(MAT4_IDENTITY, 0.0F, VEC3F_Y_AXIS);
    chunk->modelMatrix = cmath_mat_setTranslation(MAT4_IDENTITY, *position);
}

static size_t destroyed = 0;
void chunk_renderDestroy(State_t *pState, RenderChunk_t *pRenderChunk)
{
    if (!pRenderChunk)
        return;

    renderGC_pushGarbage(pState->renderer.currentFrame, pRenderChunk->vertexBuffer, pRenderChunk->vertexMemory);
    renderGC_pushGarbage(pState->renderer.currentFrame, pRenderChunk->indexBuffer, pRenderChunk->indexMemory);

    destroyed++;
    free(pRenderChunk);
}

#pragma region Create Mesh
static const uint8_t pCCW_QUAD_VERTS[6] = {0, 1, 3, 0, 3, 2};

static inline void write_face_indices_u32(uint32_t *pIndicies, uint32_t base)
{
    pIndicies[0] = base + pCCW_QUAD_VERTS[0];
    pIndicies[1] = base + pCCW_QUAD_VERTS[1];
    pIndicies[2] = base + pCCW_QUAD_VERTS[2];
    pIndicies[3] = base + pCCW_QUAD_VERTS[3];
    pIndicies[4] = base + pCCW_QUAD_VERTS[4];
    pIndicies[5] = base + pCCW_QUAD_VERTS[5];
}

static inline Vec3u8_t wrap_to_neighbor_local(uint8_t nx, uint8_t ny, uint8_t nz, CubeFace_t face)
{
    switch (face)
    {
    case CUBE_FACE_RIGHT: // nx == CHUNK_AXIS_LENGTH
        return (Vec3u8_t){0, ny, nz};

    case CUBE_FACE_LEFT: // nx == -1
        return (Vec3u8_t){(uint8_t)(CHUNK_AXIS_LENGTH - 1), ny, nz};

    case CUBE_FACE_TOP: // ny == CHUNK_AXIS_LENGTH
        return (Vec3u8_t){nx, 0, nz};

    case CUBE_FACE_BOTTOM: // ny == -1
        return (Vec3u8_t){nx, (uint8_t)(CHUNK_AXIS_LENGTH - 1), nz};

    case CUBE_FACE_FRONT: // nz == CHUNK_AXIS_LENGTH
        return (Vec3u8_t){nx, ny, 0};

    case CUBE_FACE_BACK: // nz == -1
        return (Vec3u8_t){nx, ny, (uint8_t)(CHUNK_AXIS_LENGTH - 1)};
    }

    // This should never happen
    return (Vec3u8_t){0, 0, 0};
}

static bool emit_face(const Chunk_t **restrict ppNeighbors, const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                      const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK, const Chunk_t *restrict pCHUNK,
                      const size_t BLOCK_INDEX, const int FACE)
{
    bool result = false;
    do
    {
        const Vec3u8_t N_POS = pNEIGHBOR_BLOCK_POS[cmath_blockNeighborIndex(BLOCK_INDEX, FACE)];
        const bool IN_CHUNK = pNEIGHBOR_BLOCK_IN_CHUNK[cmath_blockNeighborIndex(BLOCK_INDEX, FACE)];

        if (IN_CHUNK && pCHUNK->pTransparencyGrid)
        {
            if (pCHUNK->pTransparencyGrid->pGrid[chunkSolidityGrid_index16(N_POS.x, N_POS.y, N_POS.z)] != SOLIDITY_TRANSPARENT)
                break;
        }
        else
        {
            const Chunk_t *pN = ppNeighbors[FACE];
            if (pN && pN->pTransparencyGrid)
            {
                const Vec3u8_t POS_WRAP = wrap_to_neighbor_local(N_POS.x, N_POS.y, N_POS.z, (CubeFace_t)FACE);
                if (pN->pTransparencyGrid->pGrid[chunkSolidityGrid_index16(POS_WRAP.x, POS_WRAP.y, POS_WRAP.z)] != SOLIDITY_TRANSPARENT)
                    break;
            }
        }

        result = true;
    } while (0);

    return result;
}

static size_t created = 0;
bool chunk_mesh_create(State_t *restrict pState, const Vec3u8_t *restrict pPOINTS, const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                       const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK, Chunk_t *restrict pChunk)
{
    if (!pState || !pState->pWorldState || !pChunk || !pChunk->pBlockVoxels || !pPOINTS || !pNEIGHBOR_BLOCK_POS || !pNEIGHBOR_BLOCK_IN_CHUNK)
        return false;

    // Destroy previous renderer to avoid dangling pointers etc.
    if (pChunk->pRenderChunk)
        chunk_renderDestroy(pState, pChunk->pRenderChunk);

    Chunk_t **ppNeighbors = chunkManager_getChunkNeighbors(pState, pChunk->chunkPos);
    if (!ppNeighbors)
        return false;

    // max faces in a chunk possible (all transparent faces like glass or something)
    const size_t MAX_VERTEX_COUNT = CHUNK_BLOCK_CAPACITY * CUBE_FACE_COUNT * VERTS_PER_FACE;
    const size_t MAX_INDEX_COUNT = CHUNK_BLOCK_CAPACITY * CUBE_FACE_COUNT * INDICIES_PER_FACE;

    ShaderVertexVoxel_t *pVertices = malloc(sizeof(ShaderVertexVoxel_t) * MAX_VERTEX_COUNT);
    uint32_t *pIndices = malloc(sizeof(uint32_t) * MAX_INDEX_COUNT);
    if (!pVertices || !pIndices)
    {
        free(pVertices);
        free(pIndices);
        free(ppNeighbors);
        return false;
    }

    uint32_t vertexCursor = 0;
    uint32_t indexCursor = 0;

    for (size_t i = 0; i < CMATH_CHUNK_POINTS_COUNT; i++)
    {
        const BlockDefinition_t *pBLOCK = pChunk->pBlockVoxels[i].pBLOCK_DEFINITION;

        if (pBLOCK->BLOCK_ID == BLOCK_ID_AIR)
            continue;

        for (int face = 0; face < 6; ++face)
        {
            if (!emit_face(ppNeighbors, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pChunk, i, face))
                continue;

            const FaceTexture_t TEX = pBLOCK->pFACE_TEXTURES[face];
            const AtlasRegion_t *pATLAS_REGION = &pState->renderer.pAtlasRegions[TEX.atlasIndex];
            const Vec3i_t BASE_POS = cmath_vec3u8_to_vec3i(pPOINTS[i]);

            // Copy per-face vertices
            for (int v = 0; v < VERTS_PER_FACE; ++v)
            {
                ShaderVertexVoxel_t vert = {0};
                vert.pos = cmath_vec3i_to_vec3f(cmath_vec3i_add_vec3i(BASE_POS, pFACE_POSITIONS[face][v]));
                vert.color = COLOR_WHITE;
                vert.atlasIndex = TEX.atlasIndex;
                vert.faceID = face;
                pVertices[vertexCursor + v] = vert;
            }

            uvs_voxel_assignFaceUVs(pVertices, vertexCursor, pATLAS_REGION, TEX.rotation);

            write_face_indices_u32(&pIndices[indexCursor], vertexCursor);

            vertexCursor += VERTS_PER_FACE;
            indexCursor += INDICIES_PER_FACE;
        }
    }

    if (indexCursor == 0 || vertexCursor == 0)
    {
        free(pVertices);
        free(pIndices);
        free(ppNeighbors);

        // Meshing succeeded and nothing to draw (full chunk and surrounded)
        pChunk->pRenderChunk = NULL;
        return true;
    }
    // Shrink to used size <= max allocation
    ShaderVertexVoxel_t *pFinalVerts = realloc(pVertices, sizeof(ShaderVertexVoxel_t) * vertexCursor);
    uint32_t *pFinalIndices = realloc(pIndices, sizeof(uint32_t) * indexCursor);
    if (!pFinalVerts)
        pFinalVerts = pVertices;
    if (!pFinalIndices)
        pFinalIndices = pIndices;

    RenderChunk_t *pRenderChunk = malloc(sizeof(RenderChunk_t));
    pRenderChunk->queuedForRemesh = false;
    if (!pRenderChunk)
    {
        free(pFinalVerts);
        free(pFinalIndices);
        free(ppNeighbors);
        return false;
    }

    vertexBuffer_createFromData_Voxel(pState, pFinalVerts, vertexCursor,
                                      &pRenderChunk->vertexBuffer,
                                      &pRenderChunk->vertexMemory);

    indexBuffer_createFromData_Voxel(pState, pFinalIndices, indexCursor,
                                     &pRenderChunk->indexBuffer,
                                     &pRenderChunk->indexMemory);

    pRenderChunk->indexCount = indexCursor;

    free(pFinalVerts);
    free(pFinalIndices);

    pChunk->pRenderChunk = pRenderChunk;

    Vec3f_t worldPosition = cmath_vec3i_to_vec3f(chunkPos_to_worldOrigin(pChunk->chunkPos));
    chunk_placeRenderInWorld(pChunk->pRenderChunk, &worldPosition);

    created++;

    // Dirty loaded neighbors to update their mesh. This allows for updating chunk boundaries
    ChunkRemeshCtx_t *pCtx = remeshContext_create(pChunk, ppNeighbors);
    if (!pCtx || !chunkRenderer_enqueueRemesh(pState->pWorldState, pCtx))
        return false;

    free(ppNeighbors);

    return true;
}
#pragma endregion
#pragma region Const/Destructor
void chunkRendering_debug(void)
{
    logs_log(LOG_DEBUG, "Meshes created: %d, Destroyed: %d", created, destroyed);
}
#pragma endregion