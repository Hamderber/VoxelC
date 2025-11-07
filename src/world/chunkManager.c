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
#include "events/eventBus.h"
#pragma endregion
#pragma region Get Chunk
Chunk_t *chunkManager_getChunk(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    if (!pSTATE || !pSTATE->pWorldState)
    {
        logs_log(LOG_ERROR, "getChunk: bad state");
        return NULL;
    }
    if (!pSTATE->pWorldState->pChunksLL)
    {
        logs_log(LOG_ERROR, "getChunk: chunk list is empty");
        return NULL;
    }

    for (LinkedList_t *pNode = pSTATE->pWorldState->pChunksLL; pNode; pNode = pNode->pNext)
    {
        if (!pNode->pData)
            continue;
        const Chunk_t *pC = (const Chunk_t *)pNode->pData;
        if (cmath_vec3i_equals(pC->chunkPos, CHUNK_POS, 0))
            return (Chunk_t *)pC;
    }
    return NULL;
}
#pragma endregion
#pragma region Create Mesh
static bool chunk_mesh_create(State_t *pState, Chunk_t *pChunk)
{
    // TODO: Make neighbor check get the chunk adjacencies only once instead of per block (huge performance boost)
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

    for (uint8_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint8_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint8_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
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
                        neighbor = chunkManager_getBlock(pChunk, (Vec3u8_t){(uint8_t)nx, (uint8_t)ny, (uint8_t)nz});
                        if (neighbor.pBLOCK_DEFINITION->BLOCK_RENDER_TYPE == BLOCK_RENDER_SOLID)
                            drawFace = false;
                    }
                    else
                    {
                        // Determine which adjacent chunk this face crosses into
                        Vec3i_t neighborChunkPos = pChunk->chunkPos;
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
                        logs_log(LOG_DEBUG, "Checking neighbor of chunk (%d, %d, %d) at (%d, %d, %d).",
                                 pChunk->chunkPos.x, pChunk->chunkPos.y, pChunk->chunkPos.z,
                                 neighborChunkPos.x, neighborChunkPos.y, neighborChunkPos.z);
                        Chunk_t *pNeighborChunk = chunkManager_getChunk(pState, neighborChunkPos);
                        if (pNeighborChunk)
                        {
                            // Prevent out of bounds
                            Vec3u8_t localPos = {
                                (uint8_t)(nx + CHUNK_AXIS_LENGTH) % CHUNK_AXIS_LENGTH,
                                (uint8_t)(ny + CHUNK_AXIS_LENGTH) % CHUNK_AXIS_LENGTH,
                                (uint8_t)(nz + CHUNK_AXIS_LENGTH) % CHUNK_AXIS_LENGTH};

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
#pragma region Allocate Voxels
static Chunk_t *chunk_voxels_allocate(const State_t *pSTATE, const BlockDefinition_t *const *pBLOCK_DEFINITIONS, const Vec3i_t CHUNK_POS)
{
    Chunk_t *pChunk = malloc(sizeof(Chunk_t));
    if (!pChunk)
        return NULL;

    pChunk->pBlockVoxels = calloc(CHUNK_BLOCK_CAPACITY, sizeof(BlockVoxel_t));
    pChunk->chunkPos = CHUNK_POS;

    if (!chunkGen_genChunk(pSTATE, pBLOCK_DEFINITIONS, pChunk))
        return NULL;

    return pChunk;
}
#pragma endregion
#pragma region Create Chunk
/// @brief Iterates the provided chunk positions and returns a heap array of non-loaded chunks positions from that collection.
/// Places the size of that collection into pSize
static Vec3i_t *chunks_getUnloadedPos(State_t *pState, const Vec3i_t *pCHUNK_POS, size_t *pSize)
{
    size_t index = 0;
    Vec3i_t *pPos = calloc(*pSize, sizeof(Vec3i_t));
    for (size_t i = 0; i < *pSize; i++)
    {
        if (!chunk_isLoaded(pState, pCHUNK_POS[i]))
            pPos[index++] = pCHUNK_POS[i];
    }

    pPos = realloc(pPos, sizeof(Vec3i_t) * index);
    *pSize = index;
    return pPos;
}

bool chunkManager_chunk_createBatch(State_t *pState, const Vec3i_t *pCHUNK_POS, size_t count, Entity_t *pLoadingEntity)
{
    if (!pState || !pCHUNK_POS || count == 0)
        return false;

    Vec3i_t *pPoints = chunks_getUnloadedPos(pState, pCHUNK_POS, &count);

    Chunk_t **ppChunks = calloc(count, sizeof(Chunk_t *));
    const BlockDefinition_t *const *pBLOCK_DEFINITIONS = block_defs_getAll();

    for (size_t i = 0; i < count; i++)
    {
        ppChunks[i] = chunk_voxels_allocate(pState, pBLOCK_DEFINITIONS, pPoints[i]);
        world_chunk_addToCollection(pState, ppChunks[i]);
    }

    for (size_t i = 0; i < count; i++)
    {
        // default player is loading the chunk
        ppChunks[i]->pEntitiesLoadingChunkLL = calloc(1, sizeof(LinkedList_t));
        const Vec3i_t CHUNK_POS = ppChunks[i]->chunkPos;
        Entity_t *pEntity = pLoadingEntity ? pLoadingEntity : pState->pWorldState->pChunkLoadingEntity;
        // logs_log(LOG_DEBUG, "Chunk (%d, %d, %d) is %sloaded. (Loading entity %p)",
        //          CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z,
        //          pLoadingEntity ? "" : "permanently ");

        linkedList_data_add(&ppChunks[i]->pEntitiesLoadingChunkLL,
                            // If no loading entity is passed, permanently chunk load it
                            (void *)(pEntity));
    }

    // Blocks generated first so that neighbors can be accessed for mesh generation
    for (size_t i = 0; i < count; i++)
        chunk_mesh_create(pState, ppChunks[i]);

    free(pPoints);
    return true;
}
#pragma endregion
#pragma region Destroy Chunk
void chunk_linkedList_destroy(Chunk_t *pChunk)
{
    if (!pChunk)
        return;

    LinkedList_t *pRoot = pChunk->pEntitiesLoadingChunkLL;
    pChunk->pEntitiesLoadingChunkLL = NULL;

    while (pRoot)
    {
        LinkedList_t *pNode = pRoot;
        pRoot = pRoot->pNext;
        pNode->pNext = NULL;
        free(pNode);
    }
}

void chunk_destroy(State_t *pState, Chunk_t *pChunk)
{
    if (!pState || !pChunk)
        return;

    free(pChunk->pBlockVoxels);
    chunk_linkedList_destroy(pChunk);
    chunk_renderDestroy(pState, pChunk->pRenderChunk);
}

void chunkManager_linkedList_destroy(State_t *pState, LinkedList_t **ppChunksLL)
{
    if (!ppChunksLL || !*ppChunksLL)
        return;

    LinkedList_t *pCurrent = *ppChunksLL;
    while (pCurrent)
    {
        LinkedList_t *pNext = pCurrent->pNext;
        chunk_destroy(pState, (Chunk_t *)pCurrent->pData);
        free(pCurrent);
        pCurrent = pNext;
    }

    *ppChunksLL = NULL;
}
#pragma endregion
#pragma region Chunk (Un)Load
bool chunk_isLoaded(State_t *pState, const Vec3i_t CHUNK_POS)
{
    return chunkManager_getChunk(pState, CHUNK_POS) != NULL;
}

EventResult_t player_onChunkChange(State_t *pState, Event_t *pEvent, void *pCtx)
{
    pCtx;

    if (!pState || !pEvent)
        return EVENT_RESULT_ERROR;

    CtxChunk_t *pChunkEventData = pEvent->data.pChunkEvntData;
    Entity_t *pEntity = pChunkEventData->pEntitySource;
    Chunk_t *pChunk = pChunkEventData->pChunk;
    if (!pChunk)
        return EVENT_RESULT_ERROR;
    Vec3i_t chunkPos = pChunk->chunkPos;

    logs_log(LOG_DEBUG, "Entity %p is now in chunk (%d, %d, %d).", pEntity, chunkPos.x, chunkPos.y, chunkPos.z);

    return EVENT_RESULT_PASS;
}
#pragma endregion
#pragma region Create
void chunkManager_create(State_t *pState)
{
    const void *pSUB_CTX = NULL;
    static const bool CONSUME_LISTENER = false;
    static const bool CONSUME_EVENT = false;
    events_subscribe(&pState->eventBus, EVENT_CHANNEL_CHUNK, player_onChunkChange, CONSUME_LISTENER, CONSUME_EVENT, &pSUB_CTX);
}
#pragma endregion
#pragma region Destroy
void chunkManager_destroy(State_t *pState)
{
    events_unsubscribe(&pState->eventBus, EVENT_CHANNEL_CHUNK, player_onChunkChange);
}
#pragma endregion