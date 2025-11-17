#pragma region Includes
#pragma once
#include "chunk/chunkManager_t.h"
#pragma endregion
#pragma region ChunkAPI
bool chunkManager_chunks_aquire(ChunkManager_t *restrict pChunkManager, const Vec3i_t *restrict pCHUNK_POS, size_t count,
                                Chunk_t ***restrict pppNewChunks, size_t *restrict pNewCount, Chunk_t ***restrict pppExistingChunks,
                                size_t *restrict pExistingCount);
#pragma endregion
#pragma region Create/Destroy
ChunkManager_t *chunkManager_createNew(State_t *pState);

void chunkManager_destroyNew(State_t *restrict pState, ChunkManager_t *restrict pChunkManager);
#pragma endregion