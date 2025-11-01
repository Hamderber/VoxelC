#pragma once

typedef enum
{
    // Only included noteworthy ones for now
    ATLAS_FACE_DEBUG_0 = 0,
    ATLAS_FACE_DEBUG_1 = 1,
    ATLAS_FACE_DEBUG_2 = 2,
    ATLAS_FACE_DEBUG_3 = 3,
    ATLAS_FACE_DEBUG_4 = 4,
    ATLAS_FACE_DEBUG_5 = 5,
    // Error texture bc shouldnt render air
    ATLAS_FACE_AIR = 37,
    ATLAS_FACE_NETHERRACK = 3,
    ATLAS_FACE_NOTE_BLOCK = 4,
    ATLAS_FACE_OBSIDIAN = 5,
    ATLAS_FACE_OAK_PLANKS = 15,
    ATLAS_FACE_MELON_SIDE = 33,
    ATLAS_FACE_MELON_TOP = 36,
    ATLAS_FACE_UNDEFINED = 37,
    ATLAS_FACE_STONE = 179,
} AtlasFace_t;