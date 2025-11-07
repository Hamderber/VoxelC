#pragma once

#include <stdint.h>

static const int WORLD_CHUNK_SIM_DIST_MAX = 32;
static const int WORLD_CHUNK_SPAWN_LOAD_RADIUS_MAX = 5;

typedef struct WorldConfig_t
{
    // Radius of the cube that will be permanently loaded at spawn
    uint32_t spawnChunkLoadingRadius;
    // Similar to render distance, but for the cpu side for simulation
    uint32_t chunkSimulationDistance;
} WorldConfig_t;