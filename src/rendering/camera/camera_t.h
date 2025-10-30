#pragma once

#include "cmath/cmath.h"

typedef struct
{
    float farClippingPlane;
    float nearClippingPlane;
    float fov;
    Quaternionf_t rotation;
} Camera_t;