#pragma once

#include "cmath/cmath.h"

// A region represents the subtexture located on the atlas
typedef struct
{
    // Lower-left corner (U,V) in atlas (0–1)
    Vec2f_t uvMin;
    // Upper-right corner (U,V) in atlas (0–1)
    Vec2f_t uvMax;
} AtlasRegion_t;