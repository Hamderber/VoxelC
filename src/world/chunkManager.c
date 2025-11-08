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

/// @brief Returns an array (length = CUBE_FACE_COUNT) of pointers to each loaded chunk or null if it is not loaded around
/// neighbor chunk at CHUNK_POS. (Heap)
Chunk_t **chunkManager_getChunkNeighbors(const State_t *pSTATE, const Vec3i_t CHUNK_POS)
{
    Chunk_t **ppChunks = malloc(sizeof(Chunk_t *) * CUBE_FACE_COUNT);
    if (!ppChunks)
        return NULL;

    for (int cubeFace = 0; cubeFace < CUBE_FACE_COUNT; ++cubeFace)
    {
        int nx = CHUNK_POS.x + spNEIGHBOR_OFFSETS[cubeFace].x;
        int ny = CHUNK_POS.y + spNEIGHBOR_OFFSETS[cubeFace].y;
        int nz = CHUNK_POS.z + spNEIGHBOR_OFFSETS[cubeFace].z;

        // logs_log(LOG_DEBUG, "Checking neighbor of chunk (%d, %d, %d) at (%d, %d, %d).",
        //          CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z,
        //          nx, ny, nz);

        Vec3i_t nChunkPos = {nx, ny, nz};
        ppChunks[cubeFace] = chunkManager_getChunk(pSTATE, nChunkPos);
    }

    return ppChunks;
}
#pragma endregion
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
        return (Vec3u8_t){0,
                          ny,
                          nz};

    case CUBE_FACE_LEFT: // nx == -1
        return (Vec3u8_t){(uint8_t)(CHUNK_AXIS_LENGTH - 1),
                          ny,
                          nz};

    case CUBE_FACE_TOP: // ny == CHUNK_AXIS_LENGTH
        return (Vec3u8_t){nx,
                          0,
                          nz};

    case CUBE_FACE_BOTTOM: // ny == -1
        return (Vec3u8_t){nx,
                          (uint8_t)(CHUNK_AXIS_LENGTH - 1),
                          nz};

    case CUBE_FACE_FRONT: // nz == CHUNK_AXIS_LENGTH
        return (Vec3u8_t){nx,
                          ny,
                          0};

    case CUBE_FACE_BACK: // nz == -1
        return (Vec3u8_t){nx,
                          ny,
                          (uint8_t)(CHUNK_AXIS_LENGTH - 1)};
    }

    // This should never happen
    return (Vec3u8_t){0, 0, 0};
}
static bool chunk_mesh_create(State_t *pState, Chunk_t *pChunk)
{
    if (!pState || !pChunk || !pChunk->pBlockVoxels)
        return false;

    Chunk_t **ppNeighbors = chunkManager_getChunkNeighbors(pState, pChunk->chunkPos);

    // max faces in a chunk possible (all transparent faces like glass)
    const uint32_t MAX_VERTEX_COUNT = (uint32_t)CHUNK_BLOCK_CAPACITY * CUBE_FACE_COUNT * VERTS_PER_FACE;
    const uint32_t MAX_INDEX_COUNT = (uint32_t)CHUNK_BLOCK_CAPACITY * CUBE_FACE_COUNT * INDICIES_PER_FACE;

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

    for (uint8_t x = 0; x < CHUNK_AXIS_LENGTH; x++)
        for (uint8_t y = 0; y < CHUNK_AXIS_LENGTH; y++)
            for (uint8_t z = 0; z < CHUNK_AXIS_LENGTH; z++)
            {
                const BlockDefinition_t *pBLOCK = pChunk->pBlockVoxels[xyz_to_chunkBlockIndex(x, y, z)].pBLOCK_DEFINITION;

                if (pBLOCK->BLOCK_ID == BLOCK_ID_AIR)
                    continue;

#pragma region Neighbor Check
                // cube face in this context is the actual block
                for (int face = 0; face < CUBE_FACE_COUNT; ++face)
                {
                    CubeFace_t cubeFace = (CubeFace_t)face;
                    uint8_t nx = x + (uint8_t)spNEIGHBOR_OFFSETS[cubeFace].x;
                    uint8_t ny = y + (uint8_t)spNEIGHBOR_OFFSETS[cubeFace].y;
                    uint8_t nz = z + (uint8_t)spNEIGHBOR_OFFSETS[cubeFace].z;

                    bool neighborInChunk = nx < CHUNK_AXIS_LENGTH &&
                                           ny < CHUNK_AXIS_LENGTH &&
                                           nz < CHUNK_AXIS_LENGTH;

                    // Continue (skip rendering) when the face is determined to not be drawn
                    if (neighborInChunk)
                    {
                        Vec3u8_t neighborPos = {nx, ny, nz};
                        if (chunkManager_getBlockRenderType(pChunk, neighborPos) == BLOCK_RENDER_SOLID)
                            continue;
                    }
                    else
                    {
                        Chunk_t *pNeighborChunk = ppNeighbors[face];
                        if (pNeighborChunk)
                        {
                            Vec3u8_t neighborPos = wrap_to_neighbor_local(nx, ny, nz, cubeFace);
                            // drawing even if neighbor is a block (but transparent)
                            if (chunkManager_getBlockRenderType(pNeighborChunk, neighborPos) == BLOCK_RENDER_SOLID)
                                continue;
                        }
                    }
#pragma endregion
#pragma region Face Creation
                    const FaceTexture_t TEX = pBLOCK->pFACE_TEXTURES[face];
                    const AtlasRegion_t *pATLAS_REGION = &pState->renderer.pAtlasRegions[TEX.atlasIndex];
                    const Vec3i_t BASE_POS = {x, y, z};

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

                    assignFaceUVs(pVertices, vertexCursor, pATLAS_REGION, TEX.rotation);

                    uint32_t base = vertexCursor;
                    write_face_indices_u32(&pIndices[indexCursor], base);

                    vertexCursor += VERTS_PER_FACE;
                    indexCursor += INDICIES_PER_FACE;
                }
            }

    if (indexCursor == 0 || vertexCursor == 0)
    {
        free(pVertices);
        free(pIndices);
        free(ppNeighbors);

        pChunk->pRenderChunk = NULL;
        // Meshing succeeded and nothing to draw (full chunk and surrounded)
        logs_log(LOG_DEBUG, "Render chunk is NULL for chunk at (%d, %d, %d) because it is entirely solid with loaded neighbors. \
(Nothing to render).",
                 pChunk->chunkPos.x, pChunk->chunkPos.y, pChunk->chunkPos.z);
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

    vertexBuffer_createFromData_Voxel(pState, pFinalVerts, vertexCursor);
    indexBuffer_createFromData(pState, pFinalIndices, indexCursor);

    RenderChunk_t *pRenderChunk = malloc(sizeof(RenderChunk_t));
    if (!pRenderChunk)
    {
        free(pFinalVerts);
        free(pFinalIndices);
        free(ppNeighbors);
        return false;
    }

    pRenderChunk->vertexBuffer = pState->renderer.vertexBuffer;
    pRenderChunk->vertexMemory = pState->renderer.vertexBufferMemory;
    pRenderChunk->indexBuffer = pState->renderer.indexBuffer;
    pRenderChunk->indexMemory = pState->renderer.indexBufferMemory;
    pRenderChunk->indexCount = indexCursor;

    free(pFinalVerts);
    free(pFinalIndices);
    free(ppNeighbors);

    pChunk->pRenderChunk = pRenderChunk;

    Vec3i_t worldPos = chunkPos_to_worldOrigin(pChunk->chunkPos);
    Vec3f_t worldPosition = {
        .x = (float)worldPos.x,
        .y = (float)worldPos.y,
        .z = (float)worldPos.z,
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
        Entity_t *pEntity = pLoadingEntity ? pLoadingEntity : pState->pWorldState->pChunkLoadingEntity;
        // const Vec3i_t CHUNK_POS = ppChunks[i]->chunkPos;
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
    logs_log(LOG_DEBUG, "Checking if chunk (%d, %d, %d) is loaded.", CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z);
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