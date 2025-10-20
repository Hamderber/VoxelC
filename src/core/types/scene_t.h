#pragma once

#include <stdint.h>
#include "rendering/types/renderModel_t.h"

typedef struct Scene_t
{
    RenderModel_t **models;
    uint32_t modelCount;
    uint32_t modelCapacity;
} Scene_t;