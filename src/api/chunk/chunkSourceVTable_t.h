#pragma once

struct Chunk_t;
struct ChunkSource_t;

typedef struct ChunkSourceVTable_t
{
    /// @brief Verifies chunk at chunkpos has valid CPU data
    bool (*pLoadChunkFunc)(struct ChunkSource_t *restrict pSource, struct Chunk_t *restrict pChunk);
    /// @brief Called before destroy/unload for the implementation (save to disk, etc.)
    void (*pUnloadChunkFunc)(struct ChunkSource_t *restrict pSource, struct Chunk_t *restrict pChunk);
    /// @brief Per-frame operations like network polling or async I/O progress (NYI) w/ deltaTime (seconds)
    void (*pTickFunc)(struct ChunkSource_t *pSource, double deltaTime);
    /// @brief Destroys the ChunkSource_t itself, freeing any implementation-specific allocations etc
    void (*pDestroyFunc)(struct ChunkSource_t *pSource);
} ChunkSourceVTable_t;