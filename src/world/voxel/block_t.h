#pragma once

#include <stdint.h>
#include "world/voxel/cubeFace_t.h"
#include "rendering/types/faceTexture_t.h"

typedef enum
{
    BLOCK_ID_AIR = 0,
    BLOCK_ID_STONE = 1,
    BLOCK_ID_DIRT = 2,
    BLOCK_ID_GRASS = 3,
    BLOCK_ID_OBSIDIAN = 4,
    BLOCK_ID_COUNT
} BlockID_t;

static const char *pBLOCK_NAMES[] = {
    "AIR",
    "BLOCK_STONE",
    "DIRT",
    "GRASS",
    "OBSIDIAN",
};

typedef struct
{
    const BlockID_t BLOCK_ID;
    const FaceTexture_t pFACE_TEXTURES[6];
} BlockDefinition_t;

typedef struct
{
    const BlockDefinition_t *pBLOCK_DEFINITION;
    // 12 bits needed to pack 16x16x16 pos (future)
    short x, y, z;
} BlockVoxel_t;

#pragma region Blocks
static const BlockDefinition_t BLOCK_AIR = {
    .BLOCK_ID = BLOCK_ID_AIR,
    .pFACE_TEXTURES = {
        [FACE_LEFT] = {ATLAS_FACE_AIR, TEX_ROT_0},
        [FACE_RIGHT] = {ATLAS_FACE_AIR, TEX_ROT_0},
        [FACE_TOP] = {ATLAS_FACE_AIR, TEX_ROT_0},
        [FACE_BOTTOM] = {ATLAS_FACE_AIR, TEX_ROT_0},
        [FACE_FRONT] = {ATLAS_FACE_AIR, TEX_ROT_0},
        [FACE_BACK] = {ATLAS_FACE_AIR, TEX_ROT_0},
    }};

static const BlockDefinition_t BLOCK_STONE = {
    .BLOCK_ID = BLOCK_ID_STONE,
    .pFACE_TEXTURES = {
        [FACE_LEFT] = {ATLAS_FACE_STONE, TEX_ROT_0},
        [FACE_RIGHT] = {ATLAS_FACE_STONE, TEX_ROT_0},
        [FACE_TOP] = {ATLAS_FACE_STONE, TEX_ROT_0},
        [FACE_BOTTOM] = {ATLAS_FACE_STONE, TEX_ROT_0},
        [FACE_FRONT] = {ATLAS_FACE_STONE, TEX_ROT_0},
        [FACE_BACK] = {ATLAS_FACE_STONE, TEX_ROT_0},
    }};
#pragma endregion