#pragma once

struct Chunk_t;
struct ChunkSource_t;

typedef struct ChunkSourceVTable_t
{
    /// @brief Verifies chunk at chunkpos has valid CPU data
    bool (*pLoadChunksFunc)(struct ChunkSource_t *restrict pSource, struct Chunk_t **ppChunks, size_t count,
                            Chunk_t ***pppOutChunksBad, size_t *restrict pOutCount);
    /// @brief Called before destroy/unload for the implementation (save to disk, etc.)
    void (*pUnloadChunksFunc)(struct ChunkSource_t *restrict pSource, struct Chunk_t **restrict ppChunks, size_t count);
    /// @brief Per-frame operations like network polling or async I/O progress (NYI) w/ deltaTime (seconds)
    void (*pTickFunc)(struct ChunkSource_t *pSource, double deltaTime);
    /// @brief Destroys the ChunkSource_t itself, freeing any implementation-specific allocations etc
    void (*pDestroyFunc)(struct ChunkSource_t *pSource);
} ChunkSourceVTable_t;