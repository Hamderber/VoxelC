#pragma once

#include <stdbool.h>
#include "character/characterType_t.h"
#include "entity/entity_t.h"

typedef struct Character_t
{
    Entity_t *pEntity;
    CharacterType_t type;
    // This does NOT include the null terminator
    size_t nameLength;
    char *pName;
    bool sprintIntention;
    bool isSprinting;
} Character_t;