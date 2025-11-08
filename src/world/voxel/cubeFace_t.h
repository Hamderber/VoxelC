#pragma once

#include "cmath/cmath.h"

typedef enum
{
    CUBE_FACE_LEFT = 0,   // -X (left)
    CUBE_FACE_RIGHT = 1,  // +X (right)
    CUBE_FACE_TOP = 2,    // +Y (up)
    CUBE_FACE_BOTTOM = 3, // -Y (down)
    CUBE_FACE_FRONT = 4,  // +Z (front)
    CUBE_FACE_BACK = 5,   // -Z (back)
} CubeFace_t;

// ORDER REQUIRED: [0]=TL, [1]=BL, [2]=TR, [3]=BR (viewed from OUTSIDE), CCW
static const Vec3i_t pFACE_POSITIONS[6][4] = {
    // LEFT  (-X), view from -X -> +X
    [CUBE_FACE_LEFT] = {
        {0, 1, 0}, // TL (0,1,0)
        {0, 0, 0}, // BL (0,0,0)
        {0, 1, 1}, // TR (0,1,1)
        {0, 0, 1}  // BR (0,0,1)
    },
    // RIGHT (+X), view from +X -> -X
    [CUBE_FACE_RIGHT] = {
        {1, 1, 1}, // TL (1,1,1)
        {1, 0, 1}, // BL (1,0,1)
        {1, 1, 0}, // TR (1,1,0)
        {1, 0, 0}  // BR (1,0,0)
    },
    // TOP (+Y), view from +Y (down), screen-up = -Z, screen-right = +X
    [CUBE_FACE_TOP] = {
        {0, 1, 0}, // TL (0,1,0)
        {0, 1, 1}, // BL (0,1,1)
        {1, 1, 0}, // TR (1,1,0)
        {1, 1, 1}  // BR (1,1,1)
    },
    // BOTTOM (-Y), view from -Y (up), screen-up = +Z, screen-right = +X
    [CUBE_FACE_BOTTOM] = {
        {0, 0, 1}, // TL (0,0,1)
        {0, 0, 0}, // BL (0,0,0)
        {1, 0, 1}, // TR (1,0,1)
        {1, 0, 0}  // BR (1,0,0)
    },
    // FRONT (+Z), view from +Z -> -Z
    [CUBE_FACE_FRONT] = {
        {0, 1, 1}, // TL (0,1,1)
        {0, 0, 1}, // BL (0,0,1)
        {1, 1, 1}, // TR (1,1,1)
        {1, 0, 1}  // BR (1,0,1)
    },
    // BACK  (-Z), view from -Z -> +Z
    [CUBE_FACE_BACK] = {
        {1, 1, 0}, // TL (1,1,0)
        {1, 0, 0}, // BL (1,0,0)
        {0, 1, 0}, // TR (0,1,0)
        {0, 0, 0}  // BR (0,0,0)
    },
};

static const int VERTS_PER_FACE = 4;
static const int INDICIES_PER_FACE = 6;
static const int CUBE_FACE_COUNT = 6;