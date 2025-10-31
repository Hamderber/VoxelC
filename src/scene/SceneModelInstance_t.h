#pragma once

#include "cmath/cmath.h"
#include "rendering/types/renderModel_t.h"

typedef struct
{
    RenderModel_t *pModel;
    Mat4c_t modelMatrix;
} SceneModelInstance_t;