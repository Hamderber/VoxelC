#pragma once

#include <stdbool.h>
#include "character/characterType_t.h"

typedef struct Character_t
{
    struct Entity_t *pEntity;
    enum CharacterType_t type;
    // This does NOT include the null terminator
    size_t nameLength;
    char *pName;
    bool sprintIntention;
    bool isSprinting;
} Character_t;