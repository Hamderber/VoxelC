#pragma region Includes
#pragma once
#include "api/chunk/chunkAPI.h"
#pragma endregion
#pragma region Defines
typedef struct LocalChunkSourceImpl_t
{
    struct WorldConfig_t *pWorldCfg;
    const char *saveDirectory;
} LocalChunkSourceImpl_t;
#pragma endregion
#pragma region Operations
ChunkSource_t *chunkSource_createLocal(struct ChunkManager_t *restrict pChunkManager, struct WorldConfig_t *restrict pWorldCfg,
                                       const char *restrict SAVE_DIR);
#pragma endregion