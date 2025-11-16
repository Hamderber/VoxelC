#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include "chunkSource_local.h"
#include "world/worldConfig_t.h"
#pragma endregion
#pragma region Defines
static bool local_loadChunk(ChunkSource_t *restrict pSource, Chunk_t *restrict pChunk);
static void local_unloadChunk(ChunkSource_t *restrict pSource, Chunk_t *restrict pChunk);
static void local_tick(ChunkSource_t *pSource, double deltaTime);
static void local_destroy(ChunkSource_t *pSource);

static const ChunkSourceVTable_t LOCAL_CHUNK_SOURCE_VTABLE = {
    .pLoadChunkFunc = local_loadChunk,
    .pUnloadChunkFunc = local_unloadChunk,
    .pTickFunc = local_tick,
    .pDestroyFunc = local_destroy};
#pragma endregion
#pragma region Operations
ChunkSource_t *chunkSource_createLocal(WorldConfig_t *restrict pWorldCfg, const char *restrict SAVE_DIR)
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

    pSource->pVTABLE = &LOCAL_CHUNK_SOURCE_VTABLE;
    pSource->pImplData = pImplData;

    return pSource;
}
#pragma endregion
#pragma region VTable Functions
static bool local_loadChunk(ChunkSource_t *restrict pSource, Chunk_t *restrict pChunk)
{
    LocalChunkSourceImpl_t *pImplData = (LocalChunkSourceImpl_t *)pSource->pImplData;

    bool tryLoadResult = false;
    logs_log(LOG_DEBUG, "Trying to load chunk (%d, %d, %d) from %s...",
             pChunk->chunkPos.x, pChunk->chunkPos.y, pChunk->chunkPos.z, pImplData->saveDirectory);

    if (tryLoadResult)
    {
        // load
    }
    else
    {
        // generate pChunk
    }
    return true;
}

static void local_unloadChunk(ChunkSource_t *restrict pSource, Chunk_t *restrict pChunk)
{
    LocalChunkSourceImpl_t *pImplData = (LocalChunkSourceImpl_t *)pSource->pImplData;

    // TODO: Actually save this somewhere

    pImplData;
    pChunk;
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