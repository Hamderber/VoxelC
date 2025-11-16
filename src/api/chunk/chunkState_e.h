#pragma once

typedef enum ChunkState_e
{
    // Unloaded chunk, such as invalid data or just not loaded yet
    CHUNK_STATE_UNLOADED,
    // CPU-only, so world simulation with no rendering
    CHUNK_STATE_CPU,
    // Fully loaded chunk that is both simulated and rendered
    CHUNK_STATE_CPU_GPU,
    // Rendered but non-simulated chunk (unlikely to be used for a while)
    CHUNK_STATE_GPU,
    CHUNK_STATE_COUNT,
} ChunkState_e;