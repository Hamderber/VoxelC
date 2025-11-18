#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include "chunkSource_local.h"
#include "world/worldConfig_t.h"
#include "world/chunkGenerator.h"
#include "chunk.h"
#pragma endregion
#pragma region Defines
static bool local_loadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count);
static void local_unloadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count);
static void local_tick(ChunkSource_t *pSource, double deltaTime);
static void local_destroy(ChunkSource_t *pSource);

static const ChunkSourceVTable_t LOCAL_CHUNK_SOURCE_VTABLE = {
    .pLoadChunksFunc = local_loadChunks,
    .pUnloadChunksFunc = local_unloadChunks,
    .pTickFunc = local_tick,
    .pDestroyFunc = local_destroy};
#pragma endregion
#pragma region Operations
ChunkSource_t *chunkSource_createLocal(ChunkManager_t *restrict pChunkManager, WorldConfig_t *restrict pWorldCfg,
                                       const char *restrict SAVE_DIR)
{
    ChunkSource_t *pSource = malloc(sizeof(ChunkSource_t));
    if (!pSource)
        return NULL;

    LocalChunkSourceImpl_t *pImplData = malloc(sizeof(LocalChunkSourceImpl_t));
    if (!pImplData)
    {
        free(pSource);
        return NULL;
    }

    pImplData->pWorldCfg = pWorldCfg;
    pImplData->saveDirectory = SAVE_DIR;

    pSource->pCHUNK_MANAGER = pChunkManager;
    pSource->pVTABLE = &LOCAL_CHUNK_SOURCE_VTABLE;
    pSource->pImplData = pImplData;

    return pSource;
}
#pragma endregion
#pragma region VTable Functions
static bool local_loadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count)
{
    chunkState_setBatch(ppChunks, count, CHUNK_STATE_CPU_LOADING);

    LocalChunkSourceImpl_t *pImplData = (LocalChunkSourceImpl_t *)pSource->pImplData;

    bool tryLoadResult = false;
    bool inlineFunctionPlaceholder = true;

    // Don't forget about this
    logs_log(LOG_WARN, "Save directory is still NYI!");
    logs_log(LOG_DEBUG, "Trying to load %d chunk(s) from %s with chunk manager %p...", count, pImplData->saveDirectory,
             pSource->pCHUNK_MANAGER);

    for (size_t i = 0; i < count; i++)
    {
        const Vec3i_t CHUNK_POS = ppChunks[i]->chunkPos;
        if (tryLoadResult)
        {
            // load
        }
        else if (inlineFunctionPlaceholder)
        {
            logs_log(LOG_DEBUG, "Generating chunk %p at (%d, %d, %d) with chunk manager %p.",
                     ppChunks[i], CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z, pSource->pCHUNK_MANAGER);
        }
        else
        {
            logs_log(LOG_ERROR, "Failed to load chunk %p at (%d, %d, %d) with chunk manager %p!",
                     ppChunks[i], CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z, pSource->pCHUNK_MANAGER);
        }
    }

    return true;
}

static void local_unloadChunks(ChunkSource_t *restrict pSource, Chunk_t **restrict ppChunks, size_t count)
{
    LocalChunkSourceImpl_t *pImplData = (LocalChunkSourceImpl_t *)pSource->pImplData;

    // TODO: Actually save this somewhere

    count;
    pImplData;
    ppChunks;
}

static void local_tick(ChunkSource_t *pSource, double deltaTime)
{
    pSource;
    deltaTime;
}

static void local_destroy(ChunkSource_t *pSource)
{
    if (!pSource)
        return;

    LocalChunkSourceImpl_t *pImplData = (LocalChunkSourceImpl_t *)pSource->pImplData;
    free(pImplData);
    free(pSource);
}
#pragma endregion