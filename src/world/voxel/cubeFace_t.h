#pragma once

#include "cmath/cmath.h"

typedef enum
{
    FACE_LEFT = 0,   // -X (left)
    FACE_RIGHT = 1,  // +X (right)
    FACE_TOP = 2,    // +Y (up)
    FACE_BOTTOM = 3, // -Y (down)
    FACE_FRONT = 4,  // +Z (front)
    FACE_BACK = 5,   // -Z (back)
} CubeFace_t;

// ORDER REQUIRED: [0]=TL, [1]=BL, [2]=TR, [3]=BR (viewed from OUTSIDE), CCW
static const Vec3f_t pFACE_POSITIONS[6][4] = {
    // LEFT  (-X), view from -X -> +X
    [FACE_LEFT] = {
        {0, 1, 0}, // TL (0,1,0)
        {0, 0, 0}, // BL (0,0,0)
        {0, 1, 1}, // TR (0,1,1)
        {0, 0, 1}  // BR (0,0,1)
    },
    // RIGHT (+X), view from +X -> -X
    [FACE_RIGHT] = {
        {1, 1, 1}, // TL (1,1,1)
        {1, 0, 1}, // BL (1,0,1)
        {1, 1, 0}, // TR (1,1,0)
        {1, 0, 0}  // BR (1,0,0)
    },
    // TOP (+Y), view from +Y (down), screen-up = -Z, screen-right = +X
    [FACE_TOP] = {
        {0, 1, 0}, // TL (0,1,0)
        {0, 1, 1}, // BL (0,1,1)
        {1, 1, 0}, // TR (1,1,0)
        {1, 1, 1}  // BR (1,1,1)
    },
    // BOTTOM (-Y), view from -Y (up), screen-up = +Z, screen-right = +X
    [FACE_BOTTOM] = {
        {0, 0, 1}, // TL (0,0,1)
        {0, 0, 0}, // BL (0,0,0)
        {1, 0, 1}, // TR (1,0,1)
        {1, 0, 0}  // BR (1,0,0)
    },
    // FRONT (+Z), view from +Z -> -Z
    [FACE_FRONT] = {
        {0, 1, 1}, // TL (0,1,1)
        {0, 0, 1}, // BL (0,0,1)
        {1, 1, 1}, // TR (1,1,1)
        {1, 0, 1}  // BR (1,0,1)
    },
    // BACK  (-Z), view from -Z -> +Z
    [FACE_BACK] = {
        {1, 1, 0}, // TL (1,1,0)
        {1, 0, 0}, // BL (1,0,0)
        {0, 1, 0}, // TR (0,1,0)
        {0, 0, 0}  // BR (0,0,0)
    },
};

static const int VERTS_PER_FACE = 4;
static const int INDICIES_PER_FACE = 6;
static const int FACE_COUNT = 6;