#pragma region Includes
#include <stdlib.h>
#include <string.h>
#include "chunkSource_network.h"
#pragma endregion
static bool network_loadChunk(ChunkSource_t *restrict pSource, Chunk_t *restrict pChunk);
static void network_unloadChunk(ChunkSource_t *restrict pSource, Chunk_t *restrict pChunk);
static void network_tick(ChunkSource_t *pSource, double deltaTime);
static void network_destroy(ChunkSource_t *pSource);

// NYI