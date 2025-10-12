#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/logs.h"
#include "c_math/c_math.h"
#include "rendering/shaders.h"
#include "core/types/atlasRegion_t.h"
#include "core/types/atlasFace_t.h"

AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles);

void atlasDestroy(AtlasRegion_t *pAtlasRegions);