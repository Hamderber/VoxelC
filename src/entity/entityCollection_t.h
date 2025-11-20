#pragma once

#include "entity/entity_t.h"

// Aribtrary size, probably need to change this in the future
#define ENTITIES_MAX_IN_COLLECTION 256

typedef enum EntityCollectionCategory_e
{
    ENTITY_COLLECTION_GENERIC = 0,
    ENTITY_COLLECTION_PHYSICS,
    ENTITY_COLLECTION_COUNT,
} EntityCollectionCategory_e;

typedef struct EntityCollection_t
{
    struct Entity_t **entities;
} EntityCollection_t;