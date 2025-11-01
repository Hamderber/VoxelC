#pragma once

#include <stdbool.h>
#include "character/characterType_t.h"

typedef struct
{
    CharacterType_t type;
    // This does NOT include the null terminator
    size_t nameLength;
    char *pName;
    bool sprintIntention;
    bool isSprinting;
} Character_t;