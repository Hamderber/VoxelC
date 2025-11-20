#pragma once

struct ChunkSourceVTable_t;
struct ChunkManager_t;

typedef struct ChunkSource_t
{
    const struct ChunkManager_t *pCHUNK_MANAGER;
    const struct ChunkSourceVTable_t *pVTABLE;
    /// @brief Implementation-specific data
    void *pImplData;
} ChunkSource_t;