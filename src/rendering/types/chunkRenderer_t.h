#pragma region Includes
#pragma once
#include "collection/dynamicStack_t.h"
#pragma endregion
#pragma region Defines
typedef struct ChunkRenderer_t
{
    DynamicStack_t *pRemeshCtxQueue;
} ChunkRenderer_t;
#pragma endregion