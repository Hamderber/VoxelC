#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/logs.h"
#include "c_math/c_math.h"
#include "rendering/shaders.h"

// A region represents the subtexture located on the atlas
typedef struct
{
    // Lower-left corner (U,V) in atlas (0–1)
    Vec2f_t uvMin;
    // Upper-right corner (U,V) in atlas (0–1)
    Vec2f_t uvMax;
} AtlasRegion_t;

typedef enum
{
    // Only included noteworthy ones for now
    DEBUG_0 = 0,
    DEBUG_1 = 1,
    DEBUG_2 = 2,
    DEBUG_3 = 3,
    DEBUG_4 = 4,
    DEBUG_5 = 5,
    NETHERRACK = 4,
    NOTE_BLOCK = 5,
    OBSIDIAN = 6,
    OAK_PLANKS = 15,
    MELON_SIDE = 33,
    MELON_TOP = 36,
} AtlasFace_t;

AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles);

void atlasDestroy(AtlasRegion_t *pAtlasRegions);