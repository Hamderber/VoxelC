#pragma once

typedef enum
{
    FACE_LEFT = 0,   // -X (left)
    FACE_RIGHT = 1,  // +X (right)
    FACE_TOP = 2,    // +Y (up)
    FACE_BOTTOM = 3, // -Y (down)
    FACE_FRONT = 4,  // +Z (front)
    FACE_BACK = 5,   // -Z (back)
} CubeFace_t;