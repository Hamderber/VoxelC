#pragma region Includes
#pragma once
#include "core/logs.h"
#pragma endregion
#pragma region Defines
#define DEBUG_CHUNKSTATE

typedef enum ChunkState_e
{
    // Unloaded chunk, such as invalid data or just not loaded yet
    CHUNK_STATE_UNLOADED,
    // Chunk has been allocated but hasn't been assigned any actual data yet
    CHUNK_STATE_CPU_EMPTY,
    // Chunk is actively being created/loaded from some source (async?)
    CHUNK_STATE_CPU_LOADING,
    // CPU-only, so world simulation with no rendering
    CHUNK_STATE_CPU_ONLY,
    // Fully loaded chunk that is both simulated and rendered
    CHUNK_STATE_CPU_GPU,
    CHUNK_STATE_COUNT,
} ChunkState_e;

#if defined(DEBUG_CHUNKSTATE)
static const char *pCHUNK_STATE_NAMES[] = {
    "CHUNK_STATE_UNLOADED",
    "CHUNK_STATE_CPU_EMPTY",
    "CHUNK_STATE_CPU_LOADING",
    "CHUNK_STATE_CPU_ONLY",
    "CHUNK_STATE_CPU_GPU",
};
#endif
#pragma endregion
#pragma region Operations
static inline bool chunkState_set(Chunk_t *pChunk, ChunkState_e state)
{
    if (!pChunk)
        return false;
#if defined(DEBUG_CHUNKSTATE)
    const Vec3i_t CHUNK_POS = pChunk->chunkPos;
    logs_log(LOG_DEBUG, "Chunk %p at (%d, %d, %d) state %s -> %s.", pChunk, CHUNK_POS.x, CHUNK_POS.y, CHUNK_POS.z,
             pCHUNK_STATE_NAMES[pChunk->chunkState], pCHUNK_STATE_NAMES[state]);
#endif
    pChunk->chunkState = state;
    return true;
}

static inline bool chunkState_setBatch(Chunk_t **ppChunks, size_t count, ChunkState_e state)
{
    if (!ppChunks)
        return false;

    for (size_t i = 0; i < count; i++)
        chunkState_set(ppChunks[i], state);

    return true;
}

static inline bool chunkState_cpu(Chunk_t *pChunk)
{
    ChunkState_e state = pChunk->chunkState;
    return state == CHUNK_STATE_CPU_EMPTY || state == CHUNK_STATE_CPU_LOADING || state == CHUNK_STATE_CPU_ONLY;
}

static inline bool chunkState_gpu(Chunk_t *pChunk)
{
    ChunkState_e state = pChunk->chunkState;
    return state == CHUNK_STATE_CPU_GPU;
}
#pragma endregion
#pragma region Undefines
#undef DEBUG_CHUNKSTATE
#pragma endregion