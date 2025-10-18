#pragma once

#include "entity/entity_t.h"

// Aribtrary size, probably need to change this in the future
#define ENTITIES_MAX_IN_COLLECTION 256

typedef enum
{
    ENTITY_COLLECTION_GENERIC = 0,
    ENTITY_COLLECTION_PHYSICS,
    ENTITY_COLLECTION_COUNT,
} EntityCollectionCategory_t;

typedef struct
{
    Entity_t **entities;
} EntityCollection_t;