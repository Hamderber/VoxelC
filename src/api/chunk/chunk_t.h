#pragma once
#include "cmath/cmath.h"

struct RenderChunk_t;
struct BlockVoxel_t;
struct LinkedList_t;
struct ChunkSolidityGrid_t;

typedef struct Chunk_t
{
    struct RenderChunk_t *pRenderChunk;
    struct BlockVoxel_t *pBlockVoxels;
    struct Vec3i_t chunkPos;
    struct LinkedList_t *pEntitiesLoadingChunkLL;
    // This is stored in the chunk itself instead of the render chunk so that lighting calculations and such can be done
    // separate from rendering
    struct ChunkSolidityGrid_t *pTransparencyGrid;
} Chunk_t;