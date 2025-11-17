#pragma region Includes
#pragma once
#include "chunk/chunkManager_t.h"
#pragma endregion
#pragma region Operations
ChunkManager_t *chunkManager_createNew(State_t *pState);

void chunkManager_destroyNew(State_t *pState, ChunkManager_t *pChunkManager);
#pragma endregion