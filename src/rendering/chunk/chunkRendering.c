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
#include "api/chunk/chunkAPI.h"
#include "world/chunkSolidityGrid.h"
#include "world/voxel/block_t.h"

#pragma region Defines
#if defined(DEBUG)
// #define DEBUG_CHUNKRENDER
#endif
#pragma endregion

static const uint32_t MINIMUM_COLLECTION_SIZE = 256;

void chunkRendering_drawChunks(State_t *restrict pState, VkCommandBuffer *restrict pCmd, VkPipelineLayout *restrict pPipelineLayout)
{
    if (!pState || !pState->pWorldState || !pState->pWorldState->pChunkManager->pChunksLL || !pCmd || !pPipelineLayout)
        return;

    LinkedList_t *pCurrent = pState->pWorldState->pChunkManager->pChunksLL;
    while (pCurrent)
    {
        Chunk_t *pChunk = (Chunk_t *)pCurrent->pData;
        pCurrent = pCurrent->pNext;
        if (!pChunk || !chunkState_gpu(pChunk))
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

void chunk_placeRenderInWorld(RenderChunk_t *restrict pChunk, Vec3f_t *restrict pWorldPosition)
{
    pChunk->modelMatrix = cmath_mat_rotate(MAT4_IDENTITY, 0.0F, VEC3F_Y_AXIS);
    pChunk->modelMatrix = cmath_mat_setTranslation(MAT4_IDENTITY, *pWorldPosition);
}

void chunk_render_Destroy(State_t *restrict pState, RenderChunk_t *restrict pRenderChunk)
{
    if (!pRenderChunk)
        return;

    renderGC_pushGarbage(pState->renderer.currentFrame, pRenderChunk->vertexBuffer, pRenderChunk->vertexMemory);
    renderGC_pushGarbage(pState->renderer.currentFrame, pRenderChunk->indexBuffer, pRenderChunk->indexMemory);

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

static bool emit_face(const Vec3u8_t LOCAL_POS, const Chunk_t **restrict ppNeighbors, const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                      const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK, const Chunk_t *restrict pCHUNK,
                      const size_t BLOCK_INDEX, const int FACE)
{
    LOCAL_POS;
    bool result = false;
    do
    {
        const Vec3u8_t N_POS = pNEIGHBOR_BLOCK_POS[cmath_blockNeighborIndex(BLOCK_INDEX, FACE)];
        const bool IN_CHUNK = pNEIGHBOR_BLOCK_IN_CHUNK[cmath_blockNeighborIndex(BLOCK_INDEX, FACE)];
#if defined(DEBUG_CHUNKRENDER)
        if (!IN_CHUNK)
            logs_log(LOG_DEBUG, "Cube face %s of local block (%u, %u, %u) has a neighbor position of (%u, %u, %u)",
                     pCUBE_FACE_NAMES[FACE], LOCAL_POS.x, LOCAL_POS.y, LOCAL_POS.z, N_POS.x, N_POS.y, N_POS.z);
#endif

        if (IN_CHUNK && pCHUNK->pTransparencyGrid)
        {
            if (pCHUNK->pTransparencyGrid->pGrid[chunkSolidityGrid_index16(N_POS.x, N_POS.y, N_POS.z)] != SOLIDITY_TRANSPARENT)
                break;
        }
        else
        {
            const Chunk_t *pN = ppNeighbors[FACE];
#if defined(DEBUG_CHUNKRENDER)
            logs_log(LOG_DEBUG, "Checking face %s of local block (%u, %u, %u). It is outside of the local chunk.",
                     pCUBE_FACE_NAMES[FACE], LOCAL_POS.x, LOCAL_POS.y, LOCAL_POS.z);

#endif
            if (pN && chunkState_cpu(pN) && pN->pTransparencyGrid)
            {
                bool notTransparent = pN->pTransparencyGrid->pGrid[chunkSolidityGrid_index16(N_POS.x, N_POS.y, N_POS.z)] !=
                                      SOLIDITY_TRANSPARENT;
#if defined(DEBUG_CHUNKRENDER)
                Vec3u8_t selfPos = cmath_chunk_blockPosPacked_2_localPos((uint16_t)BLOCK_INDEX);
                Vec3i_t selfChunk = pCHUNK->chunkPos;
                Vec3i_t neighChunk = pN->chunkPos;
                logs_log(LOG_DEBUG,
                         "Boundary check: self chunk (%d,%d,%d) block (%u,%u,%u) face %s -> "
                         "neighbor chunk (%d,%d,%d) local (%u,%u,%u), val=%u",
                         selfChunk.x, selfChunk.y, selfChunk.z,
                         selfPos.x, selfPos.y, selfPos.z,
                         pCUBE_FACE_NAMES[FACE],
                         neighChunk.x, neighChunk.y, neighChunk.z,
                         N_POS.x, N_POS.y, N_POS.z,
                         notTransparent);
#endif
                if (notTransparent)
                    break;
            }
        }

        result = true;
    } while (0);

    return result;
}

static bool chunk_mesh_create(State_t *restrict pState, const Vec3u8_t *restrict pPOINTS, const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                              const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK, Chunk_t *restrict pChunk)
{
    if (!pState || !pState->pWorldState || !pChunk || !pChunk->pBlockVoxels || !pPOINTS || !pNEIGHBOR_BLOCK_POS || !pNEIGHBOR_BLOCK_IN_CHUNK)
        return false;

    Chunk_t **ppNeighbors = chunkManager_getChunkNeighbors(pState, pChunk->chunkPos);
    if (!ppNeighbors)
        return false;

#if defined(DEBUG_CHUNKRENDER)
    for (int face = 0; face < CMATH_GEOM_CUBE_FACES; face++)
    {
        const Chunk_t *pN = ppNeighbors[face];
        if (!pN)
            continue;

        Vec3i_t chunkPos = pChunk->chunkPos;
        Vec3i_t delta = {
            pN->chunkPos.x - chunkPos.x,
            pN->chunkPos.y - chunkPos.y,
            pN->chunkPos.z - chunkPos.z};

        logs_log(LOG_DEBUG,
                 "Chunk (%d,%d,%d): face %s has neighbor (%d, %d, %d) (delta %d, %d, %d)",
                 chunkPos.x, chunkPos.y, chunkPos.z,
                 pCUBE_FACE_NAMES[face],
                 pN->chunkPos.x, pN->chunkPos.y, pN->chunkPos.z,
                 delta.x, delta.y, delta.z);
        if (chunkState_cpu(ppNeighbors[face]))
            logs_log(LOG_DEBUG, "Chunk (%d, %d, %d) is using neighbor (%d, %d, %d) during remesh.",
                     pChunk->chunkPos.x, pChunk->chunkPos.y, pChunk->chunkPos.z,
                     ppNeighbors[face]->chunkPos.x, ppNeighbors[face]->chunkPos.y, ppNeighbors[face]->chunkPos.z);
    }
#endif

    // max faces in a chunk possible (all transparent faces like glass or something)
    const size_t MAX_VERTEX_COUNT = CMATH_CHUNK_BLOCK_CAPACITY * CMATH_GEOM_CUBE_FACES * VERTS_PER_FACE;
    const size_t MAX_INDEX_COUNT = CMATH_CHUNK_BLOCK_CAPACITY * CMATH_GEOM_CUBE_FACES * INDICIES_PER_FACE;

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
            // if (face == 3)
            //     continue;

            if (!emit_face(pPOINTS[i], ppNeighbors, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pChunk, i, face))
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
        if (pChunk->pRenderChunk)
            pChunk->pRenderChunk->indexCount = 0;
        return true;
    }
    // Shrink to used size <= max allocation
    ShaderVertexVoxel_t *pFinalVerts = realloc(pVertices, sizeof(ShaderVertexVoxel_t) * vertexCursor);
    uint32_t *pFinalIndices = realloc(pIndices, sizeof(uint32_t) * indexCursor);
    if (!pFinalVerts)
        pFinalVerts = pVertices;
    if (!pFinalIndices)
        pFinalIndices = pIndices;

    RenderChunk_t *pRenderChunk = pChunk->pRenderChunk;
    if (!pRenderChunk)
    {
        pRenderChunk = malloc(sizeof(RenderChunk_t));
        if (!pRenderChunk)
        {
            free(pFinalVerts);
            free(pFinalIndices);
            free(ppNeighbors);
            return false;
        }

        pRenderChunk->needsRemesh = false;

        memset(pRenderChunk, 0, sizeof(*pRenderChunk));

        uint32_t vCapacity = vertexCursor < MINIMUM_COLLECTION_SIZE ? MINIMUM_COLLECTION_SIZE : vertexCursor;
        uint32_t iCapacity = indexCursor < MINIMUM_COLLECTION_SIZE ? MINIMUM_COLLECTION_SIZE : indexCursor;

        vertexBuffer_createEmpty(pState, vCapacity, &pRenderChunk->vertexBuffer, &pRenderChunk->vertexMemory);
        indexBuffer_createEmpty(pState, iCapacity, &pRenderChunk->indexBuffer, &pRenderChunk->indexMemory);

        pRenderChunk->vertexCapacity = vCapacity;
        pRenderChunk->indexCapacity = iCapacity;

        vertexBuffer_updateFromData_Voxel(pState, pFinalVerts, vertexCursor, pRenderChunk->vertexBuffer);
        indexBuffer_updateFromData_Voxel(pState, pFinalIndices, indexCursor, pRenderChunk->indexBuffer);

        pRenderChunk->indexCount = indexCursor;
        pChunk->pRenderChunk = pRenderChunk;

        Vec3f_t worldPosition = cmath_vec3i_to_vec3f(cmath_chunk_chunkPos_2_worldPosI(pChunk->chunkPos));
        chunk_placeRenderInWorld(pChunk->pRenderChunk, &worldPosition);
    }
    else if (vertexCursor <= pRenderChunk->vertexCapacity && indexCursor <= pRenderChunk->indexCapacity)
    {
        vertexBuffer_updateFromData_Voxel(pState, pFinalVerts, vertexCursor, pRenderChunk->vertexBuffer);
        indexBuffer_updateFromData_Voxel(pState, pFinalIndices, indexCursor, pRenderChunk->indexBuffer);

        pRenderChunk->indexCount = indexCursor;
    }
    else if (vertexCursor > pRenderChunk->vertexCapacity || indexCursor > pRenderChunk->indexCapacity)
    {
        uint32_t newVCap = pRenderChunk->vertexCapacity;
        while (newVCap < vertexCursor)
            newVCap = newVCap ? newVCap * 2 : vertexCursor;

        uint32_t newICap = pRenderChunk->indexCapacity;
        while (newICap < indexCursor)
            newICap = newICap ? newICap * 2 : indexCursor;

        VkBuffer oldVB = pRenderChunk->vertexBuffer;
        VkDeviceMemory oldVM = pRenderChunk->vertexMemory;
        VkBuffer oldIB = pRenderChunk->indexBuffer;
        VkDeviceMemory oldIM = pRenderChunk->indexMemory;

        pRenderChunk->vertexCapacity = newVCap;
        pRenderChunk->indexCapacity = newICap;

        vertexBuffer_createEmpty(pState, newVCap, &pRenderChunk->vertexBuffer, &pRenderChunk->vertexMemory);
        indexBuffer_createEmpty(pState, newICap, &pRenderChunk->indexBuffer, &pRenderChunk->indexMemory);

        pRenderChunk->vertexCapacity = newVCap;
        pRenderChunk->indexCapacity = newICap;

        renderGC_pushGarbage(pState->renderer.currentFrame, oldVB, oldVM);
        renderGC_pushGarbage(pState->renderer.currentFrame, oldIB, oldIM);

        vertexBuffer_updateFromData_Voxel(pState, pFinalVerts, vertexCursor, pRenderChunk->vertexBuffer);
        indexBuffer_updateFromData_Voxel(pState, pFinalIndices, indexCursor, pRenderChunk->indexBuffer);

        pRenderChunk->indexCount = indexCursor;
    }
    else
    {
        // This should never happen
        logs_log(LOG_ERROR, "Reached a theoretically impossible location in createMesh (chunk)!");
        free(ppNeighbors);
        return false;
    }

    free(pFinalVerts);
    free(pFinalIndices);
    free(ppNeighbors);

    return true;
}

bool chunkRenderer_chunk_remesh(State_t *restrict pState, const Vec3u8_t *restrict pPOINTS, const Vec3u8_t *restrict pNEIGHBOR_BLOCK_POS,
                                const bool *restrict pNEIGHBOR_BLOCK_IN_CHUNK, Chunk_t *restrict pChunk)
{
    if (!pState || !pPOINTS || !pNEIGHBOR_BLOCK_POS || !pNEIGHBOR_BLOCK_IN_CHUNK || !pChunk || !chunkState_cpu(pChunk))
        return false;

    bool result = chunk_mesh_create(pState, pPOINTS, pNEIGHBOR_BLOCK_POS, pNEIGHBOR_BLOCK_IN_CHUNK, pChunk);
    // The first time a chunk is meshed, the renderchunk just won't exist
    if (pChunk->pRenderChunk)
        pChunk->pRenderChunk->needsRemesh = false;
    if (!chunkState_gpu(pChunk))
        chunkState_set(pChunk, CHUNK_STATE_CPU_GPU);
    return result;
}
#pragma endregion