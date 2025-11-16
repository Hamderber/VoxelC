#pragma once

struct ChunkSourceVTable_t;

typedef struct ChunkSource_t
{
    const struct ChunkSourceVTable_t *pVTABLE;
    /// @brief Implementation-specific data
    void *pImplData;
} ChunkSource_t;