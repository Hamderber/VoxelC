#pragma once

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "core/logs.h"
#include "cmath/cmath.h"
#include "rendering/shaders.h"
#include "core/types/atlasRegion_t.h"
#include "core/types/atlasFace_t.h"
#include "core/types/context_t.h"
#include "core/types/renderer_t.h"
#include "core/types/state_t.h"

void atlasTextureImageCreate(State_t *state);

void atlasTextureImageDestroy(State_t *state);

void atlasTextureViewImageCreate(State_t *state);

void atlasTextureImageViewDestroy(Context_t *context, Renderer_t *renderer);

AtlasRegion_t *atlasCreate(AtlasRegion_t *pAtlasRegions, uint32_t atlasRegionCount, uint32_t atlasWidthInTiles, uint32_t atlasHeightInTiles);

void atlasDestroy(AtlasRegion_t *pAtlasRegions);