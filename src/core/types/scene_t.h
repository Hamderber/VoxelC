#pragma once

#include <stdint.h>
#include "rendering/types/renderModel_t.h"
#include "scene/SceneModelInstance_t.h"

typedef struct Scene_t
{
    SceneModelInstance_t *pModelInstances;
    uint32_t instCount;
    uint32_t instCap;
    RenderModel_t **ppModels;
    uint32_t modelCount;
    uint32_t modelCap;
} Scene_t;